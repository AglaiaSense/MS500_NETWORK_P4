// 同步HTTP API实现 - 调用bsp_http.c中的核心功能
#include "bsp_http_sync.h"
#include "bsp_http.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "bsp_http_sync";

//------------------ 同步HTTP实现 ------------------

// 同步HTTP GET请求
esp_err_t bsp_http_sync_get(const char *url, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!bsp_http_is_initialized()) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for sync GET request");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting sync GET request to: %s", url);

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    if ((*context)->url == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for URL");
        bsp_http_free_context(*context);
        *context = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    (*context)->method = BSP_HTTP_METHOD_GET;
    (*context)->callback = callback;
    (*context)->timeout_ms = 5000;
    (*context)->is_async = false; // 关键：标记为同步请求

    // 将请求加入同步队列
    esp_err_t err = bsp_http_queue_sync_request(*context);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue sync GET request");
        bsp_http_free_context(*context);
        *context = NULL;
        return err;
    }

    ESP_LOGI(TAG, "Sync GET request queued successfully, ID: %lu", (*context)->request_id);
    return ESP_OK;
}

// 同步HTTP POST请求
esp_err_t bsp_http_sync_post(const char *url, const char *post_data, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!bsp_http_is_initialized()) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for sync POST request");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting sync POST request to: %s", url);

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    if ((*context)->url == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for URL");
        bsp_http_free_context(*context);
        *context = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    (*context)->method = BSP_HTTP_METHOD_POST;
    (*context)->callback = callback;
    (*context)->timeout_ms = 5000;
    (*context)->is_async = false; // 关键：标记为同步请求

    if (post_data) {
        (*context)->post_data = strdup(post_data);
        if ((*context)->post_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for POST data");
            bsp_http_free_context(*context);
            *context = NULL;
            return ESP_ERR_NO_MEM;
        }
        (*context)->post_data_len = strlen(post_data);
    }

    // 将请求加入同步队列
    esp_err_t err = bsp_http_queue_sync_request(*context);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue sync POST request");
        bsp_http_free_context(*context);
        *context = NULL;
        return err;
    }

    ESP_LOGI(TAG, "Sync POST request queued successfully, ID: %lu", (*context)->request_id);
    return ESP_OK;
}

// 同步HTTP通用请求
esp_err_t bsp_http_sync_request(bsp_http_request_context_t *context) {
    if (!bsp_http_is_initialized()) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (context == NULL || context->url == NULL || context->callback == NULL) {
        ESP_LOGE(TAG, "Invalid sync request parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting sync custom request to: %s", context->url);

    // 确保标记为同步请求
    context->is_async = false;

    // 将请求加入同步队列
    esp_err_t err = bsp_http_queue_sync_request(context);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue sync custom request");
        return err;
    }

    ESP_LOGI(TAG, "Sync custom request queued successfully, ID: %lu", context->request_id);
    return ESP_OK;
}

// 等待请求完成（同步专用版本，带更好的日志）
esp_err_t bsp_http_sync_wait_completion(bsp_http_request_context_t *context, uint32_t timeout_ms) {
    if (context == NULL) {
        ESP_LOGE(TAG, "Context is NULL in sync wait_completion");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (context->complete_sem == NULL) {
        ESP_LOGE(TAG, "Completion semaphore is NULL for sync request ID: %lu", context->request_id);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (context->response == NULL) {
        ESP_LOGE(TAG, "Response is NULL for sync request ID: %lu", context->request_id);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Waiting for sync request ID: %lu completion (timeout: %lu ms)", 
             context->request_id, timeout_ms);

    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    // 尝试获取信号量
    BaseType_t take_result = xSemaphoreTake(context->complete_sem, timeout_ticks);
    
    if (take_result == pdTRUE) {
        ESP_LOGI(TAG, "Sync request ID %lu completed, status: %d", 
                 context->request_id, context->response->status);
        
        // 检查请求是否成功完成
        if (context->response->status == BSP_HTTP_STATUS_COMPLETED) {
            ESP_LOGI(TAG, "Sync request ID %lu completed successfully", context->request_id);
            return ESP_OK;  // 成功完成
        } else if (context->response->status == BSP_HTTP_STATUS_ERROR) {
            ESP_LOGW(TAG, "Sync request ID %lu completed with error: %s", 
                     context->request_id, esp_err_to_name(context->response->error_code));
            return context->response->error_code;  // 返回具体错误码
        } else {
            ESP_LOGW(TAG, "Sync request ID %lu completed with unexpected status: %d", 
                     context->request_id, context->response->status);
            return ESP_FAIL;  // 状态异常
        }
    } else {
        ESP_LOGW(TAG, "Sync request ID %lu timed out after %lu ms", 
                 context->request_id, timeout_ms);
        return ESP_ERR_TIMEOUT;  // 超时
    }
}