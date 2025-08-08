// 同步HTTP测试 - 专门测试同步HTTP功能
// 此文件仅包含同步HTTP API测试

#include "bsp_http_sync.h"
#include "bsp_http.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "bsp_http_test_sync";

//------------------ 同步HTTP测试 ------------------

// 同步HTTP GET测试 - 简单GET请求
static void test_sync_http_get_basic(void) {
    ESP_LOGI(TAG, "Starting SYNC basic GET test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "SYNC GET Test [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/get";
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_sync_get(test_url, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        // 等待请求完成（10秒超时）
        esp_err_t wait_err = bsp_http_sync_wait_completion(context, 10000);
        if (wait_err == ESP_OK) {
            ESP_LOGI(TAG, "SYNC Basic GET test completed successfully");
        } else {
            ESP_LOGW(TAG, "SYNC Basic GET test timed out or failed: %s", esp_err_to_name(wait_err));
        }
        
        // 释放上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to send SYNC GET request: %s", esp_err_to_name(err));
    }
}

// 同步HTTP POST测试 - 发送JSON数据
static void test_sync_http_post_json(void) {
    ESP_LOGI(TAG, "Starting SYNC POST JSON test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "SYNC POST JSON [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/post";
    const char *json_data = "{\"device\":\"ESP32-P4\",\"message\":\"Hello from BSP HTTP SYNC\",\"timestamp\":123456789}";
    
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_sync_post(test_url, json_data, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        // 等待请求完成（10秒超时）
        esp_err_t wait_err = bsp_http_sync_wait_completion(context, 10000);
        if (wait_err == ESP_OK) {
            ESP_LOGI(TAG, "SYNC POST JSON test completed successfully");
        } else {
            ESP_LOGW(TAG, "SYNC POST JSON test timed out or failed: %s", esp_err_to_name(wait_err));
        }
        
        // 释放上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to send SYNC POST request: %s", esp_err_to_name(err));
    }
}

// 同步HTTP自定义请求测试
static void test_sync_custom_request(void) {
    ESP_LOGI(TAG, "Starting SYNC custom request test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "SYNC Custom Request [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    bsp_http_request_context_t *context = bsp_http_create_context();
    if (context != NULL) {
        context->url = strdup("http://httpbin.org/headers");
        if (context->url == NULL) {
            ESP_LOGE(TAG, "Failed to allocate URL for SYNC custom request");
            bsp_http_free_context(context);
            return;
        }
        
        context->method = BSP_HTTP_METHOD_GET;
        context->post_data = NULL;
        context->post_data_len = 0;
        context->timeout_ms = 10000;
        context->callback = inline_callback;
        
        esp_err_t err = bsp_http_sync_request(context);
        
        if (err == ESP_OK) {
            // 等待请求完成（10秒超时）
            esp_err_t wait_err = bsp_http_sync_wait_completion(context, 10000);
            if (wait_err == ESP_OK) {
                ESP_LOGI(TAG, "SYNC Custom request test completed successfully");
            } else {
                ESP_LOGW(TAG, "SYNC Custom request test timed out or failed: %s", esp_err_to_name(wait_err));
            }
        } else {
            ESP_LOGE(TAG, "Failed to send SYNC custom request: %s", esp_err_to_name(err));
        }
        
        // 释放上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to create context for SYNC custom request");
    }
}

//------------------ 主测试函数 ------------------

// 同步HTTP测试主函数
void bsp_http_test_sync(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting BSP HTTP SYNC Test Suite");
    ESP_LOGI(TAG, "========================================");
    
    // 初始化HTTP核心
    bsp_http_init();
    
    ESP_LOGI(TAG, "\n--- SYNC HTTP Tests ---");
    // 同步测试序列
    test_sync_http_get_basic();
    
    test_sync_http_post_json();
    
    test_sync_custom_request();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BSP HTTP SYNC Test Suite Completed");
    ESP_LOGI(TAG, "All sync tests use dedicated queue/task architecture");
    ESP_LOGI(TAG, "No async interference - clean sync execution");
    ESP_LOGI(TAG, "========================================");
}