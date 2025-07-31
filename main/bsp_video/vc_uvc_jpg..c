/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "vc_uvc_jpg.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <inttypes.h>
#include <pthread.h>

#define WIDTH 1920  // CONFIG_UVC_CAM2_FRAMESIZE_WIDTH
#define HEIGHT 1080 // CONFIG_UVC_CAM2_FRAMESIZE_HEIGT

#define UVC_MAX_FRAMESIZE_SIZE (410 * 1024)

static const char *TAG = "VC_UVC_JPG";
static uvc_fb_t s_fb;

static size_t current_index = 0;
bool is_jpg_reading = false;

extern storage_state_t g_storage_state;

static void camera_stop_cb(void *cb_ctx) {
    (void)cb_ctx;
    ESP_LOGI(TAG, "------------------UVC stop"); // 记录UVC启动日志
}

static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx) {
    (void)cb_ctx;
    ESP_LOGI(TAG, "UVC start"); // 记录UVC启动日志

    current_index = 0;

    return ESP_OK;
}

static uvc_fb_t *camera_fb_get_cb(void *cb_ctx) {

    (void)cb_ctx;

    // 检查是否有图片
    if (g_storage_state.image_paths_size == 0) {
        return NULL;
    }

    // 循环获取图片
    if (current_index >= g_storage_state.image_paths_size) {
        current_index = 0;
    }
    // 添加延迟防止过快
    delay_ms(200);

    const char *file_path = g_storage_state.image_paths[current_index++];

    is_jpg_reading = true;

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        is_jpg_reading = false;

        return NULL;
    }

    uint64_t us = (uint64_t)esp_timer_get_time();

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);


    // 检查上次缓冲区是否已释放
    if (s_fb.buf != NULL) {
        ESP_LOGW(TAG, "Previous buffer not released! Forcing free.");
        free(s_fb.buf);
        s_fb.buf = NULL;
    }

    // 分配内存并读取文件内容
    uint8_t *jpg_data = (uint8_t *)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!jpg_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for image");
        fclose(file);
        is_jpg_reading = false;

        return NULL;
    }

    fread(jpg_data, 1, file_size, file);
    fclose(file);
    is_jpg_reading = false;

    ESP_LOGE(TAG, "file name: %s", file_path);

    s_fb.buf = jpg_data;
    s_fb.len = file_size;
    s_fb.width = WIDTH;
    s_fb.height = HEIGHT;
    s_fb.format = UVC_FORMAT_JPEG;
    s_fb.timestamp.tv_sec = us / 1000000UL;
    s_fb.timestamp.tv_usec = us % 1000000UL;

    return &s_fb;
}

static void camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx) {
    printf("camera_fb_return_cb\n");
    (void)cb_ctx;
    assert(fb == &s_fb);
    if (fb->buf) {
        // printf("Freeing buffer of size %zu\n", fb->len);
        free(fb->buf);
        fb->buf = NULL;
    }
}

void vc_uvc_jpg_init(int cam_index) {

    int index = cam_index;
    printf("vc_uvc_jpg_init %d\n", index);

    size_t uvc_buffer_size = UVC_FRAMES_INFO[index][0].width * UVC_FRAMES_INFO[index][0].height;

    // uint8_t *uvc_buffer = (uint8_t *)malloc(uvc_buffer_size);
    uint8_t *uvc_buffer = (uint8_t *)heap_caps_malloc(uvc_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (uvc_buffer == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer fail");
    }

    uvc_device_config_t config = {
        .uvc_buffer = uvc_buffer,
        .uvc_buffer_size = uvc_buffer_size,
        .start_cb = camera_start_cb,
        .fb_get_cb = camera_fb_get_cb,
        .fb_return_cb = camera_fb_return_cb,
        .stop_cb = camera_stop_cb,
        .cb_ctx = (void *)1,
    };
    ESP_ERROR_CHECK(uvc_device_config(index, &config));
}
