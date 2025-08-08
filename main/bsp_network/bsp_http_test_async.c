// 异步HTTP测试 - 专门测试异步HTTP功能
// 此文件仅包含异步HTTP API测试

#include "bsp_http_async.h"
#include "bsp_http.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "bsp_http_test_async";

//------------------ 异步HTTP测试 ------------------

// 异步HTTP GET测试 - 简单GET请求
static void test_async_http_get_basic(void) {
    ESP_LOGI(TAG, "Starting ASYNC basic GET test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "ASYNC GET Test [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/get";
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_async_get(test_url, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        // 等待请求完成（10秒超时）
        esp_err_t wait_err = bsp_http_async_wait_completion(context, 10000);
        if (wait_err == ESP_OK) {
            ESP_LOGI(TAG, "ASYNC Basic GET test completed successfully");
        } else {
            ESP_LOGW(TAG, "ASYNC Basic GET test timed out or failed: %s", esp_err_to_name(wait_err));
        }
        
        // 释放上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to send ASYNC GET request: %s", esp_err_to_name(err));
    }
}

// 异步HTTP POST测试 - 发送JSON数据
static void test_async_http_post_json(void) {
    ESP_LOGI(TAG, "Starting ASYNC POST JSON test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "ASYNC POST JSON [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/post";
    const char *json_data = "{\"device\":\"ESP32-P4\",\"message\":\"Hello from BSP HTTP ASYNC\",\"timestamp\":123456789}";
    
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_async_post(test_url, json_data, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        // 等待请求完成（10秒超时）
        esp_err_t wait_err = bsp_http_async_wait_completion(context, 10000);
        if (wait_err == ESP_OK) {
            ESP_LOGI(TAG, "ASYNC POST JSON test completed successfully");
        } else {
            ESP_LOGW(TAG, "ASYNC POST JSON test timed out or failed: %s", esp_err_to_name(wait_err));
        }
        
        // 释放上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to send ASYNC POST request: %s", esp_err_to_name(err));
    }
}
 
// 并发异步测试
static void test_concurrent_async(void) {
    ESP_LOGI(TAG, "Starting concurrent ASYNC test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Concurrent ASYNC [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
 
    const char *urls[] = {
        "http://httpbin.org/get?test=async1",
        "http://httpbin.org/get?test=async2", 
        "http://httpbin.org/get?test=async3",
        "http://httpbin.org/get?test=async4",
        
    };


      

    
    bsp_http_request_context_t *contexts[4] = {NULL};
    
    // 并发提交多个异步请求
    for (int i = 0; i < 4; i++) {
        esp_err_t err = bsp_http_async_get(urls[i], inline_callback, &contexts[i]);
        if (err == ESP_OK && contexts[i] != NULL) {
            ESP_LOGI(TAG, "Concurrent ASYNC request %d submitted, ID: %lu", i+1, contexts[i]->request_id);
        } else {
            ESP_LOGE(TAG, "Failed to submit concurrent ASYNC request %d", i+1);
        }
        
        // 短暂延迟避免同时创建太多任务
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "All concurrent ASYNC requests submitted, waiting for completion...");
    
    // 等待所有请求完成
    for (int i = 0; i < 4; i++) {
        if (contexts[i] != NULL) {
            esp_err_t wait_err = bsp_http_async_wait_completion(contexts[i], 15000);
            if (wait_err == ESP_OK) {
                ESP_LOGI(TAG, "Concurrent ASYNC request %d (ID: %lu) completed", i+1, contexts[i]->request_id);
            } else {
                ESP_LOGW(TAG, "Concurrent ASYNC request %d (ID: %lu) failed: %s", i+1, contexts[i]->request_id, esp_err_to_name(wait_err));
            }
            
            // 释放上下文
            bsp_http_free_context(contexts[i]);
        }
    }
}

// 异步自定义请求测试
static void test_async_custom_request(void) {
    ESP_LOGI(TAG, "Starting ASYNC custom request test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "ASYNC Custom Request [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
    }
    
    bsp_http_request_context_t *context = bsp_http_create_context();
    if (context != NULL) {
        context->url = strdup("http://httpbin.org/headers");
        if (context->url == NULL) {
            ESP_LOGE(TAG, "Failed to allocate URL for ASYNC custom request");
            bsp_http_free_context(context);
            return;
        }
        
        context->method = BSP_HTTP_METHOD_GET;
        context->post_data = NULL;
        context->post_data_len = 0;
        context->timeout_ms = 10000;
        context->callback = inline_callback;
        
        esp_err_t err = bsp_http_async_request(context);
        
        if (err == ESP_OK) {
            // 等待请求完成（10秒超时）
            esp_err_t wait_err = bsp_http_async_wait_completion(context, 10000);
            if (wait_err == ESP_OK) {
                ESP_LOGI(TAG, "ASYNC Custom request test completed successfully");
            } else {
                ESP_LOGW(TAG, "ASYNC Custom request test timed out or failed: %s", esp_err_to_name(wait_err));
            }
        } else {
            ESP_LOGE(TAG, "Failed to send ASYNC custom request: %s", esp_err_to_name(err));
        }
        
        // 释放上下文
        bsp_http_free_context(context);
    } else {
        ESP_LOGE(TAG, "Failed to create context for ASYNC custom request");
    }
}

//------------------ 主测试函数 ------------------

// 异步HTTP测试主函数
void bsp_http_test_async(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting BSP HTTP ASYNC Test Suite");
    ESP_LOGI(TAG, "========================================");
    
    // 初始化HTTP核心
    bsp_http_init();
    
    ESP_LOGI(TAG, "\n--- ASYNC HTTP Tests ---");
    // 异步测试序列 
    test_async_http_get_basic();
    
    test_async_http_post_json();
    
    test_async_custom_request();
    
    
    test_concurrent_async();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BSP HTTP ASYNC Test Suite Completed");
    ESP_LOGI(TAG, "All async tests use individual task architecture");
    ESP_LOGI(TAG, "No sync interference - clean async execution");
    ESP_LOGI(TAG, "========================================");
}