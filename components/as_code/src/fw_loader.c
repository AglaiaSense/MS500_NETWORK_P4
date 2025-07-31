/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "fw_loader.h"
#include "imx501.h"
#include "fw_dnn.h"
#include <sys/time.h>

#include "imx501_macro.h"


const uint32_t kDefaultFlashAddrLoader = 0x00000000;
const uint32_t kDefaultFlashAddrMainFw = 0x00020000;

static const char *TAG = "FW LOADER";

/**
 * @brief conv_reg_signed()
 */
static int conv_reg_signed(size_t reg_val, uint8_t signed_bit, size_t reg_mask)
{
    int conv_val;

    if (((reg_val >> signed_bit) & 1) == 0) {
        conv_val = reg_val;
    }
    else {
        conv_val = -((~reg_val + 1) & reg_mask);
    }
    return conv_val;
}


/**
 * @brief get_file_size
 */
size_t get_file_size(FILE* fp) 
{
    size_t filesize = 0;

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);
    
    return filesize;
}

/**
 * @brief image_file_read
 */
static int image_file_read(FILE* fp, uint8_t* p_dst, size_t* p_size, size_t buf_size)
{
    if (fp == NULL) {
        return -1;
    }

    *p_size = fread(p_dst, 1, buf_size, fp);

    return 0;
}

/**
 * @brief copy_byteswap_buffer
 */
static void copy_byteswap_buffer(uint8_t* data, size_t size) {
    size_t pos;
    uint8_t tmp[4];

    pos = 0;

    /*CodeSonar Fix*/
    if(size % 4 != 0 || size == 0) {
        printf("[%s] : [ERROR] Invalid size [%d] \n", __func__, size);
        return;
    }

    while ( pos < size) {
        tmp[0] = data[0 + pos];
        tmp[1] = data[1 + pos];
        tmp[2] = data[2 + pos];
        tmp[3] = data[3 + pos];

        data[3 + pos] = tmp[0];
        data[2 + pos] = tmp[1];
        data[1 + pos] = tmp[2];
        data[0 + pos] = tmp[3];
        pos = pos + 4;
    }
}

/**
 * @brief FW_TransferSpi
 */
static int FW_TransferSpi(uint8_t *pTransbuf, size_t transSize)
{
    int ret = 0;

    ret = spi_master_dev_write(pTransbuf, transSize);

    return ret;
}

/**
 * @brief FW_SpiBoot_ProcFW_bak
 */
static int FW_SpiBoot_ProcFW_bak(size_t imageType, size_t transCmd, 
                                    uint8_t *pTransbuf, size_t size)
{
    int ret;
    size_t regVal;
    size_t timer_cnt = 0;
    uint8_t *pCurBuf = NULL;
    size_t residualSize = 0;
    size_t transSize = 0;
    uint8_t refStsCnt = 0;

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)(regVal);

    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_LOAD_MODE, IMX500SF_REG_FROMAP_DD_LOAD_MODE, IMX500SF_REG_SIZE_DD_LOAD_MODE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_IMAGE_TYPE, imageType, IMX500SF_REG_SIZE_DD_IMAGE_TYPE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_DIV_NUM, 0, IMX500SF_REG_SIZE_DD_DOWNLOAD_DIV_NUM);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_FILE_SIZE, size, IMX500SF_REG_SIZE_DD_DOWNLOAD_FILE_SIZE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_ST_TRANS_CMD, transCmd, IMX500SF_REG_SIZE_DD_ST_TRANS_CMD);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, IMX500SF_REG_TRANS_CMD_DD_CMD_INT, IMX500SF_REG_SIZE_DD_CMD_INT);

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }

        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* update ref status count */
    refStsCnt = (uint8_t)(regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_READY) {
        printf("[IMX500LIB][IMAGE_LOADER] boot error(reply status NG): imageType=%d, reply status=0x%02X\n", imageType, regVal);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);

    /* SPI Transfer */
    residualSize = size;
    pCurBuf = pTransbuf;

    while (1) {
        if (residualSize == 0) {
            break;
        }
        if (residualSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
            transSize = IMX500SF_SPI_ONE_TRANS_SIZE;
        } else {
            transSize = residualSize;
        }               
        ret = FW_TransferSpi(pCurBuf, transSize);
        if (ret != 0) {
            return ret;
        }
        if (transSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
            pCurBuf = pCurBuf + IMX500SF_SPI_ONE_TRANS_SIZE;
            residualSize = residualSize - IMX500SF_SPI_ONE_TRANS_SIZE;
        } else {
            residualSize = 0;
        }               
    }

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_DONE) {
        printf("[IMX500LIB][IMAGE_LOADER] boot error(reply status NG): imageType=%d, reply status=0x%02X\n", imageType, regVal);
        return -1;
    }

    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);
    printf("[IMX500LIB][IMAGE_LOADER] downloading successfully\n");
    return 0;
}

/**
 * @brief FW_SpiBoot_ProcFW
 */
static int FW_SpiBoot_ProcFW(const char *fileName, size_t imageType, size_t transCmd)
{
    int ret;
    FILE *image_fp;
    size_t regVal;
    size_t timer_cnt = 0;
    uint8_t *pCurBuf = NULL;
    size_t residualSize = 0;
    size_t transSize = 0;
    uint8_t refStsCnt = 0;
    uint8_t *pTransbuf = NULL;
    size_t size = 0;

    image_fp = fopen(fileName, "rb");
    if(image_fp == NULL) {
        printf("[FW_LOAD] Open file %s error\n", fileName);
        return -1;
    }

    size = get_file_size(image_fp);
    pTransbuf = (uint8_t *)malloc(IMAGE_FILE_MEMORY_SIZE);
    if (pTransbuf == NULL) {
        printf("[FW_LOAD] Malloc memory error\n");
        fclose(image_fp);
        return -1;
    }

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)(regVal);

    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_LOAD_MODE, IMX500SF_REG_FROMAP_DD_LOAD_MODE, IMX500SF_REG_SIZE_DD_LOAD_MODE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_IMAGE_TYPE, imageType, IMX500SF_REG_SIZE_DD_IMAGE_TYPE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_DIV_NUM, 0, IMX500SF_REG_SIZE_DD_DOWNLOAD_DIV_NUM);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_FILE_SIZE, size, IMX500SF_REG_SIZE_DD_DOWNLOAD_FILE_SIZE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_ST_TRANS_CMD, transCmd, IMX500SF_REG_SIZE_DD_ST_TRANS_CMD);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, IMX500SF_REG_TRANS_CMD_DD_CMD_INT, IMX500SF_REG_SIZE_DD_CMD_INT);

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }

        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* update ref status count */
    refStsCnt = (uint8_t)(regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_READY) {
        printf("[IMX500LIB][IMAGE_LOADER] boot error(reply status NG): imageType=%d, reply status=0x%02X\n", imageType, regVal);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);

    //struct timeval tv_now;
    //int64_t time_us;

    /* SPI Transfer */
    while (1) {
        //gettimeofday(&tv_now, NULL);
        //time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        //printf("before read file %lld\n", time_us);
        ret = image_file_read(image_fp, pTransbuf, &residualSize, IMAGE_FILE_MEMORY_SIZE);
        if (residualSize == 0)
            break;

        //gettimeofday(&tv_now, NULL);
        //time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        //printf("after read file %lld\n", time_us);

        copy_byteswap_buffer(pTransbuf, residualSize); 
        pCurBuf = pTransbuf; 

        while (1) {
            if (residualSize == 0) {
                break;
            }
            if (residualSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                transSize = IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                transSize = residualSize;
            }        
            ret = FW_TransferSpi(pCurBuf, transSize);
            if (ret != 0) {
                return ret;
            }
            if (transSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                pCurBuf = pCurBuf + IMX500SF_SPI_ONE_TRANS_SIZE;
                residualSize = residualSize - IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                residualSize = 0;
            }               
        } 
    }

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_DONE) {
        printf("[IMX500LIB][IMAGE_LOADER] boot error(reply status NG): imageType=%d, reply status=0x%02X\n", imageType, regVal);
        return -1;
    }

    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);
    printf("[IMX500LIB][IMAGE_LOADER] Downloading successfully\n");

    fclose(image_fp);
    free(pTransbuf);

    return 0;
}

/**
 * @brief FW_FlashErase
 */
static int FW_FlashErase(void)
{
    int ret;
    size_t regVal;
    size_t timer_cnt = 0;
    uint8_t refStsCnt = 0;

    printf("[IMX500LIB][ERASE] Erase...\n");

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)(regVal);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, 0x02, IMX500SF_REG_SIZE_DD_CMD_INT);

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }

        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][ERASE] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][ERASE] DD_REF_STS_REG  %02X\n", regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != 0x21) {
        printf("[IMX500LIB][ERASE] boot error(reply status NG): reply status=0x%02X\n", regVal);
        return -1;
    }
    printf("[IMX500LIB][ERASE] DD_CMD_REPLY_STS  %02X\n", regVal);

    return 0;
}

/**
 * @brief FW_ChangeStandbyWtoStandbyWo
 */
static int FW_ChangeStandbyWtoStandbyWo(void) 
{
    size_t regVal;
    size_t timer_cnt = 0;
    uint8_t refStsCnt = 0;

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)regVal;

    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG=0x%02X\n", regVal);

    I2C_ACCESS_WRITE(0xD000, 0x03, 1);
    I2C_ACCESS_WRITE(0x3080, 0x00, 1);

    /* wait change status */ 
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal,IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)regVal != refStsCnt) {
            break;
        }
        //SENSOR_USE_TIMER(10000000);  /* wait 10ms */
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_DONE) {
        printf("[IMX500LIB][IMAGE_LOADER] Standby WNet -> WoNet error(reply status NG): reply status=0x%02X\n", regVal);
        return -1;
    }
    return 0;
}

/**
 * @brief FW_FlashUpdate_ProcFW
 */
static int FW_FlashUpdate_ProcFW(const char *fileName, size_t imageType, size_t flashAddr)
{
    int ret;
    FILE *image_fp;
    size_t regVal;
    size_t timer_cnt = 0;
    uint8_t *pCurBuf = NULL;
    size_t residualSize = 0;
    size_t transSize = 0;
    uint8_t refStsCnt = 0;
    uint8_t *pTransbuf = NULL;
    size_t size = 0;

    image_fp = fopen(fileName, "rb");
    if(image_fp == NULL) {
        printf("[FW_LOAD] Open file %s error\n", fileName);
        return -1;
    }

    size = get_file_size(image_fp);
    pTransbuf = (uint8_t *)malloc(IMAGE_FILE_MEMORY_SIZE);
    if (pTransbuf == NULL) {
        printf("[FW_LOAD] Malloc memory error\n");
        fclose(image_fp);
        return -1;
    }

#if 1
    printf( "[IMX500LIB][IMAGE_LOADER] Imx500SF_FlashUpdate_MainFW flashAddr 0x%x\n",flashAddr);
    
    /* Current Sate Check */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_SYS_STATE, &regVal, IMX500SF_REG_SIZE_DD_SYS_STATE);
    printf("[IMX500LIB][IMAGE_LOADER] Current State :%d\n",regVal);

    if (regVal == IMX500SF_REG_DD_SYS_STATE_STANDBY_WNET) {
        // State Change
        ret = FW_ChangeStandbyWtoStandbyWo();
        if (ret != 0) {
            printf("[IMX500LIB][IMAGE_LOADER]  Imx500SF_FlashUpdate_MainFW ERROR \n");  
            return ret;
        }
    } else if (regVal != IMX500SF_REG_DD_SYS_STATE_STANDBY_WONET) {
        printf("[IMX500LIB][IMAGE_LOADER]  Imx500SF_FlashUpdate_MainFW Invalid State %x\n",regVal);
        return -1;
    } else {
        /* Do Nothing */
    }
#endif

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)regVal;

    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_LOAD_MODE, IMX500SF_REG_FROMAP_DD_LOAD_MODE, IMX500SF_REG_SIZE_DD_LOAD_MODE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_IMAGE_TYPE, imageType, IMX500SF_REG_SIZE_DD_IMAGE_TYPE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_DIV_NUM, 0, IMX500SF_REG_SIZE_DD_DOWNLOAD_DIV_NUM);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_FILE_SIZE, size, IMX500SF_REG_SIZE_DD_DOWNLOAD_FILE_SIZE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_FLASH_ADDR, flashAddr, IMX500SF_REG_SIZE_DD_FLASH_ADDR);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_UPDATE_CMD, IMX500SF_REG_DD_UPDATE_CMD_UPDATE_FLASH, IMX500SF_REG_SIZE_DD_UPDATE_CMD);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, IMX500SF_REG_UPDATE_CMD_DD_CMD_INT, IMX500SF_REG_SIZE_DD_CMD_INT);

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)regVal != refStsCnt) {
            break;
        }

        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* update ref status count */
    refStsCnt = (uint8_t)(regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_UPDATE_READY) {
        printf("[IMX500LIB][IMAGE_LOADER] boot error(reply status NG): imageType=%d, reply status=0x%02X\n", imageType, regVal);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);

    /* SPI Transfer */
    while (1) {
        ret = image_file_read(image_fp, pTransbuf, &residualSize, IMAGE_FILE_MEMORY_SIZE);
        if (residualSize == 0)
            break;

        copy_byteswap_buffer(pTransbuf, residualSize); 
        pCurBuf = pTransbuf; 

        while (1) {
            if (residualSize == 0) {
                break;
            }
            if (residualSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                transSize = IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                transSize = residualSize;
            }        
            ret = FW_TransferSpi(pCurBuf, transSize);
            if (ret != 0) {
                return ret;
            }
            if (transSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                pCurBuf = pCurBuf + IMX500SF_SPI_ONE_TRANS_SIZE;
                residualSize = residualSize - IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                residualSize = 0;
            }               
        } 
    }

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            return -1; 
        }
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_UPDATE_DONE) {
        printf("[IMX500LIB][IMAGE_LOADER] boot error(reply status NG): imageType=%d, reply status=0x%02X\n", imageType, regVal);
        return -1;
    }

    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);
    printf("[IMX500LIB][IMAGE_LOADER] Downloading successfully\n");

    fclose(image_fp);
    free(pTransbuf);

    return 0;
}

/**
 * @brief fw_spi_boot
 */
int fw_spi_boot(void)
{
    int ret;
    size_t exe_freq, flash_type;

    /* get sensor inck frequency and flash type */
    // regsetting_get_freq_type(&exe_freq, &flash_type);

    /* set sensor inck */
    ret = imx501_set_inck(1);
    if (ret != 0) {
        printf("imx501 set inck error\n");
        return -1;
    }

    /* download loader firmware*/
    ret = FW_SpiBoot_ProcFW(IMAGE_FILE_LOADER, IMX500SF_REG_LOADER_DD_IMAGE_TYPE, IMX500SF_REG_LOADER_LOAD_ST_TRANS_CMD);
    if (ret != 0) {
        printf("[IMX500LIB][IMAGE_LOAD] FW_SpiBoot_LoaderFW ERROR !!! \n");
    }   

    /* download main firmware */
    ret = FW_SpiBoot_ProcFW(IMAGE_FILE_MAINFW, IMX500SF_REG_MAIN_DD_IMAGE_TYPE, IMX500SF_REG_MAINFW_LOAD_ST_TRANS_CMD);
    if (ret != 0) {
        printf("[IMX500LIB][IMAGE_LOAD] FW_SpiBoot_MainFW ERROR !!! \n");
    }   

    /* write sensor registers */
    regsetting_set_reg_val();

    return 0;
}

/**
 * @brief FW_SpiBoot_ProcNW
 */
int FW_SpiBoot_ProcNW(const char *fileName)
{
    int ret = 0;
    FILE *image_fp;
    size_t size;
    size_t regVal;
    uint8_t  divNum = 0;
    size_t timer_cnt = 0;
    uint8_t *pTransbuf = NULL;
    uint8_t *pCurBuf = NULL;
    size_t residualSize = 0;
    size_t transSize = 0;
    uint8_t refStsCnt = 0;
    size_t spiTransactionSize = 0;
    uint8_t i= 0;

    image_fp = fopen(fileName, "rb");
    if(image_fp == NULL) {
        printf("[FW_LOAD] Open file %s error\n", fileName);
        return -1;
    }

    size = get_file_size(image_fp);
    pTransbuf = (uint8_t *)malloc(IMX500SF_SPI_ONE_TRANS_SIZE);
    if (pTransbuf == NULL) {
        printf("[FW_LOAD] Malloc memory error\n");
        fclose(image_fp);
        return -1;
    }

    /* switch to dnn mode */
    I2C_ACCESS_WRITE(REG_ADDR_IMAGING_ONLY_MODE_FW, DEF_VAL_NOTIMAGING_ONLY_MODE_FW, REG_SIZE_IMAGING_ONLY_MODE_FW);

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)(regVal);

    /* Determining the number of divisions */
    if ((size % IMX500SF_NW_TRANS_SIZE_UNIT) == 0) {
        divNum = (size/IMX500SF_NW_TRANS_SIZE_UNIT) - 1;  
    } else {
        divNum = size/IMX500SF_NW_TRANS_SIZE_UNIT;
    }

    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_LOAD_MODE, IMX500SF_REG_FROMAP_DD_LOAD_MODE, IMX500SF_REG_SIZE_DD_LOAD_MODE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_IMAGE_TYPE, IMX500SF_REG_NW_DD_IMAGE_TYPE, IMX500SF_REG_SIZE_DD_IMAGE_TYPE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_DIV_NUM, divNum, IMX500SF_REG_SIZE_DD_DOWNLOAD_DIV_NUM);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_FILE_SIZE, size, IMX500SF_REG_SIZE_DD_DOWNLOAD_FILE_SIZE);

    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_ST_TRANS_CMD, IMX500SF_REG_WO_NW_TO_W_NW_ST_TRANS_CMD, IMX500SF_REG_SIZE_DD_ST_TRANS_CMD);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, IMX500SF_REG_TRANS_CMD_DD_CMD_INT, IMX500SF_REG_SIZE_DD_CMD_INT);

    printf("[IMX500LIB][IMAGE_LOADER] FW_SpiBoot_ProcNW refStsCnt %x divNum %x\n",refStsCnt,divNum);
    
    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            free(pTransbuf);
            return -1; 
        }
    }

    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);
    /* update ref status count */
    refStsCnt = (uint8_t)(regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_READY) {
        printf("[IMX500LIB][IMAGE_LOADER] network boot error(reply status NG): imageType=%d, reply status=0x%02X\n", IMX500SF_REG_NW_DD_IMAGE_TYPE, regVal);
        free(pTransbuf);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);
    
    residualSize = size;
    pCurBuf = pTransbuf;

    for (i = 0; i <= divNum; i++) {
        printf("[IMX500LIB][IMAGE_LOADER] %02X divid Trans  residualSize %d\n", i, residualSize);
        /* Check DOWNLOAD Status */
        while (1) {
            I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_DOWNLOAD_STS, &regVal, IMX500SF_REG_SIZE_DD_DOWNLOAD_STS);
            if (regVal == IMX500SF_REG_READY_DD_DOWNLOAD_STS) {
                break;
            }
            //printf("regVal = 0x%x\n", regVal);
        }
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_DOWNLOAD_STS, &regVal, IMX500SF_REG_SIZE_DD_DOWNLOAD_STS);
        printf("[IMX500LIB][IMAGE_LOADER] DD_DOWNLOAD_STS  %02X\n", regVal);
        
        if (residualSize >= IMX500SF_NW_TRANS_SIZE_UNIT) {
            spiTransactionSize = IMX500SF_NW_TRANS_SIZE_UNIT;
        } else {
            spiTransactionSize = residualSize;
        }

        printf("[IMX500LIB][IMAGE_LOADER] residualSize:%d   spiTransactionSize:%d\n", residualSize,spiTransactionSize);
        while (1) {
            if (spiTransactionSize == 0) {
                break;
            }
            if (spiTransactionSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                transSize = IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                transSize = spiTransactionSize;
            }
            /* reading file */
            size_t tmp_size;
            image_file_read(image_fp, pTransbuf, &tmp_size, transSize);
            copy_byteswap_buffer(pTransbuf, transSize); 
            ret = FW_TransferSpi(pCurBuf, transSize);
            if (ret != 0) {
                return ret;
            }
            if (transSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                //pCurBuf = pCurBuf + IMX500SF_SPI_ONE_TRANS_SIZE;
                spiTransactionSize = spiTransactionSize - IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                spiTransactionSize = 0;
            }               
        }
        
        /* Update Residual Size */
        if (residualSize >= IMX500SF_NW_TRANS_SIZE_UNIT) {
            residualSize = residualSize - IMX500SF_NW_TRANS_SIZE_UNIT;
        } else {
            residualSize = 0;
        }
    }
    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            free(pTransbuf);
            return -1; 
        }
    }
    //PRINTF("[IMX500LIB][IMAGE_LOADER] timer_cnt  %x\n", timer_cnt);
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_DONE) {
        printf("[IMX500LIB][IMAGE_LOADER] network boot error(reply status NG): imageType=%d, reply status=0x%02X\n", IMX500SF_REG_NW_DD_IMAGE_TYPE, regVal);
        free(pTransbuf);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);

    fclose(image_fp);
    free(pTransbuf);

    return 0;
}

/**
 * @brief FW_FlashUpdate_ProcNW
 */
int FW_FlashUpdate_ProcNW(const char *fileName, size_t flashAddr)
{
    int ret = 0;
    FILE *image_fp;
    size_t size;
    size_t regVal;
    uint8_t  divNum = 0;
    size_t timer_cnt = 0;
    uint8_t *pTransbuf = NULL;
    uint8_t *pCurBuf = NULL;
    size_t residualSize = 0;
    size_t transSize = 0;
    uint8_t refStsCnt = 0;
    size_t spiTransactionSize = 0;
    uint8_t i= 0;

    image_fp = fopen(fileName, "rb");
    if(image_fp == NULL) {
        printf("[FW_LOAD] Open file %s error\n", fileName);
        return -1;
    }

    size = get_file_size(image_fp);
    pTransbuf = (uint8_t *)malloc(IMX500SF_SPI_ONE_TRANS_SIZE);
    if (pTransbuf == NULL) {
        printf("[FW_LOAD] Malloc memory error\n");
        fclose(image_fp);
        return -1;
    }

#if 1
    printf( "[IMX500LIB][IMAGE_LOADER] FW_FlashUpdate_ProcNW flashAddr 0x%x\n",flashAddr);
    
    /* Current Sate Check */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_SYS_STATE, &regVal, IMX500SF_REG_SIZE_DD_SYS_STATE);
    printf("[IMX500LIB][IMAGE_LOADER] Current State :%d\n",regVal);

    if (regVal == IMX500SF_REG_DD_SYS_STATE_STANDBY_WNET) {
        // State Change
        ret = FW_ChangeStandbyWtoStandbyWo();
        if (ret != 0) {
            printf("[IMX500LIB][IMAGE_LOADER]  IMX500SF_FlashUpdate_ProcNW ERROR \n");  
            return ret;
        }
    } else if (regVal != IMX500SF_REG_DD_SYS_STATE_STANDBY_WONET) {
        printf("[IMX500LIB][IMAGE_LOADER]  FW_FlashUpdate_ProcNW Invalid State %x\n",regVal);
        return -1;
    } else {
        /* Do Nothing */
    }
#endif

    /* switch to dnn mode */
    I2C_ACCESS_WRITE(REG_ADDR_IMAGING_ONLY_MODE_FW, DEF_VAL_NOTIMAGING_ONLY_MODE_FW, REG_SIZE_IMAGING_ONLY_MODE_FW);

    /* current CMD_REPLY_STS_CNT */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
    refStsCnt = (uint8_t)(regVal);

    /* Determining the number of divisions */
    if ((size % IMX500SF_NW_TRANS_SIZE_UNIT) == 0) {
        divNum = (size/IMX500SF_NW_TRANS_SIZE_UNIT) - 1;  
    } else {
        divNum = size/IMX500SF_NW_TRANS_SIZE_UNIT;
    }

    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_LOAD_MODE, IMX500SF_REG_FROMAP_DD_LOAD_MODE, IMX500SF_REG_SIZE_DD_LOAD_MODE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_IMAGE_TYPE, IMX500SF_REG_NW_DD_IMAGE_TYPE, IMX500SF_REG_SIZE_DD_IMAGE_TYPE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_DIV_NUM, divNum, IMX500SF_REG_SIZE_DD_DOWNLOAD_DIV_NUM);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_DOWNLOAD_FILE_SIZE, size, IMX500SF_REG_SIZE_DD_DOWNLOAD_FILE_SIZE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_FLASH_ADDR, flashAddr, IMX500SF_REG_SIZE_DD_FLASH_ADDR);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_UPDATE_CMD, IMX500SF_REG_DD_UPDATE_CMD_UPDATE_FLASH, IMX500SF_REG_SIZE_DD_UPDATE_CMD);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, IMX500SF_REG_UPDATE_CMD_DD_CMD_INT, IMX500SF_REG_SIZE_DD_CMD_INT);

    printf("[IMX500LIB][IMAGE_LOADER] FW_SpiBoot_ProcNW refStsCnt %x divNum %x\n",refStsCnt,divNum);
    
    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            free(pTransbuf);
            return -1; 
        }
    }

    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);
    /* update ref status count */
    refStsCnt = (uint8_t)(regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_UPDATE_READY) {
        printf("[IMX500LIB][IMAGE_LOADER] network boot error(reply status NG): imageType=%d, reply status=0x%02X\n", IMX500SF_REG_NW_DD_IMAGE_TYPE, regVal);
        free(pTransbuf);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);
    
    residualSize = size;
    pCurBuf = pTransbuf;

    for (i = 0; i <= divNum; i++) {
        printf("[IMX500LIB][IMAGE_LOADER] %02X divid Trans  residualSize %d\n", i, residualSize);
        /* Check DOWNLOAD Status */
        while (1) {
            I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_DOWNLOAD_STS, &regVal, IMX500SF_REG_SIZE_DD_DOWNLOAD_STS);
            if (regVal == IMX500SF_REG_READY_DD_DOWNLOAD_STS) {
                break;
            }
            //printf("regVal = 0x%x\n", regVal);
        }
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_DOWNLOAD_STS, &regVal, IMX500SF_REG_SIZE_DD_DOWNLOAD_STS);
        printf("[IMX500LIB][IMAGE_LOADER] DD_DOWNLOAD_STS  %02X\n", regVal);
        
        if (residualSize >= IMX500SF_NW_TRANS_SIZE_UNIT) {
            spiTransactionSize = IMX500SF_NW_TRANS_SIZE_UNIT;
        } else {
            spiTransactionSize = residualSize;
        }

        printf("[IMX500LIB][IMAGE_LOADER] residualSize:%d   spiTransactionSize:%d\n", residualSize,spiTransactionSize);
        while (1) {
            if (spiTransactionSize == 0) {
                break;
            }
            if (spiTransactionSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                transSize = IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                transSize = spiTransactionSize;
            }
            /* reading file */
            size_t tmp_size;
            image_file_read(image_fp, pTransbuf, &tmp_size, transSize);
            copy_byteswap_buffer(pTransbuf, transSize); 
            ret = FW_TransferSpi(pCurBuf, transSize);
            if (ret != 0) {
                return ret;
            }
            if (transSize >= IMX500SF_SPI_ONE_TRANS_SIZE) {
                //pCurBuf = pCurBuf + IMX500SF_SPI_ONE_TRANS_SIZE;
                spiTransactionSize = spiTransactionSize - IMX500SF_SPI_ONE_TRANS_SIZE;
            } else {
                spiTransactionSize = 0;
            }               
        }
        
        /* Update Residual Size */
        if (residualSize >= IMX500SF_NW_TRANS_SIZE_UNIT) {
            residualSize = residualSize - IMX500SF_NW_TRANS_SIZE_UNIT;
        } else {
            residualSize = 0;
        }
    }
    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &regVal, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if ((uint8_t)(regVal) != refStsCnt) {
            break;
        }
        timer_cnt++;
        if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
            printf("[IMX500LIB][IMAGE_LOADER] STATE Change ERROR \n");
            free(pTransbuf);
            return -1; 
        }
    }
    //PRINTF("[IMX500LIB][IMAGE_LOADER] timer_cnt  %x\n", timer_cnt);
    printf("[IMX500LIB][IMAGE_LOADER] DD_REF_STS_REG  %02X\n", regVal);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &regVal, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (regVal != IMX500SF_REG_DD_CMD_RPLY_STS_UPDATE_DONE) {
        printf("[IMX500LIB][IMAGE_LOADER] network boot error(reply status NG): imageType=%d, reply status=0x%02X\n", IMX500SF_REG_NW_DD_IMAGE_TYPE, regVal);
        free(pTransbuf);
        return -1;
    }
    printf("[IMX500LIB][IMAGE_LOADER] DD_CMD_REPLY_STS  %02X\n", regVal);

    fclose(image_fp);
    free(pTransbuf);

    return 0;
}

/**
 * @brief fw_flash_update
 */
int fw_flash_update(void)
{
    int ret;
    size_t exe_freq, flash_type;

    /* get sensor inck frequency and flash type */
    // regsetting_get_freq_type(&exe_freq, &flash_type);

    /* set sensor inck */
    ret = imx501_set_inck(1);
    if (ret != 0) {
        printf("imx501 set inck error\n");
        return -1;
    }

    /* download loader firmware*/
    ret = FW_SpiBoot_ProcFW(IMAGE_FILE_LOADER, IMX500SF_REG_LOADER_DD_IMAGE_TYPE, IMX500SF_REG_LOADER_LOAD_ST_TRANS_CMD);
    if (ret != 0) {
        printf("[IMX500LIB][IMAGE_LOAD] FW_SpiBoot_ProcFW ERROR !!! \n");
    }   

    /* download main firmware */
    ret = FW_SpiBoot_ProcFW(IMAGE_FILE_MAINFW, IMX500SF_REG_MAIN_DD_IMAGE_TYPE, IMX500SF_REG_MAINFW_LOAD_ST_TRANS_CMD);
    if (ret != 0) {
        printf("[IMX500LIB][IMAGE_LOAD] FW_SpiBoot_ProcFW ERROR !!! \n");
    }   

    /* erase flash */
    //FW_FlashErase();

    printf("\nFlashing firmware images ...\n");

    /* update loader firmware*/
    ret = FW_FlashUpdate_ProcFW(IMAGE_FILE_LOADER, IMX500SF_REG_LOADER_DD_IMAGE_TYPE, kDefaultFlashAddrLoader);
    if (ret != 0) {
        printf("[IMX500LIB][IMAGE_UPDATE] FW_FlashUpdate_ProcFW ERROR !!! \n");
    }   

    /* update main firmware */
    ret = FW_FlashUpdate_ProcFW(IMAGE_FILE_MAINFW, IMX500SF_REG_MAIN_DD_IMAGE_TYPE, kDefaultFlashAddrMainFw);
    if (ret != 0) {
        printf("[IMX500LIB][IMAGE_UPDATE] FW_FlashUpdate_ProcFW ERROR !!! \n");
    }   

    /* write sensor registers */
    regsetting_set_reg_val();

    return 0;
}

/**
 * @brief FlashBootKick.
 */
int FlashBootKick(size_t trnsCmd, uint8_t imgType, size_t flashAddr)
{
    int return_code = 0;
    size_t rcv_data = 0;
    size_t timer_cnt = 0;
    static uint8_t m_refStsCnt = 0;

    /* set parameter */
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_ST_TRANS_CMD, trnsCmd, IMX500SF_REG_SIZE_DD_ST_TRANS_CMD);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_LOAD_MODE, 0x01, IMX500SF_REG_SIZE_DD_LOAD_MODE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_IMAGE_TYPE, imgType, IMX500SF_REG_SIZE_DD_IMAGE_TYPE);
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_FLASH_ADDR, flashAddr, IMX500SF_REG_SIZE_DD_FLASH_ADDR);

    /* set command (notify IMX500) */
    I2C_ACCESS_WRITE(IMX500SF_REG_ADDR_DD_CMD_INT, 0x00, IMX500SF_REG_SIZE_DD_CMD_INT);

    /* wait change status */
    while (timer_cnt < IMX500SF_POLLING_TIMEOUT) {
        I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_REF_STS_REG, &rcv_data, IMX500SF_REG_SIZE_DD_REF_STS_REG);
        if (rcv_data != m_refStsCnt) {
            break;
        }
        //SENSOR_USE_TIMER(10000000);  /* wait 10ms */
        timer_cnt++;
    }
    if (timer_cnt >= IMX500SF_POLLING_TIMEOUT) {
        printf("[IMX500LIB][CTRL] boot error(timeout): img type=%d\n", imgType);
        return -1;
    }

    /* update ref status count */
    m_refStsCnt = (uint8_t)(rcv_data);

    /* check reply status */
    I2C_ACCESS_READ(IMX500SF_REG_ADDR_DD_CMD_REPLY_STS, &rcv_data, IMX500SF_REG_SIZE_DD_CMD_REPLY_STS);
    if (rcv_data != 0x01) {
        printf("[IMX500LIB][CTRL] boot error(reply status NG): img type=%d, reply status=0x%02X\n", imgType, rcv_data);
        return_code = -1;
    }

    return return_code;
}

/**
 * @brief fw_flash_boot
 */
int fw_flash_boot(void)
{
    int ret;
    int return_code;
    size_t exe_freq, flash_type;

    /* get sensor inck frequency and flash type */
    // regsetting_get_freq_type(&exe_freq, &flash_type);
    // printf("[IMX500LIB][CTRL]------------------------------ Flash Boot: exe_freq=%zu, flash_type=%zu\n", exe_freq, flash_type);
// [IMX500LIB][CTRL]------------------------------ Flash Boot: exe_freq=0, flash_type=257

    /* set sensor inck */
    ret = imx501_set_inck(0);
    if (ret != 0) {
        printf("imx501 set inck error\n");
        return -1;
    }
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
    printf("[IMX500LIB][CTRL] Flash Boot\n");

    /* loader */
    // printf("[IMX500LIB][CTRL] Flash Boot LoaderFW\n");
    return_code = FlashBootKick(0x00, 0x00, 0x00000000);
    if (return_code != 0) {
        printf("[IMX500LIB][CTRL] Failed open(loader load error) : ret=0x%08X\n", return_code);
        return return_code;
    }
    /* mainfw */
    // printf("[IMX500LIB][CTRL] Flash Boot MainFW\n");
    return_code = FlashBootKick(0x01, 0x01, 0x00020000);
    if (return_code != 0) {
        printf("[IMX500LIB][CTRL] Failed open(mainfw load error) : ret=0x%08X\n", return_code);
        return return_code;
    }
    printf("%s(%d)", __FUNCTION__, __LINE__);
    /* write sensor registers */
    regsetting_set_reg_val();

    return 0;
}
 
