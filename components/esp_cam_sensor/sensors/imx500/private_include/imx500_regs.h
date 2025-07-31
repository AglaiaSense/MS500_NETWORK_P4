/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief IMX500传感器寄存器定义。
 *
 * 该文件包含了IMX500传感器的寄存器地址定义，用于配置和控制传感器的各种功能。
 */

 #define IMX500_REG_DELAY            0xeeee  ///< 延迟寄存器
 #define IMX500_REG_END              0xffff  ///< 结束寄存器
 #define IMX500_REG_SENSOR_ID_H      0x0016  ///< 传感器ID高字节寄存器
 #define IMX500_REG_SENSOR_ID_L      0x0017  ///< 传感器ID低字节寄存器
 #define IMX500_REG_SLEEP_MODE       0x0100  ///< 睡眠模式寄存器
 #define IMX500_REG_MIPI_CTRL00      0x4800  ///< MIPI控制寄存器
 #define IMX500_REG_FRAME_OFF_NUMBER 0x4202  ///< 帧关闭编号寄存器
 #define IMX500_REG_PAD_OUT          0x300d  ///< 传感器PAD输出寄存器
#ifdef __cplusplus
}
#endif
