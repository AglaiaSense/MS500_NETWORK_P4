#ifndef AS_TOOLS_H
#define AS_TOOLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    int x;
    int y;
    int w;
    int h;
    int image_count;          // 图像计数
    float score;              // 置信度
    char plate[16];           // 车牌字符
    char software_version[4]; // 软件版本
    char ai_model_version[4]; // AI模型版本
} Plate_result_t;
#pragma pack(pop)

typedef struct {
    uint16_t addr;
    size_t val;
    size_t size;
} imx501_reg_t;

typedef struct {
    uint16_t reg;
    uint8_t val;
} imx500_reginfo_t;

//----------------------------------------------- plath -------------------------------------------------

extern Plate_result_t plate_result; //  存储车牌检测结果

extern bool is_plate_detection; // 车牌检测标志

extern int dnn_model_index;

extern char license_plate[16]; // 存储车牌字符

//----------------------------------------------- function -------------------------------------------------

void as_core_version(void) ;

imx500_reginfo_t *get_imx501_27m_1920x1080_crop_30fps_config(void);

#endif
