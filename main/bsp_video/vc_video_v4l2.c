#include "vc_video_v4l2.h"
#include "vc_video_draw.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>

extern esp_err_t store_jpg_to_sd_card(const uint8_t *data, size_t len);
extern uvc_model_t current_model;
extern bool is_plate_detection;

static const char *TAG = "VC_VIDEO_V4L2";

static video_state_t g_video_state = VIDEO_STATE_STOPPED;

/**
 * @brief 检查设备是否正在流传输
 */
static int is_streaming(int fd) {
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == 0) {
        if (fmt.fmt.pix.width != 0 && fmt.fmt.pix.height != 0) {
            return 1;
        }
    } else {
        ESP_LOGE(TAG, "Failed to get format, errno=%d", errno);
    }

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == 0) {
        return 0;
    } else if (errno == EBUSY) {
        return 1;
    }

    return 0;
}

esp_err_t video_v4l2_start(device_ctx_t *sd) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_buffer buf;
    struct v4l2_format format;
    struct v4l2_requestbuffers req;
    struct v4l2_format init_format;
    uint32_t capture_fmt = 0;
    uint32_t width, height;

    // ESP_LOGI(TAG, "Video start");

    // if (g_video_state != VIDEO_STATE_STOPPED) {
    //     ESP_LOGI(TAG, "Video already started or starting");
    //     return ESP_FAIL;
    // }

    g_video_state = VIDEO_STATE_STARTING;

    memset(&init_format, 0, sizeof(struct v4l2_format));
    init_format.type = type;
    if (ioctl(sd->cap_fd, VIDIOC_G_FMT, &init_format) != 0) {
        ESP_LOGE(TAG, "failed to get format");
        g_video_state = VIDEO_STATE_STOPPED;
        return ESP_FAIL;
    }

    width = init_format.fmt.pix.width;
    height = init_format.fmt.pix.height;

    if (sd->format == V4L2_PIX_FMT_JPEG) {
        int fmt_index = 0;
        const uint32_t jpeg_input_formats[] = {
            V4L2_PIX_FMT_RGB565,
            V4L2_PIX_FMT_YUV422P,
            V4L2_PIX_FMT_RGB24,
            V4L2_PIX_FMT_GREY};
        int jpeg_input_formats_num = sizeof(jpeg_input_formats) / sizeof(jpeg_input_formats[0]);

        while (!capture_fmt) {
            struct v4l2_fmtdesc fmtdesc = {
                .index = fmt_index++,
                .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            };

            if (ioctl(sd->cap_fd, VIDIOC_ENUM_FMT, &fmtdesc) != 0) {
                break;
            }

            for (int i = 0; i < jpeg_input_formats_num; i++) {
                if (jpeg_input_formats[i] == fmtdesc.pixelformat) {
                    capture_fmt = jpeg_input_formats[i];
                    break;
                }
            }
        }
        
#if USE_RGB_24
        capture_fmt = V4L2_PIX_FMT_RGB24;
#endif

        if (!capture_fmt) {
            ESP_LOGI(TAG, "The camera sensor output pixel format is not supported by JPEG encoder");
            g_video_state = VIDEO_STATE_STOPPED;
            return ESP_ERR_NOT_SUPPORTED;
        }
    } else if (sd->format == V4L2_PIX_FMT_H264) {
        capture_fmt = V4L2_PIX_FMT_YUV420;
    }

    /* Configure camera interface capture stream */
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = capture_fmt;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_S_FMT, &format));

    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_REQBUFS, &req));

    for (int i = 0; i < BUFFER_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QUERYBUF, &buf));

        sd->cap_buffer[i] = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                            MAP_SHARED, sd->cap_fd, buf.m.offset);
        assert(sd->cap_buffer[i]);

        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QBUF, &buf));
    }

    /* Configure codec output stream */
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = capture_fmt;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_S_FMT, &format));

    memset(&req, 0, sizeof(req));
    req.count = SKIP_STARTUP_FRAME_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    req.memory = V4L2_MEMORY_USERPTR;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_REQBUFS, &req));

    /* Configure codec capture stream */
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = sd->format;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_S_FMT, &format));

    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_REQBUFS, &req));

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QUERYBUF, &buf));

    sd->m2m_cap_buffer = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, sd->m2m_fd, buf.m.offset);
    assert(sd->m2m_cap_buffer);

    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QBUF, &buf));

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_STREAMON, &type));
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_STREAMON, &type));

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_STREAMON, &type));

    /* Skip the first few frames of the image to get a stable image. */
    for (int i = 0; i < SKIP_STARTUP_FRAME_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_DQBUF, &buf));
        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QBUF, &buf));
    }

    /* Init frame buffer's basic info. */
    sd->sd_fb.width = width;
    sd->sd_fb.height = height;
    sd->sd_fb.fmt = sd->format == V4L2_PIX_FMT_JPEG ? SD_IMG_FORMAT_JPEG : SD_IMG_FORMAT_H264;

    g_video_state = VIDEO_STATE_RUNNING;
    return ESP_OK;
}

void video_v4l2_stop(device_ctx_t *sd) {
    int type;

    // ESP_LOGI(TAG, "Video stop");

    // if (g_video_state != VIDEO_STATE_RUNNING) {
    //     ESP_LOGI(TAG, "Video not running");
    //     return;
    // }

    g_video_state = VIDEO_STATE_STOPPING;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(sd->cap_fd, VIDIOC_STREAMOFF, &type);

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ioctl(sd->m2m_fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(sd->m2m_fd, VIDIOC_STREAMOFF, &type);

    g_video_state = VIDEO_STATE_STOPPED;

}

void video_v4l2_fb_return(device_ctx_t *sd) {
    struct v4l2_buffer m2m_cap_buf;

    ESP_LOGD(TAG, "Video return");

    // if (g_video_state != VIDEO_STATE_RUNNING) {
    //     ESP_LOGW(TAG, "Video not running, cannot return frame buffer");
    //     return;
    // }

    m2m_cap_buf.index = 0;
    m2m_cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QBUF, &m2m_cap_buf));
}

sd_card_fb_t *video_get_save_jpg(device_ctx_t *sd) {
    struct v4l2_buffer cap_buf;
    struct v4l2_buffer m2m_out_buf;
    struct v4l2_buffer m2m_cap_buf;
    int64_t us;

    ESP_LOGD(TAG, "Video get");

    // if (g_video_state != VIDEO_STATE_RUNNING) {
    //     ESP_LOGW(TAG, "Video not running, cannot get frame buffer");
    //     return NULL;
    // }

    memset(&cap_buf, 0, sizeof(cap_buf));
    cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_DQBUF, &cap_buf));

    memset(&m2m_out_buf, 0, sizeof(m2m_out_buf));
    m2m_out_buf.index = 0;
    m2m_out_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    m2m_out_buf.memory = V4L2_MEMORY_USERPTR;
    m2m_out_buf.m.userptr = (unsigned long)sd->cap_buffer[cap_buf.index];
    m2m_out_buf.length = cap_buf.bytesused;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QBUF, &m2m_out_buf));

    memset(&m2m_cap_buf, 0, sizeof(m2m_cap_buf));
    m2m_cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_DQBUF, &m2m_cap_buf));

    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QBUF, &cap_buf));
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_DQBUF, &m2m_out_buf));

    sd->sd_fb.buf = sd->m2m_cap_buffer;
    sd->sd_fb.buf_bytesused = m2m_cap_buf.bytesused;
    us = esp_timer_get_time();
    sd->sd_fb.timestamp.tv_sec = us / 1000000UL;
    sd->sd_fb.timestamp.tv_usec = us % 1000000UL;

    return &sd->sd_fb;
}

// UVC帧缓冲区获取回调函数
uvc_fb_t *video_get_uvc_cam(void *cb_ctx) {
    int64_t us;
    device_ctx_t *uvc = (device_ctx_t *)cb_ctx;
    struct v4l2_format format;
    struct v4l2_buffer cap_buf;
    struct v4l2_buffer m2m_out_buf;
    struct v4l2_buffer m2m_cap_buf;

    ESP_LOGD(TAG, "UVC get");

    // 从摄像头捕获队列中获取一个缓冲区
    memset(&cap_buf, 0, sizeof(cap_buf));
    cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_DQBUF, &cap_buf));

    // 根据current_model条件性绘制图像
    video_draw_image_conditionally(uvc->cap_buffer[cap_buf.index]);

    // 配置编码器输出缓冲区
    memset(&m2m_out_buf, 0, sizeof(m2m_out_buf));
    m2m_out_buf.index = 0;
    m2m_out_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    m2m_out_buf.memory = V4L2_MEMORY_USERPTR;
    m2m_out_buf.m.userptr = (unsigned long)uvc->cap_buffer[cap_buf.index];
    m2m_out_buf.length = cap_buf.bytesused;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_QBUF, &m2m_out_buf));

    // 从编码器捕获队列中获取一个缓冲区
    memset(&m2m_cap_buf, 0, sizeof(m2m_cap_buf));
    m2m_cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_DQBUF, &m2m_cap_buf));

    // 将摄像头捕获缓冲区重新加入队列
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_QBUF, &cap_buf));
    // 将编码器输出缓冲区重新加入队列
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_DQBUF, &m2m_out_buf));

    // 获取当前视频格式
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_G_FMT, &format));

    // 填充帧缓冲区信息
    uvc->fb.buf = uvc->m2m_cap_buffer;
    uvc->fb.len = m2m_cap_buf.bytesused;
    uvc->fb.width = format.fmt.pix.width;
    uvc->fb.height = format.fmt.pix.height;
    uvc->fb.format = format.fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG ? UVC_FORMAT_JPEG : UVC_FORMAT_H264;

    // 获取当前时间戳
    us = esp_timer_get_time();
    uvc->fb.timestamp.tv_sec = us / 1000000UL;
    uvc->fb.timestamp.tv_usec = us % 1000000UL;

    if (is_plate_detection == true && current_model == UVC_MODEL_ALL) {
        store_jpg_to_sd_card(uvc->fb.buf, uvc->fb.len);
    }

    return &uvc->fb;
}

video_state_t video_v4l2_get_state(device_ctx_t *device_ctx) {
    return g_video_state;
}