#ifndef BSP_AP_STA_H
#define BSP_AP_STA_H

// WiFi mode configuration
#define BSP_WIFI_MODE_STA_ONLY    1
#define BSP_WIFI_MODE_AP_ONLY     2
#define BSP_WIFI_MODE_AP_STA      3

// Function declarations
void bsp_ap_sta_init(void);
void bsp_wifi_sta_init(void);
void bsp_wifi_ap_init(void);
void bsp_wifi_log_config(void);

// Helper functions for modular initialization
void bsp_wifi_sta_config_only(void);
void bsp_wifi_ap_config_only(void);

#endif // BSP_AP_STA_H
