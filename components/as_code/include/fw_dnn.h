/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef __FW_DNN_H__
#define __FW_DNN_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define MAX_NUM_OF_NETWORKS         (3)  /* 最大支持的神经网络数量 */

typedef enum {
    E_DNN_TYPE_SSDMOBILENET = 0,
    E_DNN_TYPE_MOBILENETV1,
    E_DNN_TYPE_INPUTTENSOR_ONLY,
    E_DNN_TYPE_CUSTOM,
    E_DNN_TYPE_MAX
} sc_option_dnn_type_t;

typedef enum {
    BAYER_CH_R = 0,
    BAYER_CH_GR,
    BAYER_CH_GB,
    BAYER_CH_B,
    BAYER_CH_MAX
} E_BAYER_CH;

typedef struct  {
    size_t dimSize;
    uint8_t padding;
    uint8_t bytePerElement;
} sc_dnn_cfg_output_tensor_size_info_t;

typedef struct  {
    size_t tensorSize;
    size_t dimSize;
    uint8_t padding;
    uint8_t bytePerElement;
} sc_output_tensor_size_info_t;

typedef struct  {
    uint16_t add;
    uint8_t shift;
    uint16_t clipMax;
    uint16_t clipMin;
} sc_input_tensor_rgb_norm_info_t;

typedef struct  {
    uint16_t inputTensorWidth;
    uint16_t inputTensorHeight;
    uint8_t  inputTensorFormat;
    uint16_t NormK00;
    uint16_t NormK02;
    uint16_t NormK03;
    uint16_t NormK11;
    uint16_t NormK13;
    uint16_t NormK20;
    uint16_t NormK22;
    uint16_t NormK23;
    size_t yClip;
    size_t cbClip;
    size_t crClip;
    uint8_t yGgain;
    uint16_t yAdd;
    uint16_t rgbNormR;
    uint8_t rgbShiftR;
    uint16_t rgbClipMaxR;
    uint16_t rgbClipMinR;
    uint16_t rgbNormGr;
    uint8_t rgbShiftGr;
    uint16_t rgbClipMaxGr;
    uint16_t rgbClipMinGr;
    uint16_t rgbNormGb;
    uint8_t rgbShiftGb;
    uint16_t rgbClipMaxGb;
    uint16_t rgbClipMinGb;
    uint16_t rgbNormB;
    uint8_t rgbShiftB;
    uint16_t rgbClipMaxB;
    uint16_t rgbClipMinB;
    uint8_t  outputTensorNum;
    sc_dnn_cfg_output_tensor_size_info_t* p_outputTensorSizeInfo;
} sc_dnn_cfg_nw_info_t;

typedef struct  {
    uint16_t dnnHeaderSize;
    uint16_t inputTensorWidth;
    uint16_t inputTensorHeight;
    size_t inputTensorWidthStride;
    size_t inputTensorHeightStride;
    size_t inputTensorSize;
    size_t inputTensorFormat;
    uint16_t NormK00;
    uint16_t NormK01;
    uint16_t NormK02;
    uint16_t NormK03;
    uint16_t NormK10;
    uint16_t NormK11;
    uint16_t NormK12;
    uint16_t NormK13;
    uint16_t NormK20;
    uint16_t NormK21;
    uint16_t NormK22;
    uint16_t NormK23;
    size_t yClip;
    size_t cbClip;
    size_t crClip;
    uint8_t yGgain;
    uint8_t ySht;
    uint16_t yAdd;
    sc_input_tensor_rgb_norm_info_t rgbNorm[BAYER_CH_MAX];
    uint8_t  outputTensorNum;
    sc_output_tensor_size_info_t* p_outputTensorSizeInfo;
} sc_dnn_nw_info_t;

typedef struct {
    sc_option_dnn_type_t dnn_network_type;
    size_t dnn_network_id;
    uint8_t num_of_networks;
    uint16_t dnnHeaderSize;
    sc_dnn_cfg_nw_info_t *nw_info[MAX_NUM_OF_NETWORKS];
} sc_dnn_config_t;

int dnn_spi_boot();
int dnn_flash_boot();
int dnn_flash_update();

const sc_dnn_nw_info_t *get_dnn_nw_info(void);
uint8_t get_dnn_num_of_networks(void);

#endif