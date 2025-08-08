#include "bsp_network.h"
#include "bsp_lte_boot.h"
#include "bsp_mqtt.h"
#include "bsp_http.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string.h>

#if BSP_NETWORK_USE_WIFI_AP_STA
#include "bsp_ap_sta.h"
#endif

#if BSP_NETWORK_USE_ETHERNET
#include "bsp_ethernet.h"
#endif

static const char *TAG = "bsp_network";

// ESP-Hosted从设备重置GPIO定义
#define ESP_HOSTED_RESET_GPIO 22
#define ESP_HOSTED_RESET_ACTIVE_HIGH 1

// Global MAC address string variable
char g_mac_str[13] = {0}; // MAC地址字符串格式，如"aabbccddeeff"

// Global network event group
EventGroupHandle_t g_network_event_group = NULL;

// Network monitoring task
static void network_monitor_task(void *pvParameter) {
    static bool mqtt_initialized = false;
    static bool http_initialized = false;

    while (1) {
        // 等待任何网络连接事件
        EventBits_t bits = xEventGroupWaitBits(g_network_event_group,
                                               NETWORK_ETHERNET_CONNECTED_BIT | NETWORK_WIFI_STA_CONNECTED_BIT | NETWORK_LTE_CONNECTED_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        // 检查是否有网络连接且服务未初始化
        if ((bits & (NETWORK_ETHERNET_CONNECTED_BIT | NETWORK_WIFI_STA_CONNECTED_BIT | NETWORK_LTE_CONNECTED_BIT))) {
            
            // 初始化HTTP客户端
            if (!http_initialized) {
                ESP_LOGI(TAG, "Network connected, initializing HTTP...");
                bsp_http_init();
                http_initialized = true;
            }
            
            // 初始化MQTT
            if (!mqtt_initialized) {
                ESP_LOGI(TAG, "Network connected, initializing MQTT...");
                bsp_mqtt_init();
                mqtt_initialized = true;
            }
            
            // 所有服务都初始化完成后删除任务
            if (mqtt_initialized && http_initialized) {
                vTaskDelete(NULL); // 删除当前任务
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒检查一次
    }
}

// ESP-Hosted从设备重置函数
static void esp_hosted_reset_slave(void) {
    ESP_LOGI(TAG, "Resetting ESP-Hosted slave device using GPIO[%d]", ESP_HOSTED_RESET_GPIO);
    
    // 配置GPIO为输出模式
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << ESP_HOSTED_RESET_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO[%d]: %s", ESP_HOSTED_RESET_GPIO, esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "GPIO [%d] configured", ESP_HOSTED_RESET_GPIO);
    
    // 执行重置序列
    if (ESP_HOSTED_RESET_ACTIVE_HIGH) {
        // 高电平有效重置
        gpio_set_level(ESP_HOSTED_RESET_GPIO, 0);  // 确保初始为低电平
        vTaskDelay(pdMS_TO_TICKS(10));             // 等待10ms
        gpio_set_level(ESP_HOSTED_RESET_GPIO, 1);  // 拉高触发重置
        vTaskDelay(pdMS_TO_TICKS(100));            // 保持100ms
        gpio_set_level(ESP_HOSTED_RESET_GPIO, 0);  // 释放重置
        vTaskDelay(pdMS_TO_TICKS(200));            // 等待设备稳定
    } else {
        // 低电平有效重置
        gpio_set_level(ESP_HOSTED_RESET_GPIO, 1);  // 确保初始为高电平
        vTaskDelay(pdMS_TO_TICKS(10));             // 等待10ms
        gpio_set_level(ESP_HOSTED_RESET_GPIO, 0);  // 拉低触发重置
        vTaskDelay(pdMS_TO_TICKS(100));            // 保持100ms
        gpio_set_level(ESP_HOSTED_RESET_GPIO, 1);  // 释放重置
        vTaskDelay(pdMS_TO_TICKS(200));            // 等待设备稳定
    }
    
    ESP_LOGI(TAG, "ESP-Hosted slave device reset sequence completed");
}

void bsp_network_get_mac_address(void) {
    esp_err_t ret = ESP_OK;
    uint8_t mac_bytes[6] = {0};

    // Get MAC address using ESP32-P4 compatible function
    ret = esp_read_mac(mac_bytes, ESP_MAC_BASE);
    if (ret == ESP_OK) {
        // 直接生成字符串格式的MAC地址
        snprintf(g_mac_str, sizeof(g_mac_str), "%02x%02x%02x%02x%02x%02x",
                 mac_bytes[0], mac_bytes[1], mac_bytes[2],
                 mac_bytes[3], mac_bytes[4], mac_bytes[5]);
        
        ESP_LOGI(TAG, "Device MAC Address: %02X:%02X:%02X:%02X:%02X:%02X (str: %s)",
                 mac_bytes[0], mac_bytes[1], mac_bytes[2],
                 mac_bytes[3], mac_bytes[4], mac_bytes[5], g_mac_str);
    } else {
        ESP_LOGE(TAG, "Failed to get device MAC address: %s", esp_err_to_name(ret));
        // 如果获取失败，使用默认值
        strcpy(g_mac_str, "000000000000");
    }
}

// 异步任务：WiFi初始化
#if BSP_NETWORK_USE_WIFI_AP_STA
static void wifi_init_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting WiFi initialization task...");
    
    // 关键修复：在WiFi初始化前重置ESP-Hosted从设备
    esp_hosted_reset_slave();
    
    bsp_wifi_log_config();
    bsp_ap_sta_init();
    
    ESP_LOGI(TAG, "WiFi initialization task completed");
    vTaskDelete(NULL);
}
#endif

// 异步任务：以太网初始化
#if BSP_NETWORK_USE_ETHERNET
static void ethernet_init_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting Ethernet initialization task...");
    
    bsp_ethernet_init();
    
    ESP_LOGI(TAG, "Ethernet initialization task completed");
    vTaskDelete(NULL);
}
#endif

// 异步任务：LTE初始化
#if BSP_NETWORK_USE_LTE
static void lte_init_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting LTE initialization task...");
    
    bsp_lte_init();
    
    ESP_LOGI(TAG, "LTE initialization task completed");
    vTaskDelete(NULL);
}
#endif

void bsp_network_init(void) {
    ESP_LOGI(TAG, "Initializing network BSP (async mode)...");

    // 关键修复：在所有网络组件初始化前，优先重置ESP-Hosted从设备
    ESP_LOGI(TAG, "Performing early ESP-Hosted slave reset before network initialization");

    // Create network event group
    g_network_event_group = xEventGroupCreate();
    if (g_network_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create network event group");
        return;
    }

    // Create network monitoring task
    if (xTaskCreate(network_monitor_task, "network_monitor", 1024*2, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create network monitor task");
        return;
    }

    // Get and print MAC address first (同步执行，快速完成)
    bsp_network_get_mac_address();

    // Common network initialization (同步执行，必须完成才能创建其他网络接口)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    
    // 异步启动WiFi初始化任务
#if BSP_NETWORK_USE_WIFI_AP_STA
    ESP_LOGI(TAG, "Creating WiFi AP/STA initialization task...");
    if (xTaskCreate(wifi_init_task, "wifi_init", 1024*4, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create WiFi initialization task");
    }
#endif

    // 异步启动以太网初始化任务
#if BSP_NETWORK_USE_ETHERNET
    ESP_LOGI(TAG, "Creating Ethernet initialization task...");
    if (xTaskCreate(ethernet_init_task, "eth_init", 1024*4, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Ethernet initialization task");
    }
#endif

    // 异步启动LTE初始化任务
#if BSP_NETWORK_USE_LTE
    ESP_LOGI(TAG, "Creating LTE initialization task...");
    if (xTaskCreate(lte_init_task, "lte_init", 1024*4, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LTE initialization task");
    }
#endif

    ESP_LOGI(TAG, "Network BSP async initialization started (tasks created)");
}

//------------------ 异步初始化辅助功能 ------------------

// 等待任意网络连接就绪（可选的同步等待）
esp_err_t bsp_network_wait_any_connection(uint32_t timeout_ms) {
    if (g_network_event_group == NULL) {
        ESP_LOGE(TAG, "Network event group not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Waiting for any network connection (timeout: %lu ms)...", timeout_ms);

    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    EventBits_t bits = xEventGroupWaitBits(g_network_event_group,
                                           NETWORK_ETHERNET_CONNECTED_BIT | NETWORK_WIFI_STA_CONNECTED_BIT | NETWORK_LTE_CONNECTED_BIT,
                                           pdFALSE,  // 不清除位
                                           pdFALSE,  // 等待任意一个位
                                           timeout_ticks);

    if (bits & (NETWORK_ETHERNET_CONNECTED_BIT | NETWORK_WIFI_STA_CONNECTED_BIT | NETWORK_LTE_CONNECTED_BIT)) {
        if (bits & NETWORK_ETHERNET_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Ethernet connection ready");
        }
        if (bits & NETWORK_WIFI_STA_CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi STA connection ready");
        }
        if (bits & NETWORK_LTE_CONNECTED_BIT) {
            ESP_LOGI(TAG, "LTE connection ready");
        }
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "No network connection established within timeout");
        return ESP_ERR_TIMEOUT;
    }
}

//------------------ 网络状态查询功能 ------------------

// 获取当前网络连接状态并打印
void bsp_network_print_status(void) {
    ESP_LOGI(TAG, "=== Network Status ===");
    
    if (g_network_event_group == NULL) {
        ESP_LOGW(TAG, "Network not initialized");
        return;
    }
    
    // 获取当前事件位状态
    EventBits_t current_bits = xEventGroupGetBits(g_network_event_group);
    bool has_connection = false;
    
    // 检查以太网连接状态
    if (current_bits & NETWORK_ETHERNET_CONNECTED_BIT) {
        ESP_LOGI(TAG, "√ Ethernet: Connected");
        has_connection = true;
    } else {
        ESP_LOGI(TAG, "× Ethernet: Disconnected");
    }
    
    // 检查WiFi STA连接状态
    if (current_bits & NETWORK_WIFI_STA_CONNECTED_BIT) {
        ESP_LOGI(TAG, "√ WiFi STA: Connected");
        has_connection = true;
    } else {
        ESP_LOGI(TAG, "× WiFi STA: Disconnected");
    }
    
    // 检查LTE连接状态
    if (current_bits & NETWORK_LTE_CONNECTED_BIT) {
        ESP_LOGI(TAG, "√ LTE: Connected");
        has_connection = true;
    } else {
        ESP_LOGI(TAG, "× LTE: Disconnected");
    }
    
    // 总结连接状态
    if (has_connection) {
        ESP_LOGI(TAG, "Network Status: Online");
    } else {
        ESP_LOGW(TAG, "Network Status: Offline - No active connections");
    }
    
    ESP_LOGI(TAG, "==================");
}
