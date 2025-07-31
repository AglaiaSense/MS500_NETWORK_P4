/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#include "ai_driver.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"


#include "ai_mipi_i2c.h"
#include "ai_update_i2c.h"

void ai_gpio_init(void) {
    /*---- GPIO输出模式配置 ----*/
    // 初始化GPIO配置结构体
    gpio_config_t io_conf = {};
    // 禁用中断功能
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 设置为输出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 通过位掩码选择要配置的GPIO引脚
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // 关闭下拉电阻
    io_conf.pull_down_en = 0;
    // 关闭上拉电阻
    io_conf.pull_up_en = 0;
    // 应用GPIO配置
    gpio_config(&io_conf);

    /*---- 传感器硬件初始化序列 ----*/
    // 拉低复位引脚（硬件复位准备）
    gpio_set_level(GPIO_OUTPUT_IO_RST, 0);


    // 保持低电平100ms确保复位有效
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // 释放复位引脚（结束硬件复位）
    gpio_set_level(GPIO_OUTPUT_IO_RST, 1);
    // 等待传感器初始化完成
    vTaskDelay(100 / portTICK_PERIOD_MS);
}




/**
 * @brief Read a sequence of bytes from a IMX501 sensor registers
 */
esp_err_t imx501_register_read(uint16_t reg_addr, size_t *data, size_t size) {

    esp_err_t ret;

    if (boot_mode == BOOT_UPDATE_FLASH) {
        ret = ai_update_i2c_register_read(reg_addr, data, size);
    } else {

        ret = ai_mipi_i2c_register_read(reg_addr, data, size);
    }

    return ret;
}

/**
 * @brief Read a sequence of bytes from a IMX501 sensor registers
 */
esp_err_t imx501_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) {
    esp_err_t ret;

    if (boot_mode == BOOT_UPDATE_FLASH) {
        ret = ai_update_i2c_register_read_id(reg_addr, data, size);

    } else {
        ret = ai_mipi_i2c_register_read_id(reg_addr, data, size);
    }

    return ret;
}

/**
 * @brief Write to IMX501 sensor register
 */
esp_err_t imx501_register_write(uint16_t reg_addr, size_t data, size_t size) {

    esp_err_t ret;
    if (boot_mode == BOOT_UPDATE_FLASH) {
        ret = ai_update_i2c_register_write(reg_addr, data, size);

    } else {
        ret = ai_mipi_i2c_register_write(reg_addr, data, size);
    }

    return ret;
}
