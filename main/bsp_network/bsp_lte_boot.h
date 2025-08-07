#ifndef BSP_LTE_BOOT_H
#define BSP_LTE_BOOT_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// 事件标志定义
#define LTE_BOOT_OK_BIT (1 << 0)
#define UART_BOOT_OK_BIT (1 << 1)

// 全局事件组句柄声明
extern EventGroupHandle_t bsp_lte_boot_event_group;

// LTE启动相关任务函数
void bsp_lte_boot_task(void *pvParameters);
void bsp_lte_uart_boot_task(void *pvParameters);

// LTE初始化函数
void bsp_lte_init(void);

// 获取水表数据任务
void bsp_lte_get_water_meter_task(void *pvParameters);

#endif // BSP_LTE_BOOT_H