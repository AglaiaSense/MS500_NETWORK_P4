#ifndef VC_VIDEO_V4L2_H
#define VC_VIDEO_V4L2_H

#include "vc_config.h"
#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 视频状态枚举
 */
typedef enum {
    VIDEO_STATE_STOPPED = 0,
    VIDEO_STATE_STARTING,
    VIDEO_STATE_RUNNING,
    VIDEO_STATE_STOPPING
} video_state_t;

/**
 * @brief 启动视频流
 * @param device_ctx 设备上下文指针
 * @return esp_err_t 错误码
 */
esp_err_t video_v4l2_start(device_ctx_t *device_ctx);

/**
 * @brief 停止视频流
 * @param device_ctx 设备上下文指针
 */
void video_v4l2_stop(device_ctx_t *device_ctx);

/**
 * @brief 返回帧缓冲区
 * @param device_ctx 设备上下文指针
 */
void video_v4l2_fb_return(device_ctx_t *device_ctx);

/**
 * @brief 获取帧缓冲区
 * @param device_ctx 设备上下文指针
 * @return sd_card_fb_t* 帧缓冲区指针
 */
sd_card_fb_t *video_get_save_jpg(device_ctx_t *device_ctx);

/**
 * @brief 获取UVC帧缓冲区回调函数
 * @param cb_ctx 回调上下文指针
 * @return uvc_fb_t* UVC帧缓冲区指针
 */
uvc_fb_t *video_get_uvc_cam(void *cb_ctx);

/**
 * @brief 获取当前视频状态
 * @param device_ctx 设备上下文指针
 * @return video_state_t 视频状态
 */
video_state_t video_v4l2_get_state(device_ctx_t *device_ctx);



#ifdef __cplusplus
}
#endif

#endif // VC_VIDEO_V4L2_H