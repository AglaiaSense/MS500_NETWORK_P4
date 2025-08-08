#include "bsp_http.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "bsp_http_async_test";

//------------------ 异步测试函数 ------------------

// 异步GET测试
static void test_async_get(void) {
    ESP_LOGI(TAG, "Starting async GET test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Async GET [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_get_async("http://httpbin.org/get", inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "Async GET request submitted, ID: %lu", context->request_id);
    } else {
        ESP_LOGE(TAG, "Failed to submit async GET request: %s", esp_err_to_name(err));
    }
}

// 异步POST测试
static void test_async_post(void) {
    ESP_LOGI(TAG, "Starting async POST test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Async POST [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    const char *json_data = "{\"device\":\"ESP32-P4\",\"async\":true,\"test\":\"async_post\"}";
    bsp_http_request_context_t *context = NULL;
    
    esp_err_t err = bsp_http_post_async("http://httpbin.org/post", json_data, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "Async POST request submitted, ID: %lu", context->request_id);
    } else {
        ESP_LOGE(TAG, "Failed to submit async POST request: %s", esp_err_to_name(err));
    }
}

// 异步JSON测试
static void test_async_json(void) {
    ESP_LOGI(TAG, "Starting async JSON test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Async JSON [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_get_async("http://httpbin.org/json", inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "Async JSON request submitted, ID: %lu", context->request_id);
    } else {
        ESP_LOGE(TAG, "Failed to submit async JSON request: %s", esp_err_to_name(err));
    }
}

// 带等待的异步测试
static void test_async_with_wait(void) {
    ESP_LOGI(TAG, "Starting async test with wait...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Async Wait [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_get_async("http://httpbin.org/delay/2", inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "Async request submitted, ID: %lu, waiting for completion...", context->request_id);
        
        // 等待完成 (5秒超时)
        esp_err_t wait_err = bsp_http_wait_completion(context, 5000);
        if (wait_err == ESP_OK) {
            ESP_LOGI(TAG, "Request ID %lu completed successfully", context->request_id);
            ESP_LOGI(TAG, "Final status: %d", bsp_http_get_status(context));
        } else if (wait_err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Request ID %lu timed out", context->request_id);
            // 可以选择取消请求
            bsp_http_cancel_request(context);
        }
        
        // 清理上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to submit async wait test: %s", esp_err_to_name(err));
    }
}

// 并发异步测试
static void test_concurrent_async(void) {
    ESP_LOGI(TAG, "Starting concurrent async test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Concurrent [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    const char *urls[] = {
        "http://httpbin.org/get?test=1",
        "http://httpbin.org/get?test=2", 
        "http://httpbin.org/get?test=3",
        "http://httpbin.org/json"
    };
    
    bsp_http_request_context_t *contexts[4] = {NULL};
    
    // 并发提交多个请求
    for (int i = 0; i < 4; i++) {
        esp_err_t err = bsp_http_get_async(urls[i], inline_callback, &contexts[i]);
        if (err == ESP_OK && contexts[i] != NULL) {
            ESP_LOGI(TAG, "Concurrent request %d submitted, ID: %lu", i+1, contexts[i]->request_id);
        } else {
            ESP_LOGE(TAG, "Failed to submit concurrent request %d", i+1);
        }
        
        // 短暂延迟避免同时创建太多任务
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "All concurrent requests submitted, they will complete asynchronously");
    
    // 可选：等待所有请求完成
    ESP_LOGI(TAG, "Waiting for all concurrent requests to complete...");
    for (int i = 0; i < 4; i++) {
        if (contexts[i] != NULL) {
            esp_err_t wait_err = bsp_http_wait_completion(contexts[i], 10000);
            if (wait_err == ESP_OK) {
                ESP_LOGI(TAG, "Concurrent request %d (ID: %lu) completed", i+1, contexts[i]->request_id);
            } else {
                ESP_LOGW(TAG, "Concurrent request %d (ID: %lu) timed out", i+1, contexts[i]->request_id);
            }
            
            // 清理上下文
            bsp_http_free_context(contexts[i]);
        }
    }
}

//------------------ 主测试函数 ------------------

// HTTP异步测试主函数
void bsp_http_async_test(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting BSP HTTP Async Test Suite");
    ESP_LOGI(TAG, "========================================");
    
    // 单独的异步测试（不等待）
    test_async_get();
    
    test_async_post();
    
    test_async_json();
    
    // 带等待的异步测试
    test_async_with_wait();
    // 并发异步测试
    test_concurrent_async();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BSP HTTP Async Test Suite Completed");
    ESP_LOGI(TAG, "Check logs for individual async results");
    ESP_LOGI(TAG, "========================================");
}