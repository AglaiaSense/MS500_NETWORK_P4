/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#include "output_tensor_parser.h"
#include "ai_utils.h"
#include "apParams.flatbuffers_generated.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <esp_heap_caps.h>

#include <stdio.h>

#define DNN_HDR_SIZE (12)

/**
 * Tensor Parser Namespace
 */
namespace output_tensor_parser {

    struct InputDataInfo {
        uint8_t *addr;
        uint16_t lineNum;
        uint16_t pitch;
        size_t size;
    };

    struct OutputTensorInfo {
        float *address;
        size_t totalSize;
        size_t tensorNum;
        size_t *tensorDataNum;
    };

    struct Dimensions {
        uint8_t ordinal;
        uint16_t size;
        uint8_t serializationIndex;
        uint8_t padding;
    };

    struct OutputTensorApParams {
        uint8_t id;
        char *name;
        uint16_t numOfDimensions;
        uint8_t bitsPerElement;
        std::vector<Dimensions> vecDim;
        uint16_t shift;
        float scale;
        uint8_t format;
    };

    struct FrameOutputInfo {
        ai_utils::DnnHeader *dnnHeaderInfo = NULL;
        std::string networkType = "";
        std::vector<OutputTensorApParams> *outputApParams = NULL;
        OutputTensorInfo *outputBodyInfo = NULL;
    };

    int parseOutputTensor(const struct InputDataInfo *inputDataInfo, struct FrameOutputInfo *frameDataInfo);
    int cleanupFrameData(FrameOutputInfo *frameOutputTensorInfo);

} // namespace output_tensor_parser

using namespace std;

using namespace output_tensor_parser;

using namespace ai_utils;

#define MAX_AP_PARAM_SIZE 4084 // 4KB - 12B(Header)

typedef enum tagTensorDatatype {
    TYPE_SIGNED = 0,
    TYPE_UNSIGNED
} TensorDataType;

/*output tensor structures */
typedef struct {
    float *address;
    size_t total_size;
    size_t tensor_num;
    size_t *tensor_data_num;
} sc_output_tensor_info_t;

typedef struct {
    uint8_t frame_valid;
    uint8_t frame_count;
    uint16_t max_line_len;
    uint16_t ap_param_size;
    uint16_t network_id;
    uint8_t tensor_type;
} sc_dnn_header_info_t;

typedef struct {
    sc_dnn_header_info_t *dnn_hdr_info;
    char *network_type;
    uint16_t num_detections;
    sc_output_tensor_info_t *output_body_info;
} sc_frame_output_info_t;

typedef struct {
    /*output tensor*/
    sc_frame_output_info_t *p_output_tensor;

} sc_proc_dnn_output_t;

#define bytes_to_uint16(MSB, LSB) (((uint16_t)((unsigned char)MSB)) & 255) << 8 | (((unsigned char)LSB) & 255)
#define bytes_to_int16(MSB, LSB) (((int16_t)((unsigned char)MSB)) & 255) << 8 | (((unsigned char)LSB) & 255)
#define extract_byte(MSB, LSB) (((((unsigned char)MSB) << 6) & 0xc0) | ((((unsigned char)LSB) & 0xfc) >> 2))
#define DIMENSION_MAX 3

float *outputTensorAddress = NULL;

static void cleanup_memory(sc_proc_dnn_output_t *p_dnn_proc_result, output_tensor_parser::FrameOutputInfo *output_tensor_mem);

/* Parse AP Params */
static int parseApParams(const char *bodyApParams, const DnnHeader *dnnHeader, FrameOutputInfo *frameOutputInfo) {
    const apParams::fb::FBApParams *fbApParams;
    const apParams::fb::FBNetwork *fbNetwork;
    const apParams::fb::FBOutputTensor *fbOutputTensor;

    fbApParams = apParams::fb::GetFBApParams(bodyApParams);
    // printf("[OUT_TENSOR] Networks size:%ld\n", fbApParams->networks()->size());

    for (size_t i = 0; i < fbApParams->networks()->size(); i++) {
        fbNetwork = (apParams::fb::FBNetwork *)(fbApParams->networks()->Get(i));
        if (fbNetwork->id() == dnnHeader->networkId) {
            frameOutputInfo->networkType = (char *)fbNetwork->type()->c_str();
            // printf("[OUT_TENSOR] Network type: %s\n", (char*)fbNetwork->type()->c_str());
            // printf("[OUT_TENSOR] Input Tensors size:%ld\n", fbNetwork->inputTensors()->size());
            // printf("[OUT_TENSOR] Output Tensors size:%ld\n", fbNetwork->outputTensors()->size());

            for (size_t j = 0; j < fbNetwork->outputTensors()->size(); j++) {
                OutputTensorApParams outApParam;

                fbOutputTensor = (apParams::fb::FBOutputTensor *)fbNetwork->outputTensors()->Get(j);

                outApParam.id = fbOutputTensor->id();
                outApParam.name = (char *)fbOutputTensor->name()->c_str();
                outApParam.numOfDimensions = fbOutputTensor->numOfDimensions();

                for (uint8_t k = 0; k < fbOutputTensor->numOfDimensions(); k++) {
                    Dimensions dim;
                    dim.ordinal = fbOutputTensor->dimensions()->Get(k)->id();
                    dim.size = fbOutputTensor->dimensions()->Get(k)->size();
                    dim.serializationIndex = fbOutputTensor->dimensions()->Get(k)->serializationIndex();
                    dim.padding = fbOutputTensor->dimensions()->Get(k)->padding();
                    if (dim.padding != 0) {
                        printf("[OUT_TENSOR] Error in AP Params, Non-Zero padding for Dimension %d", k);
                        return -1;
                    }

                    outApParam.vecDim.push_back(dim);
                }

                outApParam.bitsPerElement = fbOutputTensor->bitsPerElement();
                outApParam.shift = fbOutputTensor->shift();
                outApParam.scale = fbOutputTensor->scale();
                outApParam.format = fbOutputTensor->format();

                /* Add the element to vector */
                frameOutputInfo->outputApParams->push_back(outApParam);
            }
            break;
        }
    }

    return 0;
}

/* Parse Output Tensor Header data consisting of DNN Haeder and AP Params */
static int parseOutputTensorHeader(const uint8_t *src, FrameOutputInfo *frameOutputTensorInfo) {

    const uint8_t *readPonter = src;

    /* Extract Dnn Header */
    ai_utils::extract_dnn_header(readPonter, frameOutputTensorInfo->dnnHeaderInfo);
    readPonter += DNN_HDR_SIZE;

    if (frameOutputTensorInfo->dnnHeaderInfo->frameValid != 1) {
        printf("[OUT_TENSOR] Invalid Frame, valid: %d\n", frameOutputTensorInfo->dnnHeaderInfo->frameValid);
        return 1;
    }

    if (frameOutputTensorInfo->dnnHeaderInfo->tensorType != TYPE_OUTPUT_TENSOR) {
        // printf("[OUT_TENSOR] Wrong tensorType: %d\n", frameOutputTensorInfo->dnnHeaderInfo->tensorType);
        return -1;
    }

    if (frameOutputTensorInfo->dnnHeaderInfo->apParamSize > MAX_AP_PARAM_SIZE) {
        printf("[OUT_TENSOR] Invalid AP Param size: %d\n", frameOutputTensorInfo->dnnHeaderInfo->apParamSize);
        return -1;
    }

    /*CodeSonar Fix*/
    if (frameOutputTensorInfo->dnnHeaderInfo->apParamSize == 0) {
        printf("[OUT_TENSOR] Invalid AP Param size: %d\n", frameOutputTensorInfo->dnnHeaderInfo->apParamSize);
        return -1;
    }

    if (frameOutputTensorInfo->dnnHeaderInfo->maxLineLen == 0) {
        printf("[OUT_TENSOR] Invalid max line length: 0\n");
        return -1;
    }

    char *ap_params = new char[frameOutputTensorInfo->dnnHeaderInfo->apParamSize];
    if (ap_params == NULL) {
        printf("[OUT_TENSOR] Error allocating memory\n");
        return -1;
    }

    memset(ap_params, 0, frameOutputTensorInfo->dnnHeaderInfo->apParamSize);

    for (int j = 0; j < frameOutputTensorInfo->dnnHeaderInfo->apParamSize; j++) {
        ap_params[j] = (uint8_t)(*readPonter++);
    }

    int ret = parseApParams((const char *)ap_params, (const DnnHeader *)frameOutputTensorInfo->dnnHeaderInfo, frameOutputTensorInfo);
    if (ret != 0) {
        printf("[OUT_TENSOR] Error parsing AP Params ret =%d\n", ret);
        delete[] ap_params;
        return ret;
    }

    delete[] ap_params;
    return 0;
}

/* Parse Output Tensor Body data */
static int parseOutputTensorBody(const uint8_t *src, const uint16_t lineSize, FrameOutputInfo *frameOutputTensorInfo) {
    /* 输入参数验证 - 保持不变 */
    if (src == NULL) {
        printf("[OUT_TENSOR] Error src address is NULL\n");
        return -1;
    }

    if (frameOutputTensorInfo == NULL || frameOutputTensorInfo->outputBodyInfo->address == NULL) {
        printf("[OUT_TENSOR] Error Header info  is NULL\n");
        return -1;
    }

    if (frameOutputTensorInfo->outputApParams == NULL) {
        printf("[OUT_TENSOR] Error outApParams is NULL\n");
        return -1;
    }

    int32_t retStatus = 0;
    uint8_t *tmp_dst_buf = NULL; // 确保指针初始化

    /* 内存分配检查 - 保持不变 */
    float *dst = frameOutputTensorInfo->outputBodyInfo->address;
    if (frameOutputTensorInfo->outputBodyInfo->totalSize > (UINT32_MAX / sizeof(float))) {
        printf("[OUT_TENSOR] Error totalSize is greater than maximum size\n");
        return -1;
    }

    /* 分配临时缓冲区 - 保持不变 */
    tmp_dst_buf = (uint8_t *)heap_caps_malloc(frameOutputTensorInfo->outputBodyInfo->totalSize * sizeof(float), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // printf("%s(%d): \n", __func__, __LINE__);
    // printf( "Free SPIRAM memory: %u KB \n", (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

    if (tmp_dst_buf == NULL) {
        printf("[OUT_TENSOR] ERROR: malloc fails, size = %d\n", frameOutputTensorInfo->outputBodyInfo->totalSize * sizeof(float));
        return -1;
    }

    float *tmp_dst = (float *)(tmp_dst_buf);
    size_t len = frameOutputTensorInfo->outputBodyInfo->totalSize * sizeof(float);
    memset(tmp_dst, 0, len);

    /* 元数据计算 - 保持不变 */
    std::vector<uint16_t> numOfLinesVec(frameOutputTensorInfo->outputApParams->size());
    std::vector<size_t> outSizes(frameOutputTensorInfo->outputApParams->size());
    std::vector<size_t> offsets(frameOutputTensorInfo->outputApParams->size());
    std::vector<const uint8_t *> src_arr(frameOutputTensorInfo->outputApParams->size());
    std::vector<std::vector<Dimensions>> serialized_dims;
    std::vector<std::vector<Dimensions>> actual_dims;
    {
        auto src1 = src;
        size_t offset = 0;
        auto src = src1;
        for (unsigned int tensorIdx = 0; tensorIdx < frameOutputTensorInfo->outputApParams->size(); tensorIdx++) {
            offsets[tensorIdx] = offset;
            src_arr[tensorIdx] = src;
            size_t TensorDataNum = 0;

            OutputTensorApParams param = frameOutputTensorInfo->outputApParams->at(tensorIdx);
            size_t outputTensorSize = 0;
            size_t tensorOutSize = (param.bitsPerElement / 8);
            std::vector<Dimensions> serialized_dim(param.numOfDimensions);
            std::vector<Dimensions> actual_dim(param.numOfDimensions);

            for (int idx = 0; idx < param.numOfDimensions; idx++) {
                actual_dim[idx].size = param.vecDim.at(idx).size;
                serialized_dim[param.vecDim.at(idx).serializationIndex].size = param.vecDim.at(idx).size;

                tensorOutSize *= param.vecDim.at(idx).size;
                if (tensorOutSize >= UINT32_MAX / param.bitsPerElement / 8) {
                    printf("Invalid output tensor info");
                    retStatus = -1;
                    break;
                }

                actual_dim[idx].serializationIndex = param.vecDim.at(idx).serializationIndex;
                serialized_dim[param.vecDim.at(idx).serializationIndex].serializationIndex = (uint8_t)idx;
            }

            uint16_t numOfLines = (uint16_t)ceil(tensorOutSize / (float)frameOutputTensorInfo->dnnHeaderInfo->maxLineLen);
            outputTensorSize = tensorOutSize;
            numOfLinesVec[tensorIdx] = numOfLines;
            outSizes[tensorIdx] = tensorOutSize;

            serialized_dims.push_back(serialized_dim);
            actual_dims.push_back(actual_dim);

            src += numOfLines * lineSize;
            TensorDataNum = (outputTensorSize / (param.bitsPerElement / 8));
            offset += TensorDataNum;
            frameOutputTensorInfo->outputBodyInfo->tensorDataNum[tensorIdx] = TensorDataNum;
            if (offset > frameOutputTensorInfo->outputBodyInfo->totalSize) {
                printf("Error in Prasing output tensor offset(%d)>output_size", offset);
                retStatus = -1;
                break;
            }
        }
    }

    /* 线程处理 - 主要修改内存释放部分 */
    if (retStatus == 0) { // 仅当元数据计算成功时才创建线程
        std::thread threads[frameOutputTensorInfo->outputApParams->size()];

        std::vector<size_t> idxs(frameOutputTensorInfo->outputApParams->size());
        for (unsigned int i = 0; i < idxs.size(); i++) {
            idxs[i] = i;
        }

        for (unsigned int i = 0; i < idxs.size(); i++) {
            for (unsigned int j = 0; j < idxs.size(); j++) {
                if (numOfLinesVec[idxs[i]] > numOfLinesVec[idxs[j]]) {
                    int temp = idxs[i];
                    idxs[i] = idxs[j];
                    idxs[j] = temp;
                }
            }
        }

        // 创建线程局部变量捕获列表
        for (unsigned int i = 0; i < idxs.size(); i++) {
            auto tensorIdx = idxs[i];
            threads[tensorIdx] = std::move(std::thread([=, &retStatus] { // 按值捕获tensorIdx，引用捕获retStatus
                size_t outputTensorSize = outSizes[tensorIdx];
                uint16_t numOfLines = numOfLinesVec[tensorIdx];
                bool sortingRequired = false;

                OutputTensorApParams param = frameOutputTensorInfo->outputApParams->at(tensorIdx);
                std::vector<Dimensions> serialized_dim = serialized_dims[tensorIdx];
                std::vector<Dimensions> actual_dim = actual_dims[tensorIdx];
                const uint8_t *local_src = src_arr[tensorIdx];
                size_t local_offset = offsets[tensorIdx];

                for (int idx = 0; idx < param.numOfDimensions; idx++) {
                    if (param.vecDim.at(idx).serializationIndex != param.vecDim.at(idx).ordinal) {
                        sortingRequired = true;
                    }
                }

                if (outputTensorSize == 0) {
                    printf("Invalid output tensorsize = %d", outputTensorSize);
                    retStatus = -1;
                    return; // 不再尝试释放内存
                }

                /* 数据处理 - 完全保持原有逻辑 */
                size_t elementIndex = 0;
                if (param.bitsPerElement == 8) {
                    for (int i = 0; i < numOfLines; i++) {
                        int lineIndex = 0;
                        while (lineIndex < frameOutputTensorInfo->dnnHeaderInfo->maxLineLen) {
                            if (param.format == TYPE_SIGNED) {
                                int8_t temp = (int8_t)*(local_src + lineIndex);
                                float value = (temp - param.shift) * param.scale;
                                tmp_dst[local_offset + elementIndex] = value;
                            } else {
                                uint8_t temp = (uint8_t)*(local_src + lineIndex);
                                float value = (temp - param.shift) * param.scale;
                                tmp_dst[local_offset + elementIndex] = value;
                            }
                            elementIndex++;
                            lineIndex++;
                            if (elementIndex == outputTensorSize) {
                                break;
                            }
                        }
                        local_src += lineSize;
                        if (elementIndex == outputTensorSize) {
                            break;
                        }
                    }
                } else if (param.bitsPerElement == 16) {
                    for (int i = 0; i < numOfLines; i++) {
                        int lineIndex = 0;
                        while (lineIndex < frameOutputTensorInfo->dnnHeaderInfo->maxLineLen) {
                            if (param.format == TYPE_SIGNED) {
                                int16_t temp = bytes_to_int16((int8_t)*(local_src + lineIndex + 1), (int8_t)*(local_src + lineIndex));
                                float value = (temp - param.shift) * param.scale;
                                tmp_dst[local_offset + elementIndex] = value;
                            } else {
                                uint16_t temp = bytes_to_uint16((uint8_t)*(local_src + lineIndex + 1), (uint8_t)*(local_src + lineIndex));
                                float value = (temp - param.shift) * param.scale;
                                tmp_dst[local_offset + elementIndex] = value;
                            }
                            elementIndex++;
                            lineIndex += 2;
                            if (elementIndex >= (outputTensorSize >> 1)) {
                                break;
                            }
                        }
                        local_src += lineSize;
                        if (elementIndex >= (outputTensorSize >> 1)) {
                            break;
                        }
                    }
                } else {
                    printf("Invalid bitsPerElement value =%d", param.bitsPerElement);
                    retStatus = -1;
                    return; // 不再尝试释放内存
                }

                /* 维度重排 - 完全保持原有逻辑 */
                if (sortingRequired) {
                    size_t loop_cnt[DIMENSION_MAX] = {1, 1, 1};
                    size_t coef[DIMENSION_MAX] = {1, 1, 1};
                    for (size_t i = 0; i < param.numOfDimensions; i++) {
                        if (i >= DIMENSION_MAX) {
                            printf("[OUT_TENSOR] numOfDimensions value is 3 or higher\n");
                            break;
                        }

                        loop_cnt[i] = serialized_dim.at(i).size;

                        for (size_t j = serialized_dim.at(i).serializationIndex; j > 0; j--) {
                            coef[i] *= actual_dim.at(j - 1).size;
                        }
                    }

                    size_t src_index = 0;
                    size_t dst_index;
                    for (size_t i = 0; i < loop_cnt[DIMENSION_MAX - 1]; i++) {
                        for (size_t j = 0; j < loop_cnt[DIMENSION_MAX - 2]; j++) {
                            for (size_t k = 0; k < loop_cnt[DIMENSION_MAX - 3]; k++) {
                                dst_index = (coef[DIMENSION_MAX - 1] * i) + (coef[DIMENSION_MAX - 2] * j) + (coef[DIMENSION_MAX - 3] * k);
                                dst[local_offset + dst_index] = tmp_dst[local_offset + src_index++];
                            }
                        }
                    }
                } else {
                    if (param.bitsPerElement == 8) {
                        memcpy(dst + local_offset, tmp_dst + local_offset, outputTensorSize * sizeof(float));
                    } else if (param.bitsPerElement == 16) {
                        memcpy(dst + local_offset, tmp_dst + local_offset, (outputTensorSize >> 1) * sizeof(float));
                    } else {
                        printf("Invalid bitsPerElement value =%d", param.bitsPerElement);
                        retStatus = -1;
                        return; // 不再尝试释放内存
                    }
                }
            }));
        }

        for (unsigned int i = 0; i < frameOutputTensorInfo->outputApParams->size(); i++) {
            threads[i].join();
        }
    }

    /* 统一内存释放点 - 关键修改 */
    if (tmp_dst_buf != NULL) {
        heap_caps_free(tmp_dst_buf);
        tmp_dst_buf = NULL;
    }

    return retStatus;
}

int output_tensor_parser::parseOutputTensor(const struct InputDataInfo *in, struct FrameOutputInfo *frameOutTensorInfo) {
    size_t output_size = 0;
    struct PacketHeader *refpacketHeader = new PacketHeader();
    frameOutTensorInfo->dnnHeaderInfo = new DnnHeader();
    frameOutTensorInfo->outputApParams = new std::vector<OutputTensorApParams>();
    uint8_t *src;
    int retStatus = 0;

    /* Get input Data pointer*/
    src = in->addr;

    /* Parse output tensor Header Data */
    retStatus = parseOutputTensorHeader(src, frameOutTensorInfo);
    if (retStatus != 0) {
        delete refpacketHeader;
        return retStatus;
    }

    /*Calculate total output size*/
    size_t numOutputTensors = frameOutTensorInfo->outputApParams->size();
    size_t totalOutSize = 0;
    for (size_t k = 0; k < numOutputTensors; k++) {
        size_t totalDimensionSize = 1;
        for (int idx = 0; idx < frameOutTensorInfo->outputApParams->at(k).numOfDimensions; idx++) {
            if (totalDimensionSize >= UINT32_MAX / frameOutTensorInfo->outputApParams->at(k).vecDim.at(idx).size) {
                printf("[OUT_TENSOR] Invalid output tensor info");
                delete refpacketHeader;
                return -1;
            }

            totalDimensionSize *= frameOutTensorInfo->outputApParams->at(k).vecDim.at(idx).size;
        }

        if (totalOutSize >= UINT32_MAX - totalDimensionSize) {
            printf("[OUT_TENSOR] Invalid output tensor info\n");
            delete refpacketHeader;
            return -1;
        }

        totalOutSize += totalDimensionSize;
    }

    /* CodeSonar Check */
    if (totalOutSize == 0) {
        printf("[OUT_TENSOR] Invalid output tensor info(totalOutSize is 0)\n");
        delete refpacketHeader;
        return -1;
    }

    output_size = totalOutSize;
    // printf("[OUT_TENSOR] final output size = %d\n", output_size);

    if (output_size >= UINT32_MAX / sizeof(float)) {
        printf("[OUT_TENSOR] Invalid output tensor info");
        delete refpacketHeader;
        return -1;
    }

    /* Set FrameOutputTensor Info */
    frameOutTensorInfo->outputBodyInfo = new OutputTensorInfo();
    if (frameOutTensorInfo->outputBodyInfo == NULL) {
        printf("[OUT_TENSOR][ERROR] Memory alloc for outputBodyInfo failed !!\n");
        delete refpacketHeader;
        return -1;
    }

    memset(frameOutTensorInfo->outputBodyInfo, 0, sizeof(struct OutputTensorInfo));

    frameOutTensorInfo->outputBodyInfo->address = (float *)new float[output_size];
    if (frameOutTensorInfo->outputBodyInfo->address == NULL) {
        printf("[OUT_TENSOR] Error allocating memory for output tensor\n");
        delete refpacketHeader;
        delete frameOutTensorInfo->outputBodyInfo;
        frameOutTensorInfo->outputBodyInfo = NULL;
        return -1;
    }

    /* CodeSonar Check */
    if (numOutputTensors == 0) {
        printf("[OUT_TENSOR] Invalid numOutputTensors (numOutputTensors is 0)\n");
        delete refpacketHeader;
        delete[] frameOutTensorInfo->outputBodyInfo->address;
        frameOutTensorInfo->outputBodyInfo->address = NULL;
        delete frameOutTensorInfo->outputBodyInfo;
        frameOutTensorInfo->outputBodyInfo = NULL;
        return -1;
    }

    if (numOutputTensors >= UINT32_MAX / sizeof(size_t)) {
        printf("[OUT_TENSOR] Invalid numOutputTensors");
        delete refpacketHeader;
        delete[] frameOutTensorInfo->outputBodyInfo->address;
        frameOutTensorInfo->outputBodyInfo->address = NULL;
        delete frameOutTensorInfo->outputBodyInfo;
        frameOutTensorInfo->outputBodyInfo = NULL;
        return -1;
    }

    frameOutTensorInfo->outputBodyInfo->totalSize = output_size;
    frameOutTensorInfo->outputBodyInfo->tensorNum = numOutputTensors;
    frameOutTensorInfo->outputBodyInfo->tensorDataNum = (size_t *)new size_t[numOutputTensors];
    if (frameOutTensorInfo->outputBodyInfo->tensorDataNum == NULL) {
        printf("[OUT_TENSOR] Error allocating memory for output tensor size\n");
        delete refpacketHeader;
        delete[] frameOutTensorInfo->outputBodyInfo->address;
        frameOutTensorInfo->outputBodyInfo->address = NULL;
        delete frameOutTensorInfo->outputBodyInfo;
        frameOutTensorInfo->outputBodyInfo = NULL;
        return -1;
    }

    src += DNN_HDR_SIZE + frameOutTensorInfo->dnnHeaderInfo->apParamSize;
    uint16_t lineSize = frameOutTensorInfo->dnnHeaderInfo->maxLineLen;

    /*Parse Output Tensor Body*/
    retStatus = parseOutputTensorBody(src, lineSize, frameOutTensorInfo);
    if (retStatus != 0) {
        printf("[OUT_TENSOR] Error parsing output tensor body ret=%d\n", retStatus);
    }

    delete refpacketHeader;
    // printf("[OUT_TENSOR] Parsing OutputTensor Complete!!!\n");
    return retStatus;
}

int output_tensor_parser::cleanupFrameData(FrameOutputInfo *frameOutputTensorInfo) {
    if (frameOutputTensorInfo->outputBodyInfo != NULL) {
        if (frameOutputTensorInfo->outputBodyInfo->tensorDataNum != NULL) {
            delete[] frameOutputTensorInfo->outputBodyInfo->tensorDataNum;
        }

        if (frameOutputTensorInfo->outputBodyInfo->address != NULL) {
            delete[] frameOutputTensorInfo->outputBodyInfo->address;
        }

        delete frameOutputTensorInfo->outputBodyInfo;
    }

    if (frameOutputTensorInfo->dnnHeaderInfo != NULL) {
        delete frameOutputTensorInfo->dnnHeaderInfo;
    }

    if (frameOutputTensorInfo->outputApParams != NULL) {
        delete frameOutputTensorInfo->outputApParams;
    }

    return 0;
}

// static DecodeFunc s_decode_func = nullptr;

// void register_decode_callback(DecodeFunc func) {
//     s_decode_func = func;
// }


/**
 * @brief process_dnn_buffer
 */
extern "C" int process_dnn_buffer(uint8_t *dnn_data, size_t dnn_size) {
    int ret;
    output_tensor_parser::InputDataInfo inputInfo;
    output_tensor_parser::FrameOutputInfo frameOutputInfo;
    frameOutputInfo.outputBodyInfo = NULL;
    frameOutputInfo.dnnHeaderInfo = NULL;
    frameOutputInfo.outputApParams = NULL;

    inputInfo.addr = dnn_data;
    inputInfo.size = dnn_size;

    ret = output_tensor_parser::parseOutputTensor(&inputInfo, &frameOutputInfo);
    if (ret != 0) {
        if (ret != 1) {
            // printf("[OUT_TENSOR] Error parsing Output Tensor ret=%d\n", ret);
        }
        output_tensor_parser::cleanupFrameData(&frameOutputInfo);
        /* [NOTE]                                                                           */
        /*  Hold input tensor memory in consideration of the following frame straddling.    */
        /* free(input_tensor_memory); */
        return -1;
    }
    // printf("[OUT_TENSOR] output_tensor_parser out\n");

    //===========================================
    // set DNN result
    // prepare DNN result to be passed to the CALLBACK registered by the App
    sc_proc_dnn_output_t *p_dnn_proc_result = new sc_proc_dnn_output_t();
    if (p_dnn_proc_result == NULL) {
        printf("[OUT_TENSOR] memory alloc fails <<1>>\n");
        cleanup_memory(p_dnn_proc_result, &frameOutputInfo);
        return -1;
    }

    memset(p_dnn_proc_result, 0, sizeof(sc_proc_dnn_output_t));
    p_dnn_proc_result->p_output_tensor = NULL;

    p_dnn_proc_result->p_output_tensor = new sc_frame_output_info_t();
    if (p_dnn_proc_result->p_output_tensor == NULL) {
        printf("[OUT_TENSOR] memory alloc fails <<4>>\n");
        cleanup_memory(p_dnn_proc_result, &frameOutputInfo);
        return -1;
    }

    memset(p_dnn_proc_result->p_output_tensor, 0, sizeof(sc_frame_output_info_t));

    p_dnn_proc_result->p_output_tensor->dnn_hdr_info = new sc_dnn_header_info_t();
    if (p_dnn_proc_result->p_output_tensor->dnn_hdr_info == NULL) {
        printf("[OUT_TENSOR] memory alloc fails <<5>>\n");
        cleanup_memory(p_dnn_proc_result, &frameOutputInfo);
        return -1;
    }

    memset(p_dnn_proc_result->p_output_tensor->dnn_hdr_info, 0, sizeof(sc_dnn_header_info_t));

    p_dnn_proc_result->p_output_tensor->dnn_hdr_info->frame_valid = frameOutputInfo.dnnHeaderInfo->frameValid;
    p_dnn_proc_result->p_output_tensor->dnn_hdr_info->frame_count = frameOutputInfo.dnnHeaderInfo->frameCount;
    p_dnn_proc_result->p_output_tensor->dnn_hdr_info->max_line_len = frameOutputInfo.dnnHeaderInfo->maxLineLen;
    p_dnn_proc_result->p_output_tensor->dnn_hdr_info->ap_param_size = frameOutputInfo.dnnHeaderInfo->apParamSize;
    p_dnn_proc_result->p_output_tensor->dnn_hdr_info->network_id = frameOutputInfo.dnnHeaderInfo->networkId;
    p_dnn_proc_result->p_output_tensor->dnn_hdr_info->tensor_type = frameOutputInfo.dnnHeaderInfo->tensorType;

    p_dnn_proc_result->p_output_tensor->network_type = (char *)frameOutputInfo.networkType.c_str();

    p_dnn_proc_result->p_output_tensor->num_detections = 0;
    if (frameOutputInfo.outputApParams != NULL) {
        p_dnn_proc_result->p_output_tensor->num_detections = frameOutputInfo.outputApParams->at(0).vecDim.at(0).size;
    }

    p_dnn_proc_result->p_output_tensor->output_body_info = new sc_output_tensor_info_t();
    if (p_dnn_proc_result->p_output_tensor->output_body_info == NULL) {
        printf("[OUT_TENSOR] memory alloc fails <<6>>\n");
        cleanup_memory(p_dnn_proc_result, &frameOutputInfo);
        return -1;
    }

    memset(p_dnn_proc_result->p_output_tensor->output_body_info, 0, sizeof(sc_output_tensor_info_t));

    p_dnn_proc_result->p_output_tensor->output_body_info->total_size = frameOutputInfo.outputBodyInfo->totalSize;
    p_dnn_proc_result->p_output_tensor->output_body_info->tensor_num = frameOutputInfo.outputBodyInfo->tensorNum;

    p_dnn_proc_result->p_output_tensor->output_body_info->tensor_data_num = (size_t *)new size_t[frameOutputInfo.outputBodyInfo->tensorNum];
    if (p_dnn_proc_result->p_output_tensor->output_body_info->tensor_data_num == NULL) {
        printf("[OUT_TENSOR] memory alloc fails <<7>>\n");
        cleanup_memory(p_dnn_proc_result, &frameOutputInfo);
        return -1;
    }

    memset(p_dnn_proc_result->p_output_tensor->output_body_info->tensor_data_num, 0, (sizeof(size_t) * frameOutputInfo.outputBodyInfo->tensorNum));

    p_dnn_proc_result->p_output_tensor->output_body_info->address = (float *)new float[frameOutputInfo.outputBodyInfo->totalSize];
    if (p_dnn_proc_result->p_output_tensor->output_body_info->address == NULL) {
        printf("[OUT_TENSOR] memory alloc fails <<8>>\n");
        cleanup_memory(p_dnn_proc_result, &frameOutputInfo);
        return -1;
    }

    memset(p_dnn_proc_result->p_output_tensor->output_body_info->address, 0, (sizeof(float) * frameOutputInfo.outputBodyInfo->totalSize));

    float *src = frameOutputInfo.outputBodyInfo->address;
    float *dst = p_dnn_proc_result->p_output_tensor->output_body_info->address;
    size_t data_count = 0;

    for (size_t i = 0; i < frameOutputInfo.outputBodyInfo->tensorNum; i++) {
        for (size_t j = 0; j < frameOutputInfo.outputBodyInfo->tensorDataNum[i]; j++) {

            dst[j] = src[j];

            /*CodeSonar Fix*/
            data_count++;
            if (data_count >= frameOutputInfo.outputBodyInfo->totalSize) {
                break;
            }
        }

        p_dnn_proc_result->p_output_tensor->output_body_info->tensor_data_num[i] = frameOutputInfo.outputBodyInfo->tensorDataNum[i];

        if (data_count >= frameOutputInfo.outputBodyInfo->totalSize) {
            break;
        }

        dst += frameOutputInfo.outputBodyInfo->tensorDataNum[i];
        src += frameOutputInfo.outputBodyInfo->tensorDataNum[i];
    }

    // if (s_decode_func) {
    //     return s_decode_func(p_dnn_proc_result->p_output_tensor->output_body_info->address);
    // }

    decode_dnn_output(p_dnn_proc_result->p_output_tensor->output_body_info->address);

    cleanup_memory(p_dnn_proc_result, &frameOutputInfo);

    return 0;
}

/**
 * @brief cleanup_dnn_proc_result
 */
static void cleanup_dnn_proc_result(sc_proc_dnn_output_t *p_dnn_proc_result) {
    if (p_dnn_proc_result != NULL) {
        if (p_dnn_proc_result->p_output_tensor) {
            if (p_dnn_proc_result->p_output_tensor->output_body_info) {
                if (p_dnn_proc_result->p_output_tensor->output_body_info->address) {
                    delete[] p_dnn_proc_result->p_output_tensor->output_body_info->address;
                }

                if (p_dnn_proc_result->p_output_tensor->output_body_info->tensor_data_num) {
                    delete[] p_dnn_proc_result->p_output_tensor->output_body_info->tensor_data_num;
                }

                delete p_dnn_proc_result->p_output_tensor->output_body_info;
            }

            if (p_dnn_proc_result->p_output_tensor->dnn_hdr_info) {
                delete p_dnn_proc_result->p_output_tensor->dnn_hdr_info;
            }

            delete p_dnn_proc_result->p_output_tensor;
        }

        delete p_dnn_proc_result;
    }
}

/**
 * @brief cleanup_memory
 */
static void cleanup_memory(sc_proc_dnn_output_t *p_dnn_proc_result, output_tensor_parser::FrameOutputInfo *output_tensor_mem) {
    /* creanup DNN process result */
    cleanup_dnn_proc_result(p_dnn_proc_result);

    /* creanup OutputTensor memory */
    output_tensor_parser::cleanupFrameData(output_tensor_mem);
}
