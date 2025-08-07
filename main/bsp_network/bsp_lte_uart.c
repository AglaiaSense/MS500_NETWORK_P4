#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <portmacro.h>

#include "bsp_lte_uart.h"

char uart1_rx_data[RX1_BUF_SIZE+1];

// 初始化UART GPIO配置
void bsp_lte_uart_init_gpio(void) {
    // 串口1参数配置
    uart_config_t uart1_config = {
        .baud_rate = 115200,                  // 波特率
        .data_bits = UART_DATA_8_BITS,        // 数据位
        .parity = UART_PARITY_DISABLE,        // 校验位
        .stop_bits = UART_STOP_BITS_1,        // 停止位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE // 硬件流控
    };
    uart_param_config(UART_NUM_2, &uart1_config); // 设置串口
    // IO映射-> T:IO47  R:IO46
    esp_err_t ret = uart_set_pin(UART_NUM_2, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    printf("uart_set_pin ret = %d \r\n", ret);

    // 注册串口服务即使能+设置缓存区大小
    uart_driver_install(UART_NUM_2, RX1_BUF_SIZE * 2, TX1_BUF_SIZE * 2, 0, NULL, 0);
    printf("uart_driver_install ret = %d \r\n", ret);
}

// UART接收任务
void bsp_lte_uart_rx_task(void *pvParameters) {
    uint8_t *temp_buf = (uint8_t *)malloc(RX1_BUF_SIZE);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_2, temp_buf, RX1_BUF_SIZE, 10 / portTICK_PERIOD_MS);

        if (rxBytes > 0) {
            temp_buf[rxBytes] = 0;

            printf("*********** rx_buf **************\r\n");
            printf("%s\n", temp_buf);
            printf("-------------------------\r\n");

            int current_length = strlen(uart1_rx_data);
            // 检查是否有足够的空间进行追加
            if ((current_length + rxBytes) < RX1_BUF_SIZE) {
                // 使用 memcpy 进行追加
                memcpy(uart1_rx_data + current_length, temp_buf, rxBytes);
            } else {
                memset(uart1_rx_data, 0, RX1_BUF_SIZE);
                memcpy(uart1_rx_data, temp_buf, RX1_BUF_SIZE);
                printf("Not enough space to append data.\n");
            }
        }
    }
    free(temp_buf);
}

// 初始化UART
void bsp_lte_uart_init(void) {
    printf("%s \r\n", "------------------- bsp_lte_uart_init --------------------------------");
    
    // 串口初始化
    bsp_lte_uart_init_gpio();

    // 创建串口1接收任务
    xTaskCreate(bsp_lte_uart_rx_task, "uart1_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}

// ------------------  辅助函数  ------------------

// 延时函数
void bsp_lte_delay_ms(int nms) {
    vTaskDelay(pdMS_TO_TICKS(nms));
}

// 清除缓冲区
void bsp_lte_clear_buffer(void) {
    memset(uart1_rx_data, 0, RX1_BUF_SIZE);
}

// UART接收延时
void bsp_lte_delay_uart_rx(void) {
    vTaskDelay(pdMS_TO_TICKS(500));
}