#ifndef BSP_HTTP_ASYNC_H
#define BSP_HTTP_ASYNC_H

#include "bsp_http.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------ 异步HTTP API ------------------

// 异步HTTP GET请求
esp_err_t bsp_http_async_get(const char *url, bsp_http_callback_t callback, bsp_http_request_context_t **context);

// 异步HTTP POST请求  
esp_err_t bsp_http_async_post(const char *url, const char *post_data, bsp_http_callback_t callback, bsp_http_request_context_t **context);

// 异步HTTP通用请求
esp_err_t bsp_http_async_request(bsp_http_request_context_t *context);

// 异步请求管理
esp_err_t bsp_http_async_wait_completion(bsp_http_request_context_t *context, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // BSP_HTTP_ASYNC_H