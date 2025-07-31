#ifndef VC_VIDEO_DRAW_H
#define VC_VIDEO_DRAW_H

#include "vc_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//  USE_RGB_24定义
#define USE_RGB_24 0

/**
 * @brief 根据current_model条件性绘制图像
 * @param pixels 像素数据指针
 */
void video_draw_image_conditionally(uint8_t *pixels);

#ifdef __cplusplus
}
#endif

#endif // VC_VIDEO_DRAW_H