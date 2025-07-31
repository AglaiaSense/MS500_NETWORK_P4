/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "imx500_regs.h"
#include "imx500_types.h"
#include "sdkconfig.h"
#include <stdint.h>

#define BIT(nr) (1UL << (nr)) ///< 位操作宏，返回指定位置的位值

///< 1920x1080分辨率30FPS的IDI时钟速率
#define IMX500_IDI_CLOCK_RATE_1920x1080_30FPS (500000000ULL)
///< 1920x1080分辨率30FPS的MIPI CSI线路速率
#define IMX500_MIPI_CSI_LINE_RATE_1920x1080_30FPS (IMX500_IDI_CLOCK_RATE_1920x1080_30FPS * 4)


#define IMX500_MIPI_CTRL00_CLOCK_LANE_GATE BIT(5)    ///< MIPI控制寄存器：时钟通道门控
#define IMX500_MIPI_CTRL00_LINE_SYNC_ENABLE BIT(4)   ///< MIPI控制寄存器：行同步使能
#define IMX500_MIPI_CTRL00_BUS_IDLE BIT(2)           ///< MIPI控制寄存器：总线空闲
#define IMX500_MIPI_CTRL00_CLOCK_LANE_DISABLE BIT(0) ///< MIPI控制寄存器：时钟通道禁用

/**
 * @brief IMX500 MIPI复位寄存器配置。
 */
static const imx500_reginfo_t imx500_mipi_reset_regs[] = {
    {0x0100, 0x00},           // enable sleep，启用睡眠模式
    // {IMX500_REG_DELAY, 0x0a}, // 延迟10ms
    {IMX500_REG_END, 0x00},   // 结束标志
};
static imx500_reginfo_t imx501_27m_1920x1080_crop_30fps[] = {
    
    // Power on sequence
    {0x0004, 0x00},
    {0x0007, 0x80},
    {0x0007, 0x00},
    {0x0005, 0x01},
    {0x0004, 0x01},
    {0x0004, 0x03},
    {0x0004, 0x07},
    {0x0005, 0x12},
    // {IMX501_TABLE_WAIT_MS, IMX501_WAIT_MS_STREAM},
    // common register
    {0xA700, 0x01},  // image only   

    // ---------------------------- 进行二次初始化  ----------------------------
   {0x0100, 0x00}, //  stop streaming
 
    {IMX500_REG_END, 0x00}  // End of register list
};


#ifdef __cplusplus
}
#endif
