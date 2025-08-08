#ifndef BSP_HTTP_H
#define BSP_HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------ 公共数据结构定义 ------------------

// HTTP请求类型
typedef enum {
    BSP_HTTP_METHOD_GET = 0,
    BSP_HTTP_METHOD_POST,
    BSP_HTTP_METHOD_PUT,
    BSP_HTTP_METHOD_DELETE
} bsp_http_method_t;

// HTTP请求状态
typedef enum {
    BSP_HTTP_STATUS_PENDING = 0,    // 等待处理
    BSP_HTTP_STATUS_IN_PROGRESS,    // 正在处理
    BSP_HTTP_STATUS_COMPLETED,      // 完成
    BSP_HTTP_STATUS_ERROR          // 错误
} bsp_http_status_t;

// HTTP响应结构
typedef struct {
    int status_code;
    char *response_data;
    int response_len;
    bsp_http_status_t status;
    esp_err_t error_code;
} bsp_http_response_t;

// 前向声明
typedef struct bsp_http_request_context bsp_http_request_context_t;

// HTTP请求回调函数类型
typedef void (*bsp_http_callback_t)(bsp_http_response_t *response, bsp_http_request_context_t *context);

// HTTP请求上下文结构（统一同步和异步使用）
typedef struct bsp_http_request_context {
    char *url;
    bsp_http_method_t method;
    char *post_data;
    int post_data_len;
    char *headers;
    int timeout_ms;
    bsp_http_callback_t callback;
    
    // 管理字段
    uint32_t request_id;            // 请求唯一ID
    TaskHandle_t task_handle;       // 任务句柄
    SemaphoreHandle_t complete_sem; // 完成信号量
    bsp_http_response_t *response;  // 响应数据指针
    bool is_async;                  // 是否异步请求
    bool cancelled;                 // 取消标志，用于优雅退出
} bsp_http_request_context_t;

//------------------ 核心管理 API ------------------

// HTTP客户端初始化
void bsp_http_init(void);

// 检查是否已初始化
bool bsp_http_is_initialized(void);

// 上下文管理
bsp_http_request_context_t* bsp_http_create_context(void);
void bsp_http_free_context(bsp_http_request_context_t *context);

// 清理HTTP响应数据
void bsp_http_response_cleanup(bsp_http_response_t *response);

//------------------ 核心处理函数 ------------------

// 执行HTTP请求的核心函数（同步/异步通用）
esp_err_t bsp_http_perform_request(bsp_http_request_context_t *context);

// 同步请求队列管理
esp_err_t bsp_http_queue_sync_request(bsp_http_request_context_t *context);

//------------------ 注意 ------------------
// 如需使用HTTP功能，请包含以下专用头文件：
// - 同步HTTP：#include "bsp_http_sync.h"
// - 异步HTTP：#include "bsp_http_async.h"

#ifdef __cplusplus
}
#endif

#endif // BSP_HTTP_H