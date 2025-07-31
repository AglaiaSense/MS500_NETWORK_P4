#ifndef BSP_STORAGE_H
#define BSP_STORAGE_H

#include "vc_config.h"
#include "bsp_sd_card.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 存储JPG图片到SD卡
 * @param sd 设备上下文指针
 * @return esp_err_t 返回操作结果
 */
esp_err_t store_jpg_to_sd_card(const uint8_t *data, size_t len) ;

esp_err_t init_storage_system() ;

esp_err_t handle_wake_event() ;

#ifdef __cplusplus
}
#endif

#endif // BSP_STORAGE_H
