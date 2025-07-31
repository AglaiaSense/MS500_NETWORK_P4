#include "bsp_rtc.h"
#include "esp_system.h"

#include <sys/time.h>
#include <time.h>
#include "driver/rtc_io.h"


static const char *TAG = "BSP_RTC";

// -------------------------- 常用方法  ------------------------------------------

// 显示当前时间
void log_local_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char strftime_buf[32];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y/%m/%d, %H:%M:%S", &timeinfo);

    ESP_LOGI(TAG, "now: %s", strftime_buf); 
}

// 检查时间是否为初始时间
bool is_time_1970() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // 检查是否为1970年
    return (timeinfo.tm_year == 70);
}

// --------------------------   计算时差 ------------------------------------------

// 计算到下一个整分钟的微秒数
uint64_t time_to_next_minute() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

   // 计算到下一个整分钟的秒数差
    int seconds_to_next_minute = 60 - timeinfo.tm_sec;
    if (seconds_to_next_minute == 60) {
        seconds_to_next_minute = 0;
    }

    // 计算到下一个整分钟的微秒数
    uint64_t microseconds_to_next_minute = SEC_TO_USEC(seconds_to_next_minute + 1) - tv.tv_usec;
    return microseconds_to_next_minute;
}

// 计算到下一个整小时的微秒数
uint64_t time_to_next_hour() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // 计算到下一个整小时的秒数差
    int seconds_to_next_hour = (60 - timeinfo.tm_min) * 60 - timeinfo.tm_sec;
    if (seconds_to_next_hour == 3600) {
        seconds_to_next_hour = 0;
    }

    // 计算到下一个整小时的微秒数
    uint64_t microseconds_to_next_hour = SEC_TO_USEC(seconds_to_next_hour + 1) - tv.tv_usec;
    return microseconds_to_next_hour;
}
// 计算到下一整天的微秒数
uint64_t time_to_next_day() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // 计算到明天的秒数差
    int seconds_to_next_day = (24 - timeinfo.tm_hour) * 3600 - timeinfo.tm_min * 60 - timeinfo.tm_sec;
    if (seconds_to_next_day == 86400) {
        seconds_to_next_day = 0;
    }

    // 计算到明天的微秒数
    uint64_t microseconds_to_next_day = SEC_TO_USEC(seconds_to_next_day + 1) - tv.tv_usec;
    return microseconds_to_next_day;
}


// -------------------------- 时间同步  ------------------------------------------
//  修改系统时间
void set_time_of_day(const char *time_str) {

    ESP_LOGI(TAG, "sync_time|%s|", time_str); 

    struct tm tm;
    if (strptime(time_str, "%Y/%m/%d,%H:%M:%S", &tm) == NULL) {
        ESP_LOGI(TAG, "Failed to parse time string");
        return;
    }

    time_t t = mktime(&tm);
    struct timeval now = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now, NULL);
}

 