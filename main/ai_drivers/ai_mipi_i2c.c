#include "ai_mipi_i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_video_device_internal.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "as_tools.h"

static const char *TAG = "AI_MIPI_I2C";

#define IMX500_REG_END 0xffff   ///< 结束寄存器
#define IMX500_REG_DELAY 0xeeee ///< 延迟寄存器


#define delay_ms(ms) vTaskDelay((ms > portTICK_PERIOD_MS ? ms / portTICK_PERIOD_MS : 1))

static esp_sccb_io_handle_t sccb_handle = NULL;

void ai_mipi_i2c_copy_sccb_handle(void) {

    esp_cam_sensor_device_t *cam_dev = esp_video_get_csi_video_device_sensor();
    if (cam_dev && cam_dev->sccb_handle) {
        sccb_handle = cam_dev->sccb_handle;
    } else {
        ESP_LOGE(TAG, "Failed to get camera sensor device");
    }
}

// --------------------------------------------------------  DNN 读写i2c  --------------------------------------------------------

esp_err_t ai_mipi_i2c_register_read(uint16_t reg_addr, size_t *data, size_t size) {
    if (!sccb_handle || !data || (size != 1 && size != 2 && size != 4)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t temp;
    *data = 0;

    for (size_t i = 0; i < size; i++) {
        esp_err_t ret = esp_sccb_transmit_receive_reg_a16v8(sccb_handle, reg_addr + i, &temp);
        if (ret != ESP_OK) {
            return ret;
        }
        *data |= ((size_t)temp << ((size - 1 - i) * 8));
    }

    return ESP_OK;
}

esp_err_t ai_mipi_i2c_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) {
    if (!sccb_handle || !data || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < size; i++) {
        esp_err_t ret = esp_sccb_transmit_receive_reg_a16v8(sccb_handle, reg_addr + i, &data[i]);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t ai_mipi_i2c_register_write(uint16_t reg_addr, size_t data, size_t size) {
    if (!sccb_handle || (size != 1 && size != 2 && size != 4)) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < size; i++) {
        uint8_t byte = (data >> ((size - 1 - i) * 8)) & 0xFF;
        esp_err_t ret = esp_sccb_transmit_reg_a16v8(sccb_handle, reg_addr + i, byte);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

// --------------------------------------------------------  mipi 读写i2c  --------------------------------------------------------

static esp_err_t imx500_write(esp_sccb_io_handle_t sccb_handle, uint16_t reg, uint8_t data) {
    // esp_rom_printf("imx500_write: reg=0x%04x, data=0x%02x\n", reg, data);

    return esp_sccb_transmit_reg_a16v8(sccb_handle, reg, data);
}

static esp_err_t imx500_write_array(esp_sccb_io_handle_t sccb_handle, const imx500_reginfo_t *regarray) {
    // 打印函数名和行号

    int i = 0;
    esp_err_t ret = ESP_OK;
    // 循环写入寄存器数组中的每个寄存器
    while ((ret == ESP_OK) && regarray[i].reg != IMX500_REG_END) {
        if (regarray[i].reg != IMX500_REG_DELAY) {
            // 如果寄存器不是延迟寄存器，则写入寄存器值
            ret = imx500_write(sccb_handle, regarray[i].reg, regarray[i].val);
        } else {
            // 如果寄存器是延迟寄存器，则延迟指定的毫秒数
            delay_ms(regarray[i].val);
        }
        i++;
    }
    // 打印写入寄存器的数量
    ESP_LOGD(TAG, "count=%d", i);

    return ret;
}
// extern const imx500_reginfo_t* get_imx501_27m_1920x1080_crop_30fps_config(void);


void ai_mipi_again_init(void) {

  imx500_write_array(sccb_handle, get_imx501_27m_1920x1080_crop_30fps_config());

    
}