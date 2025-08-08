#ifndef BSP_HTTP_H
#define BSP_HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_http_client.h"

// HTTP请求类型
typedef enum {
    BSP_HTTP_METHOD_GET = 0,
    BSP_HTTP_METHOD_POST,
    BSP_HTTP_METHOD_PUT,
    BSP_HTTP_METHOD_DELETE
} bsp_http_method_t;

// HTTP响应结构
typedef struct {
    int status_code;
    char *response_data;
    int response_len;
} bsp_http_response_t;

// HTTP请求回调函数类型
typedef void (*bsp_http_callback_t)(bsp_http_response_t *response);

// HTTP请求配置结构
typedef struct {
    char *url;
    bsp_http_method_t method;
    char *post_data;
    int post_data_len;
    char *headers;
    int timeout_ms;
    bsp_http_callback_t callback;
} bsp_http_request_t;

// HTTP客户端初始化
void bsp_http_init(void);

// 执行HTTP GET请求
esp_err_t bsp_http_get(const char *url, bsp_http_callback_t callback);

// 执行HTTP POST请求
esp_err_t bsp_http_post(const char *url, const char *post_data, bsp_http_callback_t callback);

// 执行通用HTTP请求
esp_err_t bsp_http_request(bsp_http_request_t *request);

// 清理HTTP响应数据
void bsp_http_response_cleanup(bsp_http_response_t *response);


#endif // BSP_HTTP_H