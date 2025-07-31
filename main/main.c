/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: ESPRESSIF MIT
 */

#include "esp_spiffs.h"

#include "bsp_rtc.h"
#include "bsp_sd_card.h"
#include "bsp_sleep.h"
#include "vc_config.h"
#include "vc_save_jpg.h"
#include "vc_uvc_cam.h"
#include "vc_uvc_jpg.h"
#include "vc_video.h"

#include "esp_sleep.h"

#include "imx501.h"

#include "ai_driver.h"
#include "ai_mipi_i2c.h"
#include "ai_spi_dev.h"
#include "ai_spi_receive.h"
#include "ai_update_i2c.h"
#include "fw_dnn.h"
#include "fw_loader.h"

#include "bsp_sd_ota.h"
#include "bsp_spiff.h"
#include "bsp_storage.h"

#include "bsp_sd_dnn.h"

#include "output_tensor_parser.h"

#include "bsp_usb_cdc.h"
#include "vc_video_v4l2.h"

static const char *TAG = "APP_MAIN";

extern int imx501_set_standby(int enable);

extern void enter_light_sleep_before();
extern void enter_light_sleep_after();

// 全局变量声明
device_ctx_t *device_ctx;
TaskHandle_t spi_slave_task_handle = NULL;

uvc_model_t current_model = UVC_MODEL_JPG;

capture_mode_t g_capture_mode = CAPTURE_MODE_CONTINUOUS;

int runtime_max_sec;         // 最大运行时间秒数
int runtime_sec = 0;         // 运行时间秒数
bool is_image_first = false; // 是否是第一次拍照
int ms_counter = 0;          // 计数器用于每秒执行一次秒级任务

int plate_detection_count = 0; // 车牌检测次数
int32_t plate_max_count = 5;   // 最大车牌检测次数

int is_sleeping = 0;
int is_process_done = 0;

extern bool is_jpg_reading;
extern bool is_jpg_writeing;

//----------------------------------------------- task -------------------------------------------------

void ai_spi_receive_task(void *pvParam) {
    is_image_first = true;

    spi_slave_receive_data();
}
void ai_spi_receive_task_start(void) {

    xTaskCreate(ai_spi_receive_task, "ai_spi_receive_task", 1024 * 20, NULL, 5, NULL);
}

void enter_sleep_model() {
    enter_light_sleep_before();

    // enter_light_sleep_time(SEC_TO_USEC(10));
    enter_light_sleep_gpio();

    enter_light_sleep_after();
}
void handle_reset_param(void) {

    plate_result.x = 0;
    plate_result.y = 0;
    plate_result.w = 0;
    plate_result.h = 0;
    plate_result.image_count = 0;
    plate_result.score = 0.0f;
    memset(plate_result.plate, 0, sizeof(plate_result.plate));

    is_process_done = 0;
    is_sleeping = 0;

    runtime_sec = 0;           // 运行时间秒数
    ms_counter = 0;            //
    plate_detection_count = 0; // 车牌检测次数

    if (g_capture_mode == CAPTURE_MODE_SINGLE_SHOT) {
        plate_max_count = 0;
        runtime_max_sec = 10 * 1;

    } else if (g_capture_mode == CAPTURE_MODE_CONTINUOUS) {
        plate_max_count = 1 + 5;
        runtime_max_sec = 10 * 1;
    }
}

void main_jpg_task(void *pvParameters) {

    handle_reset_param();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100)); // 每100ms执行一次

        // === 1000ms 逻辑：1.打印每秒 2.进入休眠模式
        ms_counter += 100;
        if (ms_counter >= 1000) {
            ms_counter = 0;

            printf("runtime ..........: %d\r\n", runtime_sec++);

            if (runtime_sec >= runtime_max_sec) {
                enter_sleep_model();
            }
        }

        // === 100ms 逻辑：1.保存图片 2.是否睡眠
        if (is_plate_detection == true) {

            if (plate_detection_count < plate_max_count) {
                plate_detection_count++; // 增加车牌检测次数

                runtime_max_sec += 5; // 检测到车牌，增加最大运行时间5s
                vc_save_jpg_init(device_ctx);
            }
        }
        // == 初始化启动，就保存一张照片
        if (is_image_first == true) {
            is_image_first = false;
            vc_save_jpg_init(device_ctx);
        }
    }
    vTaskDelete(NULL);
}
void main_video_task(void *pvParameters) {

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("runtime ..........: %d\r\n", runtime_sec++);
        // printf("22222222222222222222 ..........: %d\r\n", runtime_sec++);
    }
    vTaskDelete(NULL);
}

void app_task_init(void) {

    if (current_model == UVC_MODEL_VIDEO ||
        current_model == UVC_MODEL_ALL) {
        xTaskCreate(main_video_task, "main_video_task", 1024 * 2, NULL, 5, NULL);

    } else if (current_model == UVC_MODEL_JPG) {
        xTaskCreate(main_jpg_task, "main_jpg_task", 1024 * 6, NULL, 5, NULL);
    }
}
//----------------------------------------------- sleep -------------------------------------------------
extern video_state_t g_video_state = VIDEO_STATE_STOPPED;
void enter_light_sleep_before() {
    ESP_LOGI(TAG, "----------------------------------------: before");

    is_sleeping = 1;

    while (is_jpg_writeing == true) {
        delay_ms(100);
    }
    while (is_jpg_reading == true) {
        delay_ms(10);
    }

    while (is_process_done == 0) {
        delay_ms(100);
    }

    delay_ms(100);
    imx501_set_standby(0);

    delay_ms(100);
    vc_uvc_cam_deinit(device_ctx);

    delay_ms(100);
    uvc_device_deinit();

    // sd卡最后卸载，别的功能还在用sd卡
    delay_ms(100);
    bsp_deinit_sd_card(device_ctx);
}
void enter_light_sleep_after() {
    ESP_LOGI(TAG, "-----------------------------------------: after");
    handle_reset_param();

    handle_wake_event();

    bsp_init_sd_card(device_ctx);

    imx501_set_standby(1);

    uvc_device_init();
    bsp_usb_cdc_init();

    ai_spi_receive_task_start();
}

//----------------------------------------------- init -------------------------------------------------

void app_cam_device_init(void) {

    // 重启imx500
    ai_gpio_init();

    if (boot_mode == BOOT_LAUNCH_FLASH) {
        handle_wake_event();
    }

    spi_master_dev_init();
}

void video_cam_init(void) {

    // 初始化I2c,mipi
    vc_video_init(device_ctx);

    if (current_model == UVC_MODEL_VIDEO) {
        vc_uvc_cam_init(device_ctx);
    } else if (current_model == UVC_MODEL_JPG) {
        vc_uvc_jpg_init(0);

    } else if (current_model == UVC_MODEL_ALL) {
        vc_uvc_cam_init(device_ctx);
        vc_uvc_jpg_init(1);
    }

    uvc_device_init();
    bsp_usb_cdc_init();
}

void imx501_main_init(void) {

    /* 初始化imx501 */
    imx501_register_init();

    /* firmware初始化 */

    if (boot_mode == BOOT_LAUNCH_FLASH) {
        printf("%s(%d)\n", __FUNCTION__, __LINE__);
        fw_flash_boot();
        printf("%s(%d)\n", __FUNCTION__, __LINE__);
        dnn_flash_boot();
        printf("%s(%d)\n", __FUNCTION__, __LINE__);
    }

    else if (boot_mode == BOOT_UPDATE_FLASH) {
        fw_flash_update();
        dnn_flash_update();
    }

    else if (boot_mode == BOOT_LAUNCH_SPI) {
        fw_spi_boot();
        dnn_spi_boot();
    }

    /* fimware反初始化 */
    // fw_uninit();

    // ------------------------ 读写IMX500模型和寄存器
    /* 使能视频检查输出 */
    stream_start();

    /* 销毁spi master，然后初始化spi slave，他们共同使用SPI2 */
    spi_master_dev_destroy();
    spi_slave_dev_init();
}

//----------------------------------------------- main -------------------------------------------------

void app_start_printf(void) {

    printf(" __  __  ____  _____  _____ \n");
    printf("|  \\/  |/ ___||  _  ||  _  |\n");
    printf("| .  . |\\___ \\| | | || | | |\n");
    printf("| |\\/| | ___) | |_| || |_| |\n");
    printf("|_|  |_||____/ \\___/  \\___/ \n");

    as_core_version();
}
void app_common_device(void) {
    device_ctx = calloc(1, sizeof(device_ctx_t));
    if (device_ctx == NULL) {
        // 处理内存分配失败的情况
        ESP_LOGE("BSP_CONFIG", "Failed to allocate memory for uvc_t");
    }

    bsp_init_sd_card(device_ctx);
    bsp_spiff_init();

    init_storage_system();
}
void app_update_ota(void) {

    int status = bsp_sd_ota_check();
    if (status == 1) {
        ESP_LOGI(TAG, "OTA update successful");

        delay_ms(100);
        bsp_deinit_sd_card(device_ctx);

        bsp_restart_print();
    }
}

void app_update_dnn() {

    int status = bsp_sd_dnn_check();

    // status = 3; // 强制更新模型

    if (status == 1 || status == 2 || status == 3) {

        if (status == 2) {
            // 如果模型不对，无线循环
            clear_model_update_pending();
            ESP_LOGI(TAG, "AI Model clean nvs flag");
        }

        boot_mode = BOOT_UPDATE_FLASH;

        app_cam_device_init();

        ai_update_i2c_master_init();

        imx501_main_init();

        if (status == 1) {
            remove_dnn_dir();
            ESP_LOGI(TAG, "AI Model file deleted");
        }

        ESP_LOGI(TAG, "AI Model update successful");

        delay_ms(100);
        bsp_deinit_sd_card(device_ctx);

        bsp_restart_print();
    }
}

void app_boot_launch_flash(void) {

    if (boot_mode != BOOT_LAUNCH_FLASH) {

        return;
    }

    app_task_init();

    app_cam_device_init();

    video_cam_init();

    // mipi copy  I2c
    ai_mipi_i2c_copy_sccb_handle();

    imx501_main_init();

    ai_mipi_again_init();

    ai_spi_receive_task_start();
}

void app_main(void) {

    app_start_printf();

    app_common_device();

    // 更新固件
    app_update_ota();
    // 更新AI模型
    app_update_dnn();

    // 启动imx500
    app_boot_launch_flash();
}
