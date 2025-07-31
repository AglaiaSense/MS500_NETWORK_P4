/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef __IMX501_H__
#define __IMX501_H__

 #include "stdint.h"
 #include "esp_err.h"
 #include "as_tools.h"


extern esp_err_t imx501_register_read(uint16_t reg_addr, size_t *val, size_t size);
extern esp_err_t imx501_register_write(uint16_t reg_addr, size_t val, size_t size);
extern esp_err_t imx501_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) ;
 

#define I2C_ACCESS_WRITE(addr, val, size) {                                             \
            esp_err_t ret = imx501_register_write((addr), (val), (size));    \
            if (ret != ESP_OK) {                                                          \
                return ret;                                                             \
            }                                                                           \
        }

#define I2C_ACCESS_READ(addr, val, size) {                                              \
            esp_err_t ret = imx501_register_read((addr), (val), (size));     \
            if (ret != ESP_OK) {                                                          \
                return ret;                                                             \
            }                                                                           \
        }

typedef struct {
    size_t    m_inckFreq;
    uint16_t    m_rawImgWidth;
    uint16_t    m_rawImgHeight;
    uint16_t    m_rawImgWidthMax;
    uint16_t    m_rawImgHeightMax;
} Imx500Param;

esp_err_t imx501_register_init(void);


size_t imx501_set_inck(int update_flag);
int imx501_set_mipi_bit_rate(size_t mipiLane);

int imx501_start(void);
int stream_start(void);

// Function to write a table of register values
esp_err_t imx501_write_table(const imx501_reg_t *table, size_t size);

int regsetting_set_reg_val(void);



#endif