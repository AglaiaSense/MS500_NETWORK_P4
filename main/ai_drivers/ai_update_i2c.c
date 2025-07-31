#include "ai_update_i2c.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "AI_UPDATE_I2C";

static i2c_master_bus_handle_t bus_handle;  // I2C总线句柄
static i2c_master_dev_handle_t dev_handle;  // I2C设备句柄

void ai_update_i2c_master_init(void) {
    esp_err_t ret;
    
    // I2C总线配置
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,  // 启用内部上拉
    };
    
    // 创建I2C总线
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    ESP_ERROR_CHECK(ret);
    
    // 设备配置
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMX501_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    // 添加设备到总线
    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "I2C NG driver initialized successfully");
}

void ai_update_i2c_master_uninit(void) {
    // 移除设备并删除总线
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C NG driver de-initialized successfully");
}

esp_err_t ai_update_i2c_register_read(uint16_t reg_addr, size_t *data, size_t size) {
    uint8_t write_buf[2] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff};  // 16位寄存器地址
    uint8_t read_buf[4] = {0};  // 读取缓冲区
    
    // 执行写-读操作
    esp_err_t err = i2c_master_transmit_receive(
        dev_handle, 
        write_buf, sizeof(write_buf),
        read_buf, size,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
    
    if (err == ESP_OK) {
        *data = 0;
        // 组合字节数据
        for (int i = 0; i < size; i++) {
            *data |= (size_t)(read_buf[i]) << (8 * (size - 1 - i));
        }
    }
    return err;
}

esp_err_t ai_update_i2c_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) {
    uint8_t write_buf[2] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff};  // 16位寄存器地址
    
    // 执行写-读操作
    return i2c_master_transmit_receive(
        dev_handle,
        write_buf, sizeof(write_buf),
        data, size,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
}

esp_err_t ai_update_i2c_register_write(uint16_t reg_addr, size_t data, size_t size) {
    uint8_t write_buf[6] = {
        (reg_addr >> 8) & 0xff,  // 寄存器地址高字节
        reg_addr & 0xff,         // 寄存器地址低字节
    };
    
    // 填充数据字节
    if (size == 1) {
        write_buf[2] = data & 0xff;
    } else if (size == 2) {
        write_buf[2] = (data >> 8) & 0xff;
        write_buf[3] = data & 0xff;
    } else if (size == 4) {
        write_buf[2] = (data >> 24) & 0xff;
        write_buf[3] = (data >> 16) & 0xff;
        write_buf[4] = (data >> 8) & 0xff;
        write_buf[5] = data & 0xff;
    }
    
    // 执行写入操作
    return i2c_master_transmit(
        dev_handle,
        write_buf,
        size + 2,  // 地址字节 + 数据字节
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
}