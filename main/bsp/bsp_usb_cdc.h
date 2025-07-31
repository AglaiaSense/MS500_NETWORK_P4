// bsp_usb_cdc.h

#ifndef BSP_USB_CDC_H
#define BSP_USB_CDC_H

 #include "esp_err.h"

/**
 * @brief 初始化USB CDC设备
 */
void bsp_usb_cdc_init(void);

void bsp_usb_cdc_deinit(void) ;

#endif // BSP_USB_CDC_H
