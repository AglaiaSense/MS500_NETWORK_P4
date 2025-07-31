#include "vc_uvc_cam.h"
#include "vc_config.h"
#include "vc_video_v4l2.h"

#include "esp_timer.h"

static const char *TAG = "VC_UVC_CAM";

static size_t s_cap_buf_len[BUFFER_COUNT];
static size_t s_m2m_cap_buf_len;

extern esp_err_t store_jpg_to_sd_card(const uint8_t *data, size_t len);

extern uvc_model_t current_model;


bool is_vodeo_started = false; // 全局变量，用于标记视频是否已启动

static esp_err_t video_start_cb(uvc_format_t uvc_format, int width, int height, int rate, void *cb_ctx) {
    ESP_LOGI(TAG, "UVC start"); // 记录UVC启动日志

    if (is_vodeo_started) {
        return ESP_OK;
    }
    is_vodeo_started = true;

    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文

    // 使用统一的视频启动函数
    esp_err_t ret = video_v4l2_start(uvc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start video");
        return ret;
    }

    return ESP_OK;
}

static void video_stop_cb(void *cb_ctx) {
    ESP_LOGI(TAG, "------------------UVC stop"); // 记录UVC停止日志

    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文

    // 使用统一的视频停止函数
    video_v4l2_stop(uvc);

    is_vodeo_started = false;
}

static uvc_fb_t *video_fb_get_cb(void *cb_ctx) {
    ESP_LOGD(TAG, "UVC get callback");

    device_ctx_t *uvc = (device_ctx_t *)cb_ctx;
    
   
    return video_get_uvc_cam(cb_ctx);
}

static void video_fb_return_cb(uvc_fb_t *fb, void *cb_ctx) {
    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文

    ESP_LOGD(TAG, "------------------UVC return"); // 记录帧缓冲区返回日志

    // 使用统一的帧缓冲区返回函数
    video_v4l2_fb_return(uvc);
}

// 初始化 UVC 设备

void vc_uvc_cam_init(device_ctx_t *device_ctx) {

    int index = 0;
    size_t uvc_buffer_size = UVC_FRAMES_INFO[index][0].width * UVC_FRAMES_INFO[index][0].height;

    // uint8_t *uvc_buffer = (uint8_t *)malloc(uvc_buffer_size);
    uint8_t *uvc_buffer = (uint8_t *)heap_caps_malloc(uvc_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (uvc_buffer == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer fail");
    }

    uvc_device_config_t config = {
        .uvc_buffer = uvc_buffer,
        .uvc_buffer_size = uvc_buffer_size, // 使用统一计算的大小
        .start_cb = video_start_cb,
        .fb_get_cb = video_fb_get_cb,
        .fb_return_cb = video_fb_return_cb,
        .stop_cb = video_stop_cb,
        .cb_ctx = (void *)device_ctx,
    };

    ESP_ERROR_CHECK(uvc_device_config(index, &config));
}

void vc_uvc_cam_deinit(device_ctx_t *uvc) {
    int type;
    struct v4l2_requestbuffers req = {0};

    ESP_LOGI(TAG, "BSP UVC Deinit ----------------------------------------- ");

    // 1) 停掉所有流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (uvc->cap_fd >= 0)
        ioctl(uvc->cap_fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (uvc->m2m_fd >= 0)
        ioctl(uvc->m2m_fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (uvc->m2m_fd >= 0)
        ioctl(uvc->m2m_fd, VIDIOC_STREAMOFF, &type);

    // 2) 释放内核端 buffer
    req.count = 0;
    req.memory = V4L2_MEMORY_MMAP;
    if (uvc->cap_fd >= 0) {
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(uvc->cap_fd, VIDIOC_REQBUFS, &req);
    }
    if (uvc->m2m_fd >= 0) {
        req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(uvc->m2m_fd, VIDIOC_REQBUFS, &req);
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(uvc->m2m_fd, VIDIOC_REQBUFS, &req);
    }

    // 3) munmap 应用层映射
    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (uvc->cap_buffer[i] && s_cap_buf_len[i] > 0) {
            munmap(uvc->cap_buffer[i], s_cap_buf_len[i]);
            uvc->cap_buffer[i] = NULL;
            s_cap_buf_len[i] = 0;
        }
    }
    if (uvc->m2m_cap_buffer && s_m2m_cap_buf_len > 0) {
        munmap(uvc->m2m_cap_buffer, s_m2m_cap_buf_len);
        uvc->m2m_cap_buffer = NULL;
        s_m2m_cap_buf_len = 0;
    }

    ESP_LOGI(TAG, "BSP UVC Deinit complete");
}