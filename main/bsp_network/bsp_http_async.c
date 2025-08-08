// 异步HTTP API实现 - 调用bsp_http.c中的核心功能
#include "bsp_http_async.h"
#include "bsp_http.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "bsp_http_async";

#define HTTP_ASYNC_TASK_STACK_SIZE (1024 * 6)  // 异步任务栈大小

//------------------ 异步任务实现 ------------------

// 异步HTTP任务处理函数
static void http_async_task(void *pvParameters) {
    bsp_http_request_context_t *context = (bsp_http_request_context_t *)pvParameters;
    if (context == NULL) {
        ESP_LOGE(TAG, "Async task received NULL context");
        vTaskDelete(NULL);
        return;
    }

    // 关键修复：任务开始前喂狗（检查任务是否已注册）
    if (esp_task_wdt_status(NULL) == ESP_OK) {
        esp_task_wdt_reset();
    }

    ESP_LOGI(TAG, "Starting async HTTP request ID: %lu, URL: %s", 
             context->request_id, context->url);

    context->response->status = BSP_HTTP_STATUS_IN_PROGRESS;
    
    ESP_LOGI(TAG, "Async task [ID:%lu] - About to perform HTTP request...", context->request_id);

    // 执行HTTP请求 (使用核心处理函数)
    esp_err_t err = bsp_http_perform_request(context);
    
    ESP_LOGI(TAG, "Async task [ID:%lu] - HTTP request processing completed", context->request_id);
    
    // 关键修复：请求完成后喂狗（检查任务是否已注册）
    if (esp_task_wdt_status(NULL) == ESP_OK) {
        esp_task_wdt_reset();
    }
    
    ESP_LOGI(TAG, "Async HTTP request ID %lu completed with status: %d", 
             context->request_id, context->response->status);
    
    // 检查是否需要自动清理（Fire-and-Forget模式）
    if (context->auto_cleanup) {
        ESP_LOGI(TAG, "Auto-cleaning up context for request ID: %lu", context->request_id);
        bsp_http_free_context(context);
    }
    
    // 任务结束
    vTaskDelete(NULL);
}

//------------------ 异步HTTP API实现 ------------------

// 异步GET请求
esp_err_t bsp_http_async_get(const char *url, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!bsp_http_is_initialized()) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for async GET request");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting async GET request to: %s", url);

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    if ((*context)->url == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for URL in async GET");
        bsp_http_free_context(*context);
        *context = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    (*context)->method = BSP_HTTP_METHOD_GET;
    (*context)->callback = callback;
    (*context)->timeout_ms = 3000;  // 减少超时时间防止长时间阻塞
    (*context)->is_async = true; // 关键：标记为异步请求

    return bsp_http_async_request(*context);
}

// 异步POST请求  
esp_err_t bsp_http_async_post(const char *url, const char *post_data, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!bsp_http_is_initialized()) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for async POST request");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting async POST request to: %s", url);

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    if ((*context)->url == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for URL in async POST");
        bsp_http_free_context(*context);
        *context = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    (*context)->method = BSP_HTTP_METHOD_POST;
    (*context)->callback = callback;
    (*context)->timeout_ms = 3000;  // 减少超时时间防止长时间阻塞
    (*context)->is_async = true; // 关键：标记为异步请求

    if (post_data) {
        (*context)->post_data = strdup(post_data);
        if ((*context)->post_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for POST data in async POST");
            bsp_http_free_context(*context);
            *context = NULL;
            return ESP_ERR_NO_MEM;
        }
        (*context)->post_data_len = strlen(post_data);
    }

    return bsp_http_async_request(*context);
}

// 执行异步请求
esp_err_t bsp_http_async_request(bsp_http_request_context_t *context) {
    if (context == NULL) {
        ESP_LOGE(TAG, "Invalid context for async request");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Creating async task for request ID: %lu", context->request_id);

    // 创建异步任务
    char task_name[32];
    snprintf(task_name, sizeof(task_name), "http_async_%lu", context->request_id);
    
    BaseType_t result = xTaskCreate(
        http_async_task,
        task_name,
        HTTP_ASYNC_TASK_STACK_SIZE,
        (void *)context,
        5,  // 任务优先级
        &context->task_handle
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create async HTTP task for ID: %lu", context->request_id);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Created async HTTP task for request ID: %lu", context->request_id);
    return ESP_OK;
}

// 等待异步请求完成
esp_err_t bsp_http_async_wait_completion(bsp_http_request_context_t *context, uint32_t timeout_ms) {
    if (context == NULL) {
        ESP_LOGE(TAG, "Context is NULL in async wait_completion");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (context->complete_sem == NULL) {
        ESP_LOGE(TAG, "Completion semaphore is NULL for async request ID: %lu", context->request_id);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (context->response == NULL) {
        ESP_LOGE(TAG, "Response is NULL for async request ID: %lu", context->request_id);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Waiting for async request ID: %lu completion (timeout: %lu ms)", 
             context->request_id, timeout_ms);

    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    // 关键修复：在长时间等待前喂狗（检查任务是否已注册）
    if (esp_task_wdt_status(NULL) == ESP_OK) {
        esp_task_wdt_reset();
    }
    
    // 修复：分段等待，避免长时间阻塞看门狗
    BaseType_t take_result = pdFALSE;
    TickType_t remaining_ticks = timeout_ticks;
    const TickType_t wdt_feed_interval = pdMS_TO_TICKS(1000); // 每秒喂一次狗
    
    while (remaining_ticks > 0) {
        TickType_t wait_time = (remaining_ticks > wdt_feed_interval) ? wdt_feed_interval : remaining_ticks;
        
        take_result = xSemaphoreTake(context->complete_sem, wait_time);
        if (take_result == pdTRUE) {
            break; // 成功获取信号量，退出循环
        }
        
        // 喂狗并更新剩余时间（检查任务是否已注册）
        if (esp_task_wdt_status(NULL) == ESP_OK) {
            esp_task_wdt_reset();
        }
        if (remaining_ticks != portMAX_DELAY) {
            remaining_ticks -= wait_time;
        }
    }
    
    if (take_result == pdTRUE) {
        ESP_LOGI(TAG, "Async request ID %lu completed, status: %d", 
                 context->request_id, context->response->status);
        
        // 检查请求是否成功完成
        if (context->response->status == BSP_HTTP_STATUS_COMPLETED) {
            ESP_LOGI(TAG, "Async request ID %lu completed successfully", context->request_id);
            return ESP_OK;  // 成功完成
        } else if (context->response->status == BSP_HTTP_STATUS_ERROR) {
            ESP_LOGW(TAG, "Async request ID %lu completed with error: %s", 
                     context->request_id, esp_err_to_name(context->response->error_code));
            return context->response->error_code;  // 返回具体错误码
        } else {
            ESP_LOGW(TAG, "Async request ID %lu completed with unexpected status: %d", 
                     context->request_id, context->response->status);
            return ESP_FAIL;  // 状态异常
        }
    } else {
        ESP_LOGW(TAG, "Async request ID %lu timed out after %lu ms", 
                 context->request_id, timeout_ms);
        return ESP_ERR_TIMEOUT;  // 超时
    }
}
 