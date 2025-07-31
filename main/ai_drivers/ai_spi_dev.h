/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef __AI_SPI_DEV_H__
#define __AI_SPI_DEV_H__

#include "ai_driver.h"
#include "stdint.h"

// 主设备引脚配置
#define PIN_MASTER_MISO (13) // 主设备MISO引脚
#define PIN_MASTER_MOSI (14) // 主设备MOSI引脚
#define PIN_MASTER_CLK (12)  // 主设备时钟引脚
#define PIN_MASTER_CS (11)   // 主设备片选引脚

// 主设备最大传输大小
#define MASTER_MAX_TRANS_SIZE (4096)

// 从设备引脚配置
#define PIN_SLAVE_MISO (8) // 从设备MISO引脚
#define PIN_SLAVE_MOSI (10) // 从设备MOSI引脚
#define PIN_SLAVE_CLK (9)  // 从设备时钟引脚
#define PIN_SLAVE_CS (7)   // 从设备片选引脚

// LPD/LPR输入输出最大数据大小
#define LPD_LPR_INOUT_MAX_SIZE (249580)

void spi_master_dev_init(void);
void spi_master_dev_destroy(void);
void spi_master_dev_read(uint8_t *data);
int spi_master_dev_write(uint8_t *data, int len);
void spi_master_test() ;



void spi_slave_dev_init(void);

#endif