
#ifndef __AI_DRIVER_H__
#define __AI_DRIVER_H__

#include "vc_config.h"


/*!< 传感器硬件复位引脚    */
#define GPIO_OUTPUT_IO_RST (3)


#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_RST))

void ai_gpio_init(void);


esp_err_t imx501_register_read(uint16_t reg_addr, size_t *val, size_t size);
esp_err_t imx501_register_write(uint16_t reg_addr, size_t val, size_t size);
esp_err_t imx501_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) ;

#endif