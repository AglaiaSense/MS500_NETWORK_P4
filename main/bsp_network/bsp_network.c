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
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#if BSP_NETWORK_USE_WIFI_AP_STA
#include "bsp_ap_sta.h"
#endif

#if BSP_NETWORK_USE_ETHERNET
#include "bsp_ethernet.h"
#endif

static const char *TAG = "bsp_network";

// Global MAC address variable
uint8_t g_device_mac[6] = {0};

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

void bsp_network_get_mac_address(void) {
    esp_err_t ret = ESP_OK;

    // Get MAC address using ESP32-P4 compatible function
    ret = esp_read_mac(g_device_mac, ESP_MAC_BASE);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Device MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 g_device_mac[0], g_device_mac[1], g_device_mac[2],
                 g_device_mac[3], g_device_mac[4], g_device_mac[5]);
    } else {
        ESP_LOGE(TAG, "Failed to get device MAC address: %s", esp_err_to_name(ret));
    }
}

void bsp_network_init(void) {
    ESP_LOGI(TAG, "Initializing network BSP...");

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

    // Get and print MAC address first
    bsp_network_get_mac_address();

    // Common network initialization
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#if BSP_NETWORK_USE_WIFI_AP_STA
    ESP_LOGI(TAG, "Using WiFi AP/STA mode");
    bsp_wifi_log_config();
    bsp_ap_sta_init();
#endif

#if BSP_NETWORK_USE_ETHERNET
    ESP_LOGI(TAG, "Using Ethernet mode");
    bsp_ethernet_init();
#endif

#if BSP_NETWORK_USE_LTE
    ESP_LOGI(TAG, "Using LTE mode");
    bsp_lte_init();

#endif

    ESP_LOGI(TAG, "Network BSP initialized successfully");
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
