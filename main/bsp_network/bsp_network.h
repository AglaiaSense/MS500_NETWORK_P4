#ifndef BSP_NETWORK_H
#define BSP_NETWORK_H

#include <stdint.h>

// Network configuration macros
#define BSP_NETWORK_USE_WIFI_AP_STA    0
#define BSP_NETWORK_USE_ETHERNET       1

// Global MAC address variable
extern uint8_t g_device_mac[6];

// Network BSP interface functions
void bsp_network_init(void);
void bsp_network_get_mac_address(void);
void bsp_network_wait_connection(void);

#endif // BSP_NETWORK_H