#ifndef BSP_INFERENCE_H
#define BSP_INFERENCE_H

#include <stdint.h>

/**
 * @brief 解码DNN输出数据
 * @param data 输入数据指针
 * @return 解码结果状态码 (0表示成功)
 */
int decode_dnn_output(const void *data);

#endif // BSP_INFERENCE_H
