#ifndef BSP_LTE_UART_H
#define BSP_LTE_UART_H

#include <stdint.h>
#include "driver/uart.h"

// UART1 配置定义
#define RX1_BUF_SIZE (1024)
#define TX1_BUF_SIZE (512)
#define TXD1_PIN (GPIO_NUM_46)
#define RXD1_PIN (GPIO_NUM_47)

// 全局变量声明
extern char uart1_rx_data[RX1_BUF_SIZE+1];

// 函数声明
void bsp_lte_uart_init_gpio(void);
void bsp_lte_uart_init(void);
void bsp_lte_uart_rx_task(void *pvParameters);

// 辅助函数
void bsp_lte_delay_ms(int nms);
void bsp_lte_clear_buffer(void);
void bsp_lte_delay_uart_rx(void);

#endif // BSP_LTE_UART_H