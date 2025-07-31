/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef OUTPUT_TENSOR_PARSER_H_
#define OUTPUT_TENSOR_PARSER_H_

#include <stdint.h>
#include <stddef.h>

#define bytes_to_u16(MSB, LSB) (((unsigned int)((unsigned char)MSB)) & 255) << 8 | (((unsigned char)LSB) & 255)
#define extract_byte(MSB, LSB) (((((unsigned char)MSB) << 6) & 0xc0) | ((((unsigned char)LSB) & 0xfc) >> 2))

#ifdef __cplusplus
extern "C" {
#endif

extern int decode_dnn_output(const void *data);

int process_dnn_buffer(uint8_t *dnn_data, size_t dnn_size);



#ifdef __cplusplus
}

#endif

#endif // TENSOR_PARSER_H_
