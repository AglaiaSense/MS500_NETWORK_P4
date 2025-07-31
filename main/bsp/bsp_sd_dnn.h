#ifndef BSP_SD_DNN_H
#define BSP_SD_DNN_H

#ifdef __cplusplus
extern "C" {
#endif
#include "esp_err.h"


// 完成更新清理
void remove_dnn_dir();

int bsp_sd_dnn_check();

esp_err_t clear_model_update_pending() ;

#ifdef __cplusplus
}
#endif

#endif // BSP_SD_DNN_H
