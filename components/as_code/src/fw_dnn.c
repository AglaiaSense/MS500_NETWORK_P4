/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#include <stdio.h>
# include <string.h>
#include "fw_dnn.h"
#include "imx501_macro.h"

extern int FlashBootKick(size_t trnsCmd, uint8_t imgType, size_t flashAddr);
extern int FW_SpiBoot_ProcNW(const char *fileName);
extern int FW_FlashUpdate_ProcNW(const char *fileName, size_t flashAddr);

static sc_option_dnn_type_t s_dnn_nw_type = E_DNN_TYPE_CUSTOM;

static sc_dnn_nw_info_t s_nw_info_list[MAX_NUM_OF_NETWORKS];

static size_t s_dnn_nw_id;
static uint8_t s_num_of_networks = 0;

// static const char *IMAGE_FILE_NETWK_CSTM = "/spiffs/dnn/network.fpk";
static const char *IMAGE_FILE_NETWK_CSTM = "/download/dnn/network.fpk";

static const char *CONFIG_FILE_NETWK_CSTM = "/download/dnn/network_info.txt";

const uint32_t kDefaultFlashAddrNetWk  = 0x00100000;

/**
 * @brief set_nw_info_from_file
 * @param p_nw_info
 * @return non-0 if failed for some reason, 0 otherwise
 */
static int set_nw_info_from_file(const char *dnn_config_file)
{

    FILE *fp;
    int res;
    char getstr[256] = {0};
    char str_value[256] = {0};
    size_t get_value;
    char filename1st[128];     /* CodeSonar check */
    char filename2nd[128];     /* CodeSonar check */

    //printf("[DNN_LOADER] set nw info from file \n");

    if(dnn_config_file == NULL) {
        printf("[DNN_LOADER][ERROR] dnn config file is NULL \n");
        return -1;
    }
    snprintf(filename1st, sizeof(filename1st), "%s", dnn_config_file);
    fp = fopen(filename1st, "rb");
    if (fp == NULL) {
        printf("[DNN_LOADER][ERROR] file open error %s \n", filename1st);
        return -1;
    }

    uint16_t dnnHeaderSize = 0;

    while ((res = fscanf(fp, " %[^=]%*c%s", getstr, str_value)) != EOF) {
        get_value = atoi(str_value);

        if (strcmp(getstr, "networkID") == 0) {
            /* check Network ID(decimal) */
            if (get_value > SC_DNN_MAX_NETWORK_ID_DEC) {
                printf("[DNN_LOADER][ERROR] Invalid Network ID(decimal) %d\n", get_value);
                fclose(fp);
                return -1;
            }

            /* convert string to BCD */
            s_dnn_nw_id = 0;
            for (int loop_cnt = 0; loop_cnt < 6; loop_cnt++)
            {
                s_dnn_nw_id |= (str_value[loop_cnt] - '0') << (20 - (loop_cnt * 4));
            }
        }
        else if (strcmp(getstr, "apParamSize") == 0) {
            dnnHeaderSize = 12 + (((get_value + 15) / 16) * 16);
        }
        else if (strcmp(getstr,"networkNum") == 0) {
            s_num_of_networks = (uint8_t) get_value;
        }
        else{
            /*do-nothing*/
        }
    }

    fclose(fp);

    if (s_dnn_nw_id > SC_DNN_MAX_NETWORK_ID){
        printf("[DNN_LOADER][ERROR] Invalid Network ID(BCD) 0x%08X\n", s_dnn_nw_id);
        return -1;
    }

    if (dnnHeaderSize > MAX_DNN_HEADER_SIZE){
        printf("[DNN_LOADER][ERROR] Invalid DNN Header Size %d\n", dnnHeaderSize);
        return -1;
    }
    
    if ((s_num_of_networks == 0) || (s_num_of_networks > MAX_NUM_OF_NETWORKS)) {
        printf("[DNN_LOADER][ERROR] Invalid num of networks %d\n", s_num_of_networks);
        return -1;
    }

    /* get output tensor size */
    snprintf(filename2nd, sizeof(filename2nd), "%s", dnn_config_file);
    fp = fopen(filename2nd, "rb");
    if (fp == NULL) {
        printf("[DNN_LOADER][ERROR] File open error for %s \n", filename2nd);
        return -1;
    }

    // Get parameters for each DNN
    uint8_t nwOrdinal = 0;
    while ((nwOrdinal < s_num_of_networks) && ((res = fscanf(fp, " %[^=]%*c%s", getstr, str_value)) != EOF)) {
        get_value = atoi(str_value);

        if((strcmp(getstr, "networkOrdinal") == 0) && (get_value == nwOrdinal)) {
            s_nw_info_list[nwOrdinal].dnnHeaderSize = dnnHeaderSize;

            while ((res = fscanf(fp, " %[^=]%*c%s", getstr, str_value)) != EOF) {
                get_value = strtol(str_value, NULL, 0);

                if(strcmp(getstr, "inputTensorWidth") == 0) {
                    s_nw_info_list[nwOrdinal].inputTensorWidth = (uint16_t) get_value;
                }
                else if(strcmp(getstr, "inputTensorHeight") == 0) {
                    s_nw_info_list[nwOrdinal].inputTensorHeight = (uint16_t) get_value;
                }
                else if(strcmp(getstr, "inputTensorFormat") == 0) {
                    if (strcmp(str_value, "RGB") == 0) {
                        s_nw_info_list[nwOrdinal].inputTensorFormat = DNN_INPUT_FORMAT_RGB;
                    }
                    else if (strcmp(str_value, "BGR") == 0) {
                        s_nw_info_list[nwOrdinal].inputTensorFormat = DNN_INPUT_FORMAT_BGR;
                    }
                    else if (strcmp(str_value, "Y") == 0) {
                        s_nw_info_list[nwOrdinal].inputTensorFormat = DNN_INPUT_FORMAT_Y;
                    }
                    else if (strcmp(str_value, "BayerRGB") == 0) {
                        s_nw_info_list[nwOrdinal].inputTensorFormat = DNN_INPUT_FORMAT_BAYER_RGB;
                    }
                    else {
                        printf("[DNN_LOADER][WARNING] invalid InputTensor format(%s), corrected to RGB\n", str_value);
                        s_nw_info_list[nwOrdinal].inputTensorFormat = DNN_INPUT_FORMAT_RGB;
                    }
                }
                else if(strcmp(getstr, "inputTensorNorm_K00") == 0) {
                    s_nw_info_list[nwOrdinal].NormK00 = (uint16_t)(get_value & 0x0FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K02") == 0) {
                    s_nw_info_list[nwOrdinal].NormK02 = (uint16_t)(get_value & 0x0FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K03") == 0) {
                    s_nw_info_list[nwOrdinal].NormK03 = (uint16_t)(get_value & 0x1FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K11") == 0) {
                    s_nw_info_list[nwOrdinal].NormK11 = (uint16_t)(get_value & 0x0FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K13") == 0) {
                    s_nw_info_list[nwOrdinal].NormK13 = (uint16_t)(get_value & 0x1FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K20") == 0) {
                    s_nw_info_list[nwOrdinal].NormK20 = (uint16_t)(get_value & 0x0FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K22") == 0) {
                    s_nw_info_list[nwOrdinal].NormK22 = (uint16_t)(get_value & 0x0FFF);
                }
                else if(strcmp(getstr, "inputTensorNorm_K23") == 0) {
                    s_nw_info_list[nwOrdinal].NormK23 = (uint16_t)(get_value & 0x1FFF);
                }
                else if(strcmp(getstr, "yClip") == 0) {
                    s_nw_info_list[nwOrdinal].yClip = (size_t)get_value;
                }
                else if(strcmp(getstr, "cbClip") == 0) {
                    s_nw_info_list[nwOrdinal].cbClip = (size_t)get_value;
                }
                else if(strcmp(getstr, "crClip") == 0) {
                    s_nw_info_list[nwOrdinal].crClip = (size_t)get_value;
                }
                else if(strcmp(getstr, "inputNorm_CH0") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_R].add = (uint16_t)(get_value & 0x01FF);
                }
                else if(strcmp(getstr, "inputNormShift_CH0") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_R].shift = (uint8_t)(get_value & 0x01);
                }
                else if(strcmp(getstr, "inputNormClip_CH0") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_R].clipMax = (uint16_t)((get_value >> 16) & 0x01FF);
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_R].clipMin = (uint16_t)((get_value >> 0)  & 0x01FF);
                }
                else if(strcmp(getstr, "inputNorm_CH1") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GR].add = (uint16_t)(get_value & 0x01FF);
                }
                else if(strcmp(getstr, "inputNormShift_CH1") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GR].shift = (uint8_t)(get_value & 0x01);
                }
                else if(strcmp(getstr, "inputNormClip_CH1") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GR].clipMax = (uint16_t)((get_value >> 16) & 0x01FF);
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GR].clipMin = (uint16_t)((get_value >> 0)  & 0x01FF);
                }
                else if(strcmp(getstr, "inputNorm_CH2") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GB].add = (uint16_t)(get_value & 0x01FF);
                }
                else if(strcmp(getstr, "inputNormShift_CH2") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GB].shift = (uint8_t)(get_value & 0x01);
                }
                else if(strcmp(getstr, "inputNormClip_CH2") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GB].clipMax = (uint16_t)((get_value >> 16) & 0x01FF);
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_GB].clipMin = (uint16_t)((get_value >> 0)  & 0x01FF);
                }
                else if(strcmp(getstr, "inputNorm_CH3") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_B].add = (uint16_t)(get_value & 0x01FF);
                }
                else if(strcmp(getstr, "inputNormShift_CH3") == 0) {    
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_B].shift = (uint8_t)(get_value & 0x01);
                }
                else if(strcmp(getstr, "inputNormClip_CH3") == 0) {
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_B].clipMax = (uint16_t)((get_value >> 16) & 0x01FF);
                    s_nw_info_list[nwOrdinal].rgbNorm[BAYER_CH_B].clipMin = (uint16_t)((get_value >> 0)  & 0x01FF);
                }
                else if(strcmp(getstr, "inputTensorNorm_YGain") == 0) {
                    s_nw_info_list[nwOrdinal].yGgain = (uint8_t)get_value;
                }
                else if(strcmp(getstr, "inputTensorNorm_YAdd") == 0) {
                    s_nw_info_list[nwOrdinal].yAdd = (uint16_t)get_value;
                }
                else if(strcmp(getstr, "outputTensorNum") == 0) {
                    s_nw_info_list[nwOrdinal].outputTensorNum = (uint8_t) get_value;
                    break;
                }
            }

            if (s_nw_info_list[nwOrdinal].outputTensorNum == 0) {
                printf("[DNN_LOADER][ERROR] OUTPUTTENSOR_NUM for NetworkOrdinal %d is set to 0 in config.txt\n", nwOrdinal);
                fclose(fp);
                return -1;
            }

            if (s_nw_info_list[nwOrdinal].outputTensorNum >= MAX_OUTPUT_TENSOR_NUM) {
                printf("[DNN_LOADER][ERROR] OUTPUTTENSOR_NUM  for NetworkOrdinal %d in the config.txt is set to %d\n", nwOrdinal, s_nw_info_list[nwOrdinal].outputTensorNum);
                fclose(fp);
                return -1;
            }

            /*Fix CodeSonar Check - handle potential integer overflow*/
            if ((UINT32_MAX / s_nw_info_list[nwOrdinal].outputTensorNum) < sizeof(sc_output_tensor_size_info_t)) {
                printf("[DNN_LOADER][ERROR] Error output_tensor_num_arr_size overflows, num=%d, structureSize=%u\n",
                              s_nw_info_list[nwOrdinal].outputTensorNum,
                              sizeof(sc_output_tensor_size_info_t));
                fclose(fp);
                return -1;
            }

            size_t output_tensor_num_arr_size = s_nw_info_list[nwOrdinal].outputTensorNum * sizeof(sc_output_tensor_size_info_t);
            s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo = (sc_output_tensor_size_info_t*)malloc(output_tensor_num_arr_size);
            if (s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo == NULL) {
                printf("[DNN_LOADER][ERROR] p_outputTensorSizeInfo is NULL\n");
                fclose(fp);
                return -1;
            }

            /* get output tensor dimension size */
            size_t size_cnt = 0;
            while ((size_cnt < s_nw_info_list[nwOrdinal].outputTensorNum) && ((res = fscanf(fp, " %[^=]%*c%s", getstr, str_value)) != EOF)) {
                get_value = atoi(str_value);

                if (strncmp(getstr, "outputTensorDimSize", 19) == 0) {
                    s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo[size_cnt++].dimSize = (size_t) get_value;
                }
            }
            if (size_cnt < s_nw_info_list[nwOrdinal].outputTensorNum) {
                printf("[DNN_LOADER][ERROR] Insufficient outputTensorDimSize* settings in network_info.txt\n");
                fclose(fp);
                free(s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo);
                return -1;
            }

            /* get output tensor padding size */
            size_cnt = 0;
            while ((size_cnt < s_nw_info_list[nwOrdinal].outputTensorNum) && ((res = fscanf(fp, " %[^=]%*c%s", getstr, str_value)) != EOF)) {
                get_value = atoi(str_value);

                if (strncmp(getstr, "outputTensorPadding", 19) == 0) {
                    s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo[size_cnt++].padding = (uint8_t) get_value;
                }
            }
            if (size_cnt < s_nw_info_list[nwOrdinal].outputTensorNum) {
                printf("[DNN_LOADER][ERROR] Insufficient outputTensorPadding* settings in network_info.txt\n");
                fclose(fp);
                free(s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo);
                return -1;
            }

            /* get output tensor byte per element */
            size_cnt = 0;
            while ((size_cnt < s_nw_info_list[nwOrdinal].outputTensorNum) && ((res = fscanf(fp, " %[^=]%*c%s", getstr, str_value)) != EOF)) {
                get_value = atoi(str_value);

                if (strncmp(getstr, "outputTensorBytesPerElement", 27) == 0) {
                    s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo[size_cnt++].bytePerElement = (uint8_t) get_value;
                }
            }
            if (size_cnt < s_nw_info_list[nwOrdinal].outputTensorNum) {
                printf("[DNN_LOADER][ERROR] Insufficient outputTensorBytesPerElement* settings in network_info.txt\n");
                fclose(fp);
                free(s_nw_info_list[nwOrdinal].p_outputTensorSizeInfo);
                return -1;
            }

            nwOrdinal++;
        }

    }

    fclose(fp);
    
    if (nwOrdinal < s_num_of_networks) {
        printf("[DNN_LOADER][ERROR] Insufficient network_num settings in network_info.txt\n");
        for(int i = 0; i < nwOrdinal; i++) {
            if (s_nw_info_list[i].p_outputTensorSizeInfo != NULL) {
                free(s_nw_info_list[i].p_outputTensorSizeInfo);
            }
        }
        return -1;
    }

    return 0;
}


/* @brief dnn_init
 * @param
 * @return
*/
static int dnn_init(const sc_dnn_config_t *dnn_config)
{
    if (s_dnn_nw_type >= E_DNN_TYPE_MAX) {
        printf("[DNN_LOADER][ERROR] Invalid user network!!\n");
        return -1;
    }

    for(int i = 0; i < MAX_NUM_OF_NETWORKS; i++) {
        /*free-up the memory if already allocated*/
        if (s_nw_info_list[i].p_outputTensorSizeInfo != NULL) {
            free(s_nw_info_list[i].p_outputTensorSizeInfo);
        }

        memset(&s_nw_info_list[i], 0, sizeof(sc_dnn_nw_info_t));
    }

    if (set_nw_info_from_file(CONFIG_FILE_NETWK_CSTM) != 0) {
        printf("[DNN_LOADER] set_nw_info_from_file failed !!\n");
        return -1;
    }

    /* get gain value */
    size_t tmp_gain = 0;
    tmp_gain = 0x20;  //reg_es2.txt //LEV_PL_GAIN_VALUE,                  0xD600, 1,  0x20

    // regsetting_get_gain( &tmp_gain);

    /* set network information */
    for(int k = 0; k < s_num_of_networks; k++) {
        s_nw_info_list[k].inputTensorWidthStride = (((s_nw_info_list[k].inputTensorWidth + 31) / 32) * 32);
        s_nw_info_list[k].inputTensorHeightStride = ((s_nw_info_list[k].inputTensorHeight % 2) == 0) ? s_nw_info_list[k].inputTensorHeight : s_nw_info_list[k].inputTensorHeight + 1 ;

        if ((s_nw_info_list[k].inputTensorFormat != DNN_INPUT_FORMAT_Y)) {
            if (s_nw_info_list[k].inputTensorFormat != DNN_INPUT_FORMAT_BAYER_RGB) {
                s_nw_info_list[k].inputTensorSize = (s_nw_info_list[k].inputTensorWidthStride * s_nw_info_list[k].inputTensorHeightStride * 3);
            }
            else {
                s_nw_info_list[k].inputTensorSize = (s_nw_info_list[k].inputTensorWidthStride * s_nw_info_list[k].inputTensorHeightStride * 4);
            }
            s_nw_info_list[k].yGgain = tmp_gain;
        }
        else { /* p_cfg_param->binning_opt == E_BINNING_OPT_MONO) || (s_nw_info_list[k].inputTensorFormat == DNN_INPUT_FORMAT_Y) */
            s_nw_info_list[k].inputTensorSize = (s_nw_info_list[k].inputTensorWidthStride * s_nw_info_list[k].inputTensorHeightStride * 1);
        }

    }

    return 0;
}

/**
 * @brief dnn_flash_update
 */
int dnn_flash_update()
{
    int ret;
    sc_dnn_config_t *p_dnn_config = NULL;

    sc_dnn_config_t *p_dnn_cfg = NULL;

    if (p_dnn_config == NULL) {
        p_dnn_cfg = (sc_dnn_config_t *) malloc(sizeof(sc_dnn_config_t));
        if (p_dnn_cfg == NULL) {
            printf("[DNN_LOADER] malloc p_dnn_cfg failed\n");
            return -1;
        }

        memset(p_dnn_cfg, 0, sizeof(sc_dnn_config_t));
        p_dnn_cfg->dnn_network_type = E_DNN_TYPE_CUSTOM;

        p_dnn_config = p_dnn_cfg;
    }

    printf("[DNN_LOAD] dnn_load Start\n");

    if (dnn_init(p_dnn_config) != 0) {
        printf("[DNN_LOADER] dnn_init failed\n");
        return -1;
    }
    free(p_dnn_config);

    ret = FW_SpiBoot_ProcNW(IMAGE_FILE_NETWK_CSTM);
    if (ret != 0) {
        printf("[DNN_LOADER] FW_SpiBoot_ProcNW failed\n");
        return -1;
    }

    printf("[DNN_LOAD] IMX500 load_dnn done\n");

    printf("\nFlashing DNN image ...\n");
    ret = FW_FlashUpdate_ProcNW(IMAGE_FILE_NETWK_CSTM, kDefaultFlashAddrNetWk);
    if (ret != 0) {
        printf("[DNN_LOADER] FW_FlashUpdate_ProcNW failed\n");
        return -1;
    }

    return ret;
}

/**
 * @brief dnn_spi_boot
 */
int dnn_spi_boot()
{
    int ret;
    sc_dnn_config_t *p_dnn_config = NULL;

    sc_dnn_config_t *p_dnn_cfg = NULL;

    if (p_dnn_config == NULL) {
        p_dnn_cfg = (sc_dnn_config_t *) malloc(sizeof(sc_dnn_config_t));
        if (p_dnn_cfg == NULL) {
            printf("[DNN_LOADER] malloc p_dnn_cfg failed\n");
            return -1;
        }

        memset(p_dnn_cfg, 0, sizeof(sc_dnn_config_t));
        p_dnn_cfg->dnn_network_type = E_DNN_TYPE_CUSTOM;

        p_dnn_config = p_dnn_cfg;
    }

    printf("[DNN_LOAD] Flash Boot DNN\n");

    if (dnn_init(p_dnn_config) != 0) {
        printf("[DNN_LOADER] dnn_init failed\n");
        return -1;
    }
    free(p_dnn_config);

    ret = FW_SpiBoot_ProcNW(IMAGE_FILE_NETWK_CSTM);
    if (ret != 0) {
        printf("[DNN_LOADER] FW_SpiBoot_ProcNW failed\n");
        return -1;
    }

    printf("[DNN_LOAD] IMX500 load_dnn done\n");

    return ret;
}

/**
 * @brief dnn_flash_boot
 */
int dnn_flash_boot()
{
    int ret;
    sc_dnn_config_t *p_dnn_config = NULL;

    sc_dnn_config_t *p_dnn_cfg = NULL;

    if (p_dnn_config == NULL) {
        p_dnn_cfg = (sc_dnn_config_t *) malloc(sizeof(sc_dnn_config_t));
        if (p_dnn_cfg == NULL) {
            printf("[DNN_LOADER] malloc p_dnn_cfg failed\n");
            return -1;
        }
        //  printf("[DNN_LOADER] Allocated p_dnn_cfg, size: %zu bytes\n", sizeof(sc_dnn_config_t));
        memset(p_dnn_cfg, 0, sizeof(sc_dnn_config_t));
        p_dnn_cfg->dnn_network_type = E_DNN_TYPE_CUSTOM;

        p_dnn_config = p_dnn_cfg;
    }

    printf("[DNN_LOAD] dnn_load Start\n");

    if (dnn_init(p_dnn_config) != 0) {
        printf("[DNN_LOADER] dnn_init failed\n");
        return -1;
    }
    free(p_dnn_config);

    /* dnn */
    ret = FlashBootKick(0x02, 0x02, 0x00100000);
    if (ret != 0) {
        printf("[IMX500LIB][CTRL] Failed open(netwk load error) : ret=0x%08X\n", ret);
        return ret;
    }

    printf("[DNN_LOAD] IMX500 load_dnn done\n");

    return ret;
}

/**
 * @brief get_dnn_nw_info
 */
const sc_dnn_nw_info_t *get_dnn_nw_info(void)
{
    return (const sc_dnn_nw_info_t *) &s_nw_info_list[0];
}

/**
 * @brief get_dnn_num_of_networks
 */
uint8_t get_dnn_num_of_networks(void)
{
    return s_num_of_networks;
}