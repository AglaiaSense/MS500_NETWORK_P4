#include "bsp_network.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_eth.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "bsp_mqtt.h"

#if BSP_NETWORK_USE_WIFI_AP_STA
#include "bsp_ap_sta.h"
#endif

#if BSP_NETWORK_USE_ETHERNET
#include "bsp_ethernet.h"
#endif

static const char *TAG = "bsp_network";

// Network connection event group
static EventGroupHandle_t s_network_event_group;
#define NETWORK_CONNECTED_BIT BIT0
#define NETWORK_FAIL_BIT BIT1

// Global MAC address variable
uint8_t g_device_mac[6] = {0};

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

static void bsp_network_connection_event_handler(void *arg, esp_event_base_t event_base,
                                                 int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected");
        xEventGroupSetBits(s_network_event_group, NETWORK_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_network_event_group, NETWORK_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Ethernet got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_network_event_group, NETWORK_CONNECTED_BIT);
    }
}

void bsp_network_wait_connection(void) {
    if (!s_network_event_group) {
        ESP_LOGE(TAG, "Network event group not initialized");
        return;
    }
    
    ESP_LOGI(TAG, "Waiting for network connection...");
    
    EventBits_t bits = xEventGroupWaitBits(s_network_event_group,
                                           NETWORK_CONNECTED_BIT | NETWORK_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & NETWORK_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Network connected successfully");
        ESP_LOGI(TAG, "Starting MQTT initialization...");
        bsp_mqtt_init();
    } else if (bits & NETWORK_FAIL_BIT) {
        ESP_LOGI(TAG, "Network connection failed");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void bsp_network_init(void)
{
    ESP_LOGI(TAG, "Initializing network BSP...");
    
    // Get and print MAC address first
    bsp_network_get_mac_address();
    
    // Create network event group
    s_network_event_group = xEventGroupCreate();
    
    // Common network initialization
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Register network connection event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, 
                                              &bsp_network_connection_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                              &bsp_network_connection_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, 
                                              &bsp_network_connection_event_handler, NULL));
    
#if BSP_NETWORK_USE_WIFI_AP_STA
    ESP_LOGI(TAG, "Using WiFi AP/STA mode");
    bsp_wifi_log_config();
    bsp_ap_sta_init();
    
#elif BSP_NETWORK_USE_ETHERNET
    ESP_LOGI(TAG, "Using Ethernet mode"); 
    bsp_ethernet_init();
    
#else
    #error "No network mode selected! Please enable either BSP_NETWORK_USE_WIFI_AP_STA or BSP_NETWORK_USE_ETHERNET"
#endif

    ESP_LOGI(TAG, "Network BSP initialized successfully");
    
    // Wait for network connection and start MQTT when connected
    bsp_network_wait_connection();
}