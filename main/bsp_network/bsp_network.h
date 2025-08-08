#ifndef BSP_NETWORK_H
#define BSP_NETWORK_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


// Network configuration macros
#define BSP_NETWORK_USE_WIFI_AP_STA    1
#define BSP_NETWORK_USE_ETHERNET       0
#define BSP_NETWORK_USE_LTE            0

// Network status event bits
#define NETWORK_ETHERNET_CONNECTED_BIT   BIT0
#define NETWORK_WIFI_STA_CONNECTED_BIT   BIT1
#define NETWORK_LTE_CONNECTED_BIT        BIT2

// Network connection status enum
typedef enum {
    NETWORK_STATUS_DISCONNECTED = 0,
    NETWORK_STATUS_ETHERNET_CONNECTED,
    NETWORK_STATUS_WIFI_STA_CONNECTED,
    NETWORK_STATUS_LTE_CONNECTED
} network_status_t;

extern char g_mac_str[13]; // MAC地址字符串格式，如"aabbccddeeff"
extern EventGroupHandle_t g_network_event_group;
 
void bsp_network_init(void);
//等待网络连接就绪 (可调用网络方法)
esp_err_t bsp_network_wait_any_connection(uint32_t timeout_ms);  

void bsp_network_get_mac_address(void);
void bsp_network_print_status(void);

#endif // BSP_NETWORK_H