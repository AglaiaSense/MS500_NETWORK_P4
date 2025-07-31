#include "vc_save_jpg.h"
#include "esp_timer.h"
#include "vc_uvc_jpg.h"
#include "vc_video_v4l2.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static const char *TAG = "VC_SAVE_JPG";

extern esp_err_t store_jpg_to_sd_card(const uint8_t *data, size_t len);
extern uvc_model_t current_model;
int is_streaming(int fd) {
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // 先尝试获取格式
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == 0) {
        // 格式已设置且宽高有效
        if (fmt.fmt.pix.width != 0 && fmt.fmt.pix.height != 0) {
            return 1;
        }
    } else {
        // 获取格式失败，记录错误
        ESP_LOGE(TAG, "Failed to get format, errno=%d", errno);
    }

    // 尝试设置格式来判断流状态
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == 0) {
        return 0; // 设置成功说明不在流状态
    } else if (errno == EBUSY) {
        return 1; // 设备忙表示正在流传输
    }

    return 0;
}

void vc_save_jpg_init(device_ctx_t *video) {
    // printf("vc_save_jpg_init start\n");

    sd_card_fb_t *image_fb = NULL;
    esp_err_t ret = ESP_OK;

    video_state_t current_state = video_v4l2_get_state(video);

    ret = video_v4l2_start(video);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start video");
        return;
    }
 
    image_fb = video_get_save_jpg(video);
 
    store_jpg_to_sd_card(image_fb->buf, image_fb->buf_bytesused);
    video_v4l2_fb_return(video);
    video_v4l2_stop(video);
}
