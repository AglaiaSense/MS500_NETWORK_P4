#ifndef BSP_SPIFF_H
#define BSP_SPIFF_H

#include "esp_spiffs.h"

// 模型文件路径宏定义
#define IMAGE_FILE_NETWK_CSTM "/download/dnn/network.fpk"

void bsp_spiff_init(void);
void bsp_spiff_uninit(void);

void print_all_spiffs_usage(void) ;

void bsp_spiff_format_download(void);

#endif
