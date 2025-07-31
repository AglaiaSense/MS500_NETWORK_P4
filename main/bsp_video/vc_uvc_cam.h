#ifndef BSP_UVC_CAM_H
#define BSP_UVC_CAM_H

#include "vc_config.h"

#include "esp_err.h"
#include "esp_log.h"

void vc_uvc_cam_init(device_ctx_t *device_ctx);
void vc_uvc_cam_deinit(device_ctx_t *uvc);

#endif // BSP_UVC_CAM_H