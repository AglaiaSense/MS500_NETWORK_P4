#ifndef BSP_HTTP_SYNC_H
#define BSP_HTTP_SYNC_H

#include "bsp_http.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------ 同步HTTP API ------------------

// 同步HTTP GET请求
esp_err_t bsp_http_sync_get(const char *url, bsp_http_callback_t callback, bsp_http_request_context_t **context);

// 同步HTTP POST请求
esp_err_t bsp_http_sync_post(const char *url, const char *post_data, bsp_http_callback_t callback, bsp_http_request_context_t **context);

// 同步HTTP通用请求
esp_err_t bsp_http_sync_request(bsp_http_request_context_t *context);

// 等待请求完成
esp_err_t bsp_http_sync_wait_completion(bsp_http_request_context_t *context, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // BSP_HTTP_SYNC_H