#ifndef BSP_RTC_H
#define BSP_RTC_H

#include "vc_config.h"

// 宏定义将秒转换为微秒
#define SEC_TO_USEC(s) ((s) * 1000000)
// 宏定义将分钟转换为微秒
#define MIN_TO_USEC(m) ((m) * 60 * 1000000)
// 宏定义将小时转换为微秒
#define HOUR_TO_USEC(h) ((h) * 60 * 60 * 1000000)

uint64_t time_to_next_minute();
uint64_t time_to_next_hour();
uint64_t time_to_next_day();

void log_local_time();
bool is_time_1970();

#endif // BSP_RTC_H
