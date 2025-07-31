/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "esp_cam_sensor.h"
#include "esp_cam_sensor_detect.h"
#include "imx500.h"
#include "imx500_settings.h"

#define IMX500_IO_MUX_LOCK(mux)
#define IMX500_IO_MUX_UNLOCK(mux)
#define IMX500_ENABLE_OUT_CLOCK(pin, clk)
#define IMX500_DISABLE_OUT_CLOCK(pin)

#define IMX500_PID (0x0500)
#define IMX500_SENSOR_NAME "IMX500"
#define IMX500_AE_TARGET_DEFAULT (0x50)

#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif
#define delay_ms(ms) vTaskDelay((ms > portTICK_PERIOD_MS ? ms / portTICK_PERIOD_MS : 1))
#define IMX500_SUPPORT_NUM CONFIG_CAMERA_IMX500_MAX_SUPPORT

static const char *TAG = "imx500";

#define O5647_IDI_CLOCK_RATE_1920x1080_30FPS (91666700ULL)
#define O5647_MIPI_CSI_LINE_RATE_1920x1080_30FPS (O5647_IDI_CLOCK_RATE_1920x1080_30FPS * 5)


static const esp_cam_sensor_isp_info_t imx500_isp_info[] = {

    {.isp_v1_info = {
         .version = SENSOR_ISP_INFO_VERSION_DEFAULT,
         .pclk = 210600000,
         .vts = 2980,
         .hts = 4730,
         .bayer_type = ESP_CAM_SENSOR_BAYER_RGGB,
     }},
 
};

static const esp_cam_sensor_format_t imx500_format_info[] = {
    {
        .name = "imx501_27m_1920x1080_crop_30fps",
        .format = ESP_CAM_SENSOR_PIXFORMAT_RAW10,
        .port = ESP_CAM_SENSOR_MIPI_CSI,
        .xclk = 27000000,
        .width = 1920,
        .height = 1080,
        .regs = imx501_27m_1920x1080_crop_30fps,
        .regs_size = ARRAY_SIZE(imx501_27m_1920x1080_crop_30fps),
        .fps = 30,
        .isp_info = &imx500_isp_info[0],
        .mipi_info = {
            .mipi_clk = 745000000, // 1053000000, //IMX500_MIPI_CSI_LINE_RATE_1920x1080_30FPS,
            .lane_num = 2,
            .line_sync_en = CONFIG_CAMERA_IMX500_CSI_LINESYNC_ENABLE ? true : false,
        },
        .reserved = NULL,
    },
 
};

static esp_err_t imx500_read(esp_sccb_io_handle_t sccb_handle, uint16_t reg, uint8_t *read_buf) {

    esp_err_t ret = esp_sccb_transmit_receive_reg_a16v8(sccb_handle, reg, read_buf);

    // esp_rom_printf("imx500_read: reg=0x%04x, data=0x%02x\n", reg, *read_buf);

    return ret;
}
static esp_err_t imx500_write(esp_sccb_io_handle_t sccb_handle, uint16_t reg, uint8_t data) {
    // esp_rom_printf("imx500_write: reg=0x%04x, data=0x%02x\n", reg, data);

    return esp_sccb_transmit_reg_a16v8(sccb_handle, reg, data);
}

static esp_err_t imx500_read_array(esp_sccb_io_handle_t sccb_handle, const imx500_reginfo_t *regarray) {

    int i = 0;
    esp_err_t ret = ESP_OK;
    uint8_t reg_data = 0;

    // 循环写入寄存器数组中的每个寄存器
    while ((ret == ESP_OK) && regarray[i].reg != IMX500_REG_END) {
        if (regarray[i].reg != IMX500_REG_DELAY) {
            // 如果寄存器不是延迟寄存器，则写入寄存器值
            ret = imx500_read(sccb_handle, regarray[i].reg, &reg_data);
        } else {
            // 如果寄存器是延迟寄存器，则延迟指定的毫秒数
            delay_ms(regarray[i].val);
        }
        i++;
    }
    // 打印写入寄存器的数量
    ESP_LOGD(TAG, "count=%d", i);
    esp_rom_printf("count=%d\n", i);

    return ret;
}

static esp_err_t imx500_write_array(esp_sccb_io_handle_t sccb_handle, const imx500_reginfo_t *regarray) {

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
    // esp_rom_printf("count=%d\n", i);

    return ret;
}
static esp_err_t imx500_set_reg_bits(esp_sccb_io_handle_t sccb_handle, uint16_t reg, uint8_t offset, uint8_t length, uint8_t value) {
    esp_err_t ret = ESP_OK;
    uint8_t reg_data = 0;

    ret = imx500_read(sccb_handle, reg, &reg_data);
    if (ret != ESP_OK) {
        return ret;
    }
    uint8_t mask = ((1 << length) - 1) << offset;
    value = (reg_data & ~mask) | ((value << offset) & mask);
    ret = imx500_write(sccb_handle, reg, value);
    return ret;
}

static esp_err_t imx500_set_test_pattern(esp_cam_sensor_device_t *dev, int enable) {

    return ESP_OK;
}

static esp_err_t imx500_hw_reset(esp_cam_sensor_device_t *dev) {

    if (dev->reset_pin >= 0) {
        gpio_set_level(dev->reset_pin, 0);
        delay_ms(10);
        gpio_set_level(dev->reset_pin, 1);
        delay_ms(10);
    }
    return 0;
}

static esp_err_t imx500_soft_reset(esp_cam_sensor_device_t *dev) {
    return ESP_OK;

    esp_err_t ret = imx500_set_reg_bits(dev->sccb_handle, 0x0103, 0, 1, 0x01);
    delay_ms(5);
    return ret;
}

static esp_err_t imx500_get_sensor_id(esp_cam_sensor_device_t *dev, esp_cam_sensor_id_t *id) {

    // return ESP_OK;

    uint8_t pid_h, pid_l;

    esp_err_t ret = imx500_read(dev->sccb_handle, IMX500_REG_SENSOR_ID_H, &pid_h);
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "read pid_h failed");

    ret = imx500_read(dev->sccb_handle, IMX500_REG_SENSOR_ID_L, &pid_l);
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "read pid_l failed");

    uint16_t pid = (pid_h << 8) | pid_l;
    if (pid) {
        id->pid = pid;
    }

    return ret;
}

/**
 * @brief 设置传感器的流状态。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param enable 启用或禁用流。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_set_stream(esp_cam_sensor_device_t *dev, int enable) {
    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);

    esp_err_t ret;
    //     uint8_t val = IMX500_MIPI_CTRL00_BUS_IDLE;
    //     if (enable) {
    // #if CSI2_NONCONTINUOUS_CLOCK
    //         // 如果启用流且使用非连续时钟，则设置时钟门控和行同步使能
    //         val |= IMX500_MIPI_CTRL00_CLOCK_LANE_GATE | IMX500_MIPI_CTRL00_LINE_SYNC_ENABLE;
    // #endif
    //     } else {
    //         // 如果禁用流，则设置时钟门控和时钟通道禁用
    //         val |= IMX500_MIPI_CTRL00_CLOCK_LANE_GATE | IMX500_MIPI_CTRL00_CLOCK_LANE_DISABLE;
    //     }

    //     // 写入寄存器0x4800，设置CSI行同步使能
    //     ret = imx500_write(dev->sccb_handle, 0x4800, CONFIG_CAMERA_IMX500_CSI_LINESYNC_ENABLE ? 0x14 : 0x00);
    //     ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write pad out failed");

    // #if CONFIG_CAMERA_IMX500_ISP_AF_ENABLE
    //     // 如果启用ISP自动对焦，则写入相关寄存器
    //     ret = imx500_write(dev->sccb_handle, 0x3002, enable ? 0x01 : 0x00);
    //     ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write pad out failed");

    //     ret = imx500_write(dev->sccb_handle, 0x3010, enable ? 0x01 : 0x00);
    //     ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write pad out failed");

    //     ret = imx500_write(dev->sccb_handle, 0x300D, enable ? 0x01 : 0x00);
    //     ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write pad out failed");
    // #endif

    // 写入寄存器0x0100，启用或禁用流
    ret = imx500_write(dev->sccb_handle, 0x0100, enable ? 0x01 : 0x01);
 
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write pad out failed");

    // 更新流状态
    dev->stream_status = enable;

    // 打印流状态
    // ESP_LOGI(TAG, "Stream=%d", enable);
    return ret;
}

/**
 * @brief 设置传感器的水平镜像。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param enable 启用或禁用水平镜像。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_set_mirror(esp_cam_sensor_device_t *dev, int enable) {
    return ESP_OK;

    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);

    // 设置寄存器位以启用或禁用水平镜像
    return imx500_set_reg_bits(dev->sccb_handle, 0x3821, 1, 1, enable ? 0x01 : 0x00);
}
/**
 * @brief 设置传感器的垂直翻转。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param enable 启用或禁用垂直翻转。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_set_vflip(esp_cam_sensor_device_t *dev, int enable) {
    return ESP_OK;

    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);

    // 设置寄存器位以启用或禁用垂直翻转
    return imx500_set_reg_bits(dev->sccb_handle, 0x3820, 1, 1, enable ? 0x01 : 0x00);
}

/**
 * @brief 设置传感器的自动曝光（AE）目标值。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param target 要设置的AE目标值。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_set_AE_target(esp_cam_sensor_device_t *dev, int target) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    return ESP_OK;

    esp_err_t ret = ESP_OK;
    /* 稳定在高位 */
    int fast_high, fast_low;
    // 计算高低AE值
    int AE_low = target * 23 / 25;  /* 0.92 */
    int AE_high = target * 27 / 25; /* 1.08 */

    // 计算快速高低AE值
    fast_high = AE_high << 1;
    if (fast_high > 255) {
        fast_high = 255;
    }

    fast_low = AE_low >> 1;

    // 写入AE高低值到寄存器
    ret |= imx500_write(dev->sccb_handle, 0x3a0f, AE_high);
    ret |= imx500_write(dev->sccb_handle, 0x3a10, AE_low);
    ret |= imx500_write(dev->sccb_handle, 0x3a1b, AE_high);
    ret |= imx500_write(dev->sccb_handle, 0x3a1e, AE_low);
    ret |= imx500_write(dev->sccb_handle, 0x3a11, fast_high);
    ret |= imx500_write(dev->sccb_handle, 0x3a1f, fast_low);

    return ret;
}

static esp_err_t imx500_query_para_desc(esp_cam_sensor_device_t *dev, esp_cam_sensor_param_desc_t *qdesc) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    esp_err_t ret = ESP_OK;
    switch (qdesc->id) {
    case ESP_CAM_SENSOR_VFLIP:
    case ESP_CAM_SENSOR_HMIRROR:
        qdesc->type = ESP_CAM_SENSOR_PARAM_TYPE_NUMBER;
        qdesc->number.minimum = 0;
        qdesc->number.maximum = 1;
        qdesc->number.step = 1;
        qdesc->default_value = 0;
        break;
    case ESP_CAM_SENSOR_EXPOSURE_VAL:
        qdesc->type = ESP_CAM_SENSOR_PARAM_TYPE_NUMBER;
        qdesc->number.minimum = 2;
        qdesc->number.maximum = 235;
        qdesc->number.step = 1;
        qdesc->default_value = IMX500_AE_TARGET_DEFAULT;
        break;
    default: {
        ESP_LOGD(TAG, "id=%" PRIx32 " is not supported", qdesc->id);
        ret = ESP_ERR_INVALID_ARG;
        break;
    }
    }
    return ret;
}

/**
 * @brief 获取传感器参数值。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param id 参数ID。
 * @param arg 存储参数值的指针。
 * @param size 参数值的大小。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_get_para_value(esp_cam_sensor_device_t *dev, uint32_t id, void *arg, size_t size) {
    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    return ESP_ERR_NOT_SUPPORTED;
}
/**
 * @brief 设置传感器参数值。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param id 参数ID。
 * @param arg 指向参数值的指针。
 * @param size 参数值的大小。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_set_para_value(esp_cam_sensor_device_t *dev, uint32_t id, const void *arg, size_t size) {
    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    esp_err_t ret = ESP_OK;

    switch (id) {
    case ESP_CAM_SENSOR_VFLIP: {
        // 获取参数值并设置垂直翻转
        int *value = (int *)arg;
        ret = imx500_set_vflip(dev, *value);
        break;
    }
    case ESP_CAM_SENSOR_HMIRROR: {
        // 获取参数值并设置水平镜像
        int *value = (int *)arg;
        ret = imx500_set_mirror(dev, *value);
        break;
    }
    case ESP_CAM_SENSOR_EXPOSURE_VAL: {
        // 获取参数值并设置曝光值
        int *value = (int *)arg;
        ret = imx500_set_AE_target(dev, *value);
        break;
    }
    default: {
        // 不支持的参数ID
        ESP_LOGE(TAG, "set id=%" PRIx32 " is not supported", id);
        ret = ESP_ERR_INVALID_ARG;
        break;
    }
    }

    return ret;
}
static esp_err_t imx500_query_support_formats(esp_cam_sensor_device_t *dev, esp_cam_sensor_format_array_t *formats) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    // ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, dev);
    // ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, formats);

    formats->count = ARRAY_SIZE(imx500_format_info);
    formats->format_array = &imx500_format_info[0];
    return ESP_OK;
}

static esp_err_t imx500_query_support_capability(esp_cam_sensor_device_t *dev, esp_cam_sensor_capability_t *sensor_cap) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    // ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, dev);
    // ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, sensor_cap);

    sensor_cap->fmt_raw = 1;
    return ESP_OK;
}
/**
 * @brief 获取系统时钟频率。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @return int 系统时钟频率。
 */
static int imx500_get_sysclk(esp_cam_sensor_device_t *dev) {
    return ESP_OK;

    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    /* 计算系统时钟 */
    int xvclk = dev->cur_format->xclk / 10000;
    int sysclk = 0;
    uint8_t temp1, temp2;
    int pre_div02x, div_cnt7b, sdiv0, pll_rdiv, bit_div2x, sclk_div, VCO;
    // 预分频器映射表
    const int pre_div02x_map[] = {2, 2, 4, 6, 8, 3, 12, 5, 16, 2, 2, 2, 2, 2, 2, 2};
    // sdiv0映射表
    const int sdiv0_map[] = {16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    // pll_rdiv映射表
    const int pll_rdiv_map[] = {1, 2};
    // bit_div2x映射表
    const int bit_div2x_map[] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 5, 2, 2, 2, 2, 2};
    // sclk_div映射表
    const int sclk_div_map[] = {1, 2, 4, 1};

    // 读取寄存器值并计算预分频器值
    imx500_read(dev->sccb_handle, 0x3037, &temp1);
    temp2 = temp1 & 0x0f;
    pre_div02x = pre_div02x_map[temp2];
    temp2 = (temp1 >> 4) & 0x01;
    pll_rdiv = pll_rdiv_map[temp2];
    imx500_read(dev->sccb_handle, 0x3036, &temp1);

    // 读取寄存器值并计算分频计数器值
    div_cnt7b = temp1;

    // 计算VCO频率
    VCO = xvclk * 2 / pre_div02x * div_cnt7b;
    imx500_read(dev->sccb_handle, 0x3035, &temp1);
    temp2 = temp1 >> 4;
    sdiv0 = sdiv0_map[temp2];
    imx500_read(dev->sccb_handle, 0x3034, &temp1);
    temp2 = temp1 & 0x0f;
    bit_div2x = bit_div2x_map[temp2];
    imx500_read(dev->sccb_handle, 0x3106, &temp1);
    temp2 = (temp1 >> 2) & 0x03;
    sclk_div = sclk_div_map[temp2];
    // 计算系统时钟频率
    sysclk = VCO * 2 / sdiv0 / pll_rdiv / bit_div2x / sclk_div;
    return sysclk;
}
/**
 * @brief 获取水平总尺寸（HTS）。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @return int 水平总尺寸（HTS）。
 */
static int imx500_get_hts(esp_cam_sensor_device_t *dev) {
    return ESP_OK;

    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    /* 从寄存器设置中读取HTS */
    int hts = 0;
    uint8_t temp1, temp2;

    // 读取寄存器0x380c和0x380d的值
    imx500_read(dev->sccb_handle, 0x380c, &temp1);
    imx500_read(dev->sccb_handle, 0x380d, &temp2);
    // 计算HTS值
    hts = (temp1 << 8) + temp2;

    return hts;
}

/**
 * @brief 获取垂直总尺寸（VTS）。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @return int 垂直总尺寸（VTS）。
 */
static int imx500_get_vts(esp_cam_sensor_device_t *dev) {
    return ESP_OK;

    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    /* 从寄存器设置中读取VTS */
    int vts = 0;
    uint8_t temp1, temp2;

    /* 垂直总尺寸[15:8]高字节 */
    imx500_read(dev->sccb_handle, 0x380e, &temp1);
    imx500_read(dev->sccb_handle, 0x380f, &temp2);

    // 计算VTS值
    vts = (temp1 << 8) + temp2;

    return vts;
}

static int imx500_get_light_freq(esp_cam_sensor_device_t *dev) {
    return 60;

    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    /* get banding filter value */
    uint8_t temp, temp1;
    int light_freq = 0;

    imx500_read(dev->sccb_handle, 0x3c01, &temp);

    if (temp & 0x80) {
        /* manual */
        imx500_read(dev->sccb_handle, 0x3c00, &temp1);
        if (temp1 & 0x04) {
            /* 50Hz */
            light_freq = 50;
        } else {
            /* 60Hz */
            light_freq = 60;
        }
    } else {
        /* auto */
        imx500_read(dev->sccb_handle, 0x3c0c, &temp1);
        if (temp1 & 0x01) {
            /* 50Hz */
            light_freq = 50;
        } else {
            light_freq = 60;
        }
    }
    return light_freq;
}
/**
 * @brief 设置频闪滤波器。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_set_bandingfilter(esp_cam_sensor_device_t *dev) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    return ESP_OK;

    // 打印函数名和行号
    esp_err_t ret;
    int prev_sysclk, prev_VTS, prev_HTS;
    int band_step60, max_band60, band_step50, max_band50;

    // 读取预览时的系统时钟
    prev_sysclk = imx500_get_sysclk(dev);
    // 读取预览时的水平总尺寸
    prev_HTS = imx500_get_hts(dev);

    // 读取预览时的垂直总尺寸
    prev_VTS = imx500_get_vts(dev);

    // 计算频闪滤波器
    // 60Hz
    band_step60 = prev_sysclk * 100 / prev_HTS * 100 / 120;
    ret = imx500_write(dev->sccb_handle, 0x3a0a, (uint8_t)(band_step60 >> 8));
    ret |= imx500_write(dev->sccb_handle, 0x3a0b, (uint8_t)(band_step60 & 0xff));

    max_band60 = (int)((prev_VTS - 4) / band_step60);
    ret |= imx500_write(dev->sccb_handle, 0x3a0d, (uint8_t)max_band60);

    // 50Hz
    band_step50 = prev_sysclk * 100 / prev_HTS;
    ret |= imx500_write(dev->sccb_handle, 0x3a08, (uint8_t)(band_step50 >> 8));
    ret |= imx500_write(dev->sccb_handle, 0x3a09, (uint8_t)(band_step50 & 0xff));

    max_band50 = (int)((prev_VTS - 4) / band_step50);
    ret |= imx500_write(dev->sccb_handle, 0x3a0e, (uint8_t)max_band50);
    return ret;
}

static esp_err_t imx500_set_format(esp_cam_sensor_device_t *dev, const esp_cam_sensor_format_t *format) {
    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);

    // 检查指针是否为空
    ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, dev);

    esp_err_t ret = ESP_OK;
    /* 根据接口类型，自动加载可用配置。
    您可以在不使用query_format()的情况下设置传感器的输出格式。*/
    if (format == NULL) {
        if (dev->sensor_port == ESP_CAM_SENSOR_MIPI_CSI) {
            // 如果格式为空且接口类型为MIPI CSI，则使用默认格式
            format = &imx500_format_info[CONFIG_CAMERA_IMX500_MIPI_IF_FORMAT_INDEX_DAFAULT];
        } else {
            // 如果接口类型不支持，打印错误信息
            ESP_LOGE(TAG, "Not support DVP port");
        }
    }

    // esp_rom_printf("format 1 ：%s\n", format->name);
    // 重置传感器
    ret = imx500_write_array(dev->sccb_handle, imx500_mipi_reset_regs);
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write reset regs failed");
    // 写入格式相关寄存器
    ret = imx500_write_array(dev->sccb_handle, (const imx500_reginfo_t *)format->regs);
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write fmt regs failed");

    // esp_rom_printf("format 2 :%s\n", format->name);
    // 设置自动曝光目标
    ret = imx500_set_AE_target(dev, IMX500_AE_TARGET_DEFAULT);
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "set ae target failed");
    // 设置频闪滤波器
    imx500_set_bandingfilter(dev);

    // 默认停止流
    ret = imx500_set_stream(dev, 0);
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "write stream regs failed");
    // 打印光频
    ESP_LOGD(TAG, "light freq=0x%x", imx500_get_light_freq(dev));

    // 更新当前格式
    dev->cur_format = format;

    return ret;
}

/**
 * @brief 获取传感器的当前格式。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param format 指向存储当前格式的结构体的指针。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_get_format(esp_cam_sensor_device_t *dev, esp_cam_sensor_format_t *format) {
    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    // 检查指针是否为空
    ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, dev);
    ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, format);

    esp_err_t ret = ESP_FAIL;

    // 如果当前格式不为空，则复制当前格式到format指针指向的结构体
    if (dev->cur_format != NULL) {
        memcpy(format, dev->cur_format, sizeof(esp_cam_sensor_format_t));
        ret = ESP_OK;
    }
    return ret;
}

/**
 * @brief 私有IO控制函数，用于处理各种传感器命令。
 *
 * @param dev 指向摄像头传感器设备的指针。
 * @param cmd 要执行的命令。
 * @param arg 命令参数。
 * @return esp_err_t 成功时返回ESP_OK，否则返回错误代码。
 */
static esp_err_t imx500_priv_ioctl(esp_cam_sensor_device_t *dev, uint32_t cmd, void *arg) {
    // 打印函数名和行号
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);

    // 检查指针是否为空
    ESP_CAM_SENSOR_NULL_POINTER_CHECK(TAG, dev);

    esp_err_t ret = ESP_FAIL;
    uint8_t regval;
    esp_cam_sensor_reg_val_t *sensor_reg;
    // 锁定IO多路复用
    IMX500_IO_MUX_LOCK(mux);
    switch (cmd) {
    case ESP_CAM_SENSOR_IOC_HW_RESET:
        // 硬件复位
        ret = imx500_hw_reset(dev);
        break;
    case ESP_CAM_SENSOR_IOC_SW_RESET:
        // 软件复位
        ret = imx500_soft_reset(dev);
        break;
    case ESP_CAM_SENSOR_IOC_S_REG:
        // 设置寄存器值
        sensor_reg = (esp_cam_sensor_reg_val_t *)arg;
        ret = imx500_write(dev->sccb_handle, sensor_reg->regaddr, sensor_reg->value);
        break;
    case ESP_CAM_SENSOR_IOC_S_STREAM:
        // 设置流状态
        ret = imx500_set_stream(dev, *(int *)arg);
        break;
    case ESP_CAM_SENSOR_IOC_S_TEST_PATTERN:
        // 设置测试模式
        ret = imx500_set_test_pattern(dev, *(int *)arg);
        break;
    case ESP_CAM_SENSOR_IOC_G_REG:
        // 获取寄存器值
        sensor_reg = (esp_cam_sensor_reg_val_t *)arg;
        ret = imx500_read(dev->sccb_handle, sensor_reg->regaddr, &regval);
        if (ret == ESP_OK) {
            sensor_reg->value = regval;
        }
        break;
    case ESP_CAM_SENSOR_IOC_G_CHIP_ID:
        // 获取芯片ID
        ret = imx500_get_sensor_id(dev, arg);
        break;
    default:
        // 无效的命令
        ret = ESP_ERR_INVALID_ARG;
        break;
    }
    // 解锁IO多路复用
    IMX500_IO_MUX_UNLOCK(mux);
    return ret;
}

static esp_err_t imx500_power_on(esp_cam_sensor_device_t *dev) {
    // return ESP_OK;

    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    esp_err_t ret = ESP_OK;

    if (dev->xclk_pin >= 0) {
        IMX500_ENABLE_OUT_CLOCK(dev->xclk_pin, dev->xclk_freq_hz);
    }

    if (dev->pwdn_pin >= 0) {
        gpio_config_t conf = {0};
        conf.pin_bit_mask = 1LL << dev->pwdn_pin;
        conf.mode = GPIO_MODE_OUTPUT;
        ret = gpio_config(&conf);
        ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "pwdn pin config failed");

        // carefully, logic is inverted compared to reset pin
        gpio_set_level(dev->pwdn_pin, 1);
        delay_ms(10);
        gpio_set_level(dev->pwdn_pin, 0);
        delay_ms(10);
    }

    if (dev->reset_pin >= 0) {
        gpio_config_t conf = {0};
        conf.pin_bit_mask = 1LL << dev->reset_pin;
        conf.mode = GPIO_MODE_OUTPUT;
        ret = gpio_config(&conf);
        ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "reset pin config failed");

        gpio_set_level(dev->reset_pin, 0);
        delay_ms(10);
        gpio_set_level(dev->reset_pin, 1);
        delay_ms(10);
    }

    return ret;
}

static esp_err_t imx500_power_off(esp_cam_sensor_device_t *dev) {
    // return ESP_OK;

    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    esp_err_t ret = ESP_OK;

    if (dev->xclk_pin >= 0) {
        IMX500_DISABLE_OUT_CLOCK(dev->xclk_pin);
    }

    if (dev->pwdn_pin >= 0) {
        gpio_set_level(dev->pwdn_pin, 0);
        delay_ms(10);
        gpio_set_level(dev->pwdn_pin, 1);
        delay_ms(10);
    }

    if (dev->reset_pin >= 0) {
        gpio_set_level(dev->reset_pin, 1);
        delay_ms(10);
        gpio_set_level(dev->reset_pin, 0);
        delay_ms(10);
    }

    return ret;
}

static esp_err_t imx500_delete(esp_cam_sensor_device_t *dev) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    // ESP_LOGD(TAG, "del imx500 (%p)", dev);
    if (dev) {
        free(dev);
        dev = NULL;
    }

    return ESP_OK;
}

static const esp_cam_sensor_ops_t imx500_ops = {
    .query_para_desc = imx500_query_para_desc,
    .get_para_value = imx500_get_para_value,
    .set_para_value = imx500_set_para_value,
    .query_support_formats = imx500_query_support_formats,
    .query_support_capability = imx500_query_support_capability,
    .set_format = imx500_set_format,
    .get_format = imx500_get_format,
    .priv_ioctl = imx500_priv_ioctl,
    .del = imx500_delete};

// We need manage these devices, and maybe need to add it into the private member of esp_device
esp_cam_sensor_device_t *imx500_detect(esp_cam_sensor_config_t *config) {
    // esp_rom_printf("%s(%d)\n", __func__, __LINE__);
    esp_cam_sensor_device_t *dev = NULL;

    if (config == NULL) {
        return NULL;
    }

    dev = calloc(1, sizeof(esp_cam_sensor_device_t));
    if (dev == NULL) {
        ESP_LOGE(TAG, "No memory for camera");
        return NULL;
    }

    dev->name = (char *)IMX500_SENSOR_NAME;
    dev->sccb_handle = config->sccb_handle;
    dev->xclk_pin = config->xclk_pin;
    dev->reset_pin = config->reset_pin;
    dev->pwdn_pin = config->pwdn_pin;
    dev->sensor_port = config->sensor_port;
    dev->ops = &imx500_ops;
    if (config->sensor_port == ESP_CAM_SENSOR_MIPI_CSI) {
        dev->cur_format = &imx500_format_info[CONFIG_CAMERA_IMX500_MIPI_IF_FORMAT_INDEX_DAFAULT];
    } else {
        ESP_LOGE(TAG, "Not support DVP port");
    }

    // Configure sensor power, clock, and SCCB port
    if (imx500_power_on(dev) != ESP_OK) {
        ESP_LOGE(TAG, "Camera power on failed");
        goto err_free_handler;
    }

    // imx500_read_array(dev->sccb_handle, imx500_input_24M_MIPI_2lane_raw8_800x640_50fps);

    if (imx500_get_sensor_id(dev, &dev->id) != ESP_OK) {
        ESP_LOGE(TAG, "Get sensor ID failed");
        goto err_free_handler;
    } else if (dev->id.pid != IMX500_PID) {
        // ESP_LOGE(TAG, "Camera sensor is not IMX500, PID=0x%x", dev->id.pid);
        // goto err_free_handler;
    }
    ESP_LOGI(TAG, "Detected Camera sensor PID=0x%x", dev->id.pid);

    return dev;

err_free_handler:
    imx500_power_off(dev);
    free(dev);

    return NULL;
}

// #if CONFIG_CAMERA_IMX500_AUTO_DETECT_MIPI_INTERFACE_SENSOR
ESP_CAM_SENSOR_DETECT_FN(imx500_detect, ESP_CAM_SENSOR_MIPI_CSI, IMX500_SCCB_ADDR) {
    ((esp_cam_sensor_config_t *)config)->sensor_port = ESP_CAM_SENSOR_MIPI_CSI;
    return imx500_detect(config);
}
// #endif
