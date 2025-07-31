

#ifndef BSP_FILE_OPERATE_H
#define BSP_FILE_OPERATE_H

#include <stdint.h>

// 文件复制回调函数类型
typedef uint8_t (*copy_callback_t)(const char* filename, uint8_t percent, uint8_t mode);

 

// 核心文件复制函数
uint8_t file_copy(copy_callback_t callback, 
                 const char* src_path,
                 const char* dst_path,
                 uint32_t total_size,
                 uint32_t copied_size,
                 uint8_t overwrite);

// 递归复制目录
void recursive_copy(const char* src_base, const char* dst_base);

// 递归删除目录
void recursive_delete(const char* path);

 

 

#endif // BSP_FILE_OPERATE_H
