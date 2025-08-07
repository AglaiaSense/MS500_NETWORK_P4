#ifndef BSP_LTE_EG25_H
#define BSP_LTE_EG25_H

#include "esp_err.h"

// 网络配置和激活相关函数
void bsp_lte_net_config(void);
void bsp_lte_act_config(void);
void bsp_lte_http_config(void);

// 功耗管理函数
void bsp_lte_set_low_power(void);
void bsp_lte_set_all_power(void);
void bsp_lte_set_close_power(void);

// 网络时间同步函数
void bsp_lte_get_net_time(char *time_buffer);

// HTTP请求函数
esp_err_t bsp_lte_http_get_request(const char *request_data, char *respond_data);
esp_err_t bsp_lte_http_post_request(const char *request_url, const char *request_params, char *respond_data);
esp_err_t bsp_lte_http_put_request(const char *request_url, const char *request_params, char *respond_data);

// 初始化函数
void bsp_lte_eg25_init(void);

#endif // BSP_LTE_EG25_H