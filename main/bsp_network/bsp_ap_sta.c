/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "bsp_mqtt.h"
#include "bsp_network.h"

#include "bsp_ap_sta.h"

// External reference to global MAC address
extern uint8_t g_device_mac[6];

static const char *TAG = "bsp_ap_sta";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define EXAMPLE_ESP_WIFI_SSID  "AglaiaSense"
#define EXAMPLE_ESP_WIFI_PASS  "Guangshi@20243"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

// AP configuration macros
#define BSP_AP_SSID_PREFIX "MS500"
#define BSP_AP_PASSWORD "12345678"
 
// ------------------------------------      event      ------------------------------------

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "WiFi EVENT type %s id %d", event_base, (int)event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
        // Set WiFi STA connected bit in network event group
        if (g_network_event_group != NULL) {
            xEventGroupSetBits(g_network_event_group, NETWORK_WIFI_STA_CONNECTED_BIT);
            ESP_LOGI(TAG, "WiFi STA connected bit set in network event group");
        }
    }
}


static void wifi_init_common(void) {
    s_wifi_event_group = xEventGroupCreate();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
}

// -------------------- config ---------------------------------
void bsp_wifi_log_config(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
}

void bsp_wifi_sta_config_only(void) {
    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
}

static void generate_ap_ssid(char *ssid_buffer, size_t buffer_size) {
    snprintf(ssid_buffer, buffer_size, "%s_%02X%02X%02X%02X%02X%02X", 
             BSP_AP_SSID_PREFIX, g_device_mac[0], g_device_mac[1], g_device_mac[2], 
             g_device_mac[3], g_device_mac[4], g_device_mac[5]);
}

void bsp_wifi_ap_config_only(void) {
    char ap_ssid[32];
    generate_ap_ssid(ap_ssid, sizeof(ap_ssid));
    
    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid_len = strlen(ap_ssid),
            .password = BSP_AP_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    
    // Copy SSID to config
    strcpy((char*)wifi_config_ap.ap.ssid, ap_ssid);
    
    if (strlen(BSP_AP_PASSWORD) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_LOGI(TAG, "AP SSID: %s", ap_ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
}

// -------------------- init    ------------------------
void bsp_wifi_sta_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi STA mode");
    
    esp_netif_create_default_wifi_sta();
    wifi_init_common();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    bsp_wifi_sta_config_only();
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi STA initialization finished");
}

void bsp_wifi_ap_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi AP mode");
    
    esp_netif_create_default_wifi_ap();
    wifi_init_common();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    bsp_wifi_ap_config_only();
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP initialization finished");
}

void bsp_ap_sta_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi AP+STA mode");
    
    // Create network interfaces for both AP and STA
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    
    // Initialize WiFi common components
    wifi_init_common();
    
    // Set AP+STA mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // Configure both AP and STA using helper functions
    bsp_wifi_sta_config_only();
    bsp_wifi_ap_config_only();
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP+STA initialization finished");
}
