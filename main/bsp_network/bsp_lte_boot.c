#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "bsp_lte_boot.h"
#include "bsp_lte_uart.h"
#include "bsp_lte_eg25.h"

// 全局事件组句柄
EventGroupHandle_t bsp_lte_boot_event_group;

// 示例数据（从原main.c中移植）
static int camera_id = 0;
static char water_number[12] = "889900";
static int water_count = 0;

// LTE启动任务
void bsp_lte_boot_task(void *pvParameters) {
    printf("%s \r\n", "------------------- bsp_lte_boot_task --------------------------------");

    // 初始化LTE模块
    bsp_lte_eg25_init();
    
    // 标记 LTE 启动成功
    xEventGroupSetBits(bsp_lte_boot_event_group, LTE_BOOT_OK_BIT);
    printf("LTE boot task completed\r\n");

    vTaskDelete(NULL);
}

// UART启动任务
void bsp_lte_uart_boot_task(void *pvParameters) {
    int condition = 1;
    while (true) {
        printf("bsp_lte_uart_boot_task running: %d\r\n", condition);

        // 模拟 UART 启动过程
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时 1 秒
        condition++;
        
        if (condition > 10) {
            // 标记 UART 启动成功
            xEventGroupSetBits(bsp_lte_boot_event_group, UART_BOOT_OK_BIT);
            printf("UART boot is successful.\n");
            vTaskDelete(NULL);
        }
    }
}

// 发送HTTP GET请求示例
static void send_get_request() {
    char full_url[256];
    memset(full_url, 0, sizeof(full_url));

    const char *camera_sn = "CA500-MIPI-BELZ-0010";
    const char *url = "https://dm-be.leopardaws.com/camera/c";
    sprintf(full_url, "%s?c_sn=%s", url, camera_sn);

    char respond_data[1024];
    esp_err_t result = bsp_lte_http_get_request(full_url, respond_data);

    if (result == ESP_OK) {
        printf("*********** HTTP GET SUCCESS **************\r\n");
        printf("%s\n", respond_data);
        printf("-------------------------\r\n");
        
        // 这里可以解析JSON获取camera_id
        // camera_id = parse_json_and_get_id(respond_data);
        
    } else if (result == ESP_ERR_NO_MEM) {
        printf("HTTP GET failed: Insufficient response data buffer\n");
    } else {
        printf("HTTP GET failed: other error\n");
    }
}

// 发送HTTP POST请求示例
static void send_post_request() {
    char respond_data[RX1_BUF_SIZE];
    const char *url = "https://dm-be.leopardaws.com/record/r/";
    
    // 构建简单的JSON参数
    char params[256];
    sprintf(params, "{\"r_camera\": [%d], \"r_detail\": \"{\\\"water\\\": {\\\"label\\\": \\\"Water Meter Reading\\\", \\\"value\\\": \\\"%s\\\"}}\"}",
            camera_id, water_number);

    esp_err_t result = bsp_lte_http_post_request(url, params, respond_data);

    if (result == ESP_OK) {
        printf("*********** HTTP POST SUCCESS **************\r\n");
        printf("%s\n", respond_data);
        printf("-------------------------\r\n");
    } else if (result == ESP_ERR_NO_MEM) {
        printf("HTTP POST failed: Insufficient response data buffer\n");
    } else {
        printf("HTTP POST failed: other error\n");
    }
}

// 获取水表数据任务
void bsp_lte_get_water_meter_task(void *pvParameters) {
    while (1) {
        // 等待 LTE 和 UART 启动成功的事件
        EventBits_t uxBits = xEventGroupWaitBits(
            bsp_lte_boot_event_group,           // 事件组句柄
            LTE_BOOT_OK_BIT | UART_BOOT_OK_BIT, // 等待的事件标志
            pdFALSE,                            // 不清除事件标志
            pdTRUE,                             // 等待所有标志位
            portMAX_DELAY                       // 无限期等待
        );

        if ((uxBits & (LTE_BOOT_OK_BIT | UART_BOOT_OK_BIT)) == (LTE_BOOT_OK_BIT | UART_BOOT_OK_BIT)) {
            printf("bsp_lte_get_water_meter_task running\r\n");

            vTaskDelay(pdMS_TO_TICKS(5000)); // 延时5秒
            if (water_count++ >= 3) {
                water_count = 0;

                send_get_request();
                send_post_request();
            }
        }
    }
}
void configure_gpio() {

 
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_27); // 选择 GPIO27
    io_conf.mode = GPIO_MODE_OUTPUT;              // 设置为输出模式
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // 禁用上拉
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉
    io_conf.intr_type = GPIO_INTR_DISABLE;        // 禁用中断
    gpio_config(&io_conf);                        // 应用配置
    gpio_set_level(GPIO_NUM_27, 1);
  
 
}

// LTE模块初始化
void bsp_lte_init(void) {
    printf("%s \r\n", "------------------- bsp_lte_init --------------------------------");

    configure_gpio();

    // 创建事件组
    bsp_lte_boot_event_group = xEventGroupCreate();

    // 初始化UART
    bsp_lte_uart_init();

    // 创建LTE相关任务
    xTaskCreate(bsp_lte_boot_task, "lte_boot_task", 1024 * 5, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(bsp_lte_uart_boot_task, "uart_boot_task", 2048, NULL, 5, NULL);
    xTaskCreate(bsp_lte_get_water_meter_task, "get_water_meter_task", 1024 * 5, NULL, configMAX_PRIORITIES - 2, NULL);
    
    printf("LTE initialization tasks created\r\n");
}