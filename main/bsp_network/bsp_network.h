#ifndef BSP_NETWORK_H
#define BSP_NETWORK_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


// Network configuration macros
#define BSP_NETWORK_USE_WIFI_AP_STA    1
#define BSP_NETWORK_USE_ETHERNET       1
#define BSP_NETWORK_USE_LTE            1

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

extern uint8_t g_device_mac[6];
extern EventGroupHandle_t g_network_event_group;
 
void bsp_network_init(void);
void bsp_network_get_mac_address(void);
void bsp_network_print_status(void);

#endif // BSP_NETWORK_H