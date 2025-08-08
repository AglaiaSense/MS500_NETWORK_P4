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

// 异步HTTP GET测试 - 简单GET请求（Fire-and-Forget）
static void test_async_http_get_basic(void) {
    ESP_LOGI(TAG, "Starting ASYNC basic GET test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "ASYNC GET Test [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
        // 标记自动清理
        context->auto_cleanup = true;
    }
    
    const char *test_url = "http://httpbin.org/get?test=basic";
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_async_get(test_url, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "ASYNC Basic GET test submitted, ID: %lu", context->request_id);
    } else {
        ESP_LOGE(TAG, "Failed to send ASYNC GET request: %s", esp_err_to_name(err));
    }
}

// 异步HTTP POST测试 - 发送JSON数据（Fire-and-Forget）
static void test_async_http_post_json(void) {
    ESP_LOGI(TAG, "Starting ASYNC POST JSON test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "ASYNC POST JSON [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
        // 标记自动清理
        context->auto_cleanup = true;
    }
    
    const char *test_url = "http://httpbin.org/post?test=json";
    const char *json_data = "{\"device\":\"ESP32-P4\",\"message\":\"Hello from BSP HTTP ASYNC\",\"timestamp\":123456789}";
    
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_async_post(test_url, json_data, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "ASYNC POST JSON test submitted, ID: %lu", context->request_id);
    } else {
        ESP_LOGE(TAG, "Failed to send ASYNC POST request: %s", esp_err_to_name(err));
    }
}
 
// 并发异步测试（Fire-and-Forget）
static void test_concurrent_async(void) {
    ESP_LOGI(TAG, "Starting concurrent ASYNC test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Concurrent ASYNC [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
        // 标记自动清理
        context->auto_cleanup = true;
    }
    
    const char *urls[] = {
        "http://httpbin.org/get?test=concurrent1",
        "http://httpbin.org/get?test=concurrent2", 
        "http://httpbin.org/get?test=concurrent3",
    };
    
    // 并发提交多个异步请求
    for (int i = 0; i < 3; i++) {
        bsp_http_request_context_t *context = NULL;
        esp_err_t err = bsp_http_async_get(urls[i], inline_callback, &context);
        if (err == ESP_OK && context != NULL) {
            ESP_LOGI(TAG, "Concurrent ASYNC request %d submitted, ID: %lu", i+1, context->request_id);
        } else {
            ESP_LOGE(TAG, "Failed to submit concurrent ASYNC request %d", i+1);
        }
        
        // 短暂延迟避免同时创建太多任务
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGI(TAG, "All concurrent ASYNC requests submitted, running in background");
}

// 异步自定义请求测试（Fire-and-Forget）
static void test_async_custom_request(void) {
    ESP_LOGI(TAG, "Starting ASYNC custom request test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "ASYNC Custom Request [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
        // 标记自动清理
        context->auto_cleanup = true;
    }
    
    bsp_http_request_context_t *context = bsp_http_create_context();
    if (context != NULL) {
        context->url = strdup("http://httpbin.org/headers?test=custom");
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
            ESP_LOGI(TAG, "ASYNC Custom request test submitted, ID: %lu", context->request_id);
        } else {
            ESP_LOGE(TAG, "Failed to send ASYNC custom request: %s", esp_err_to_name(err));
            // 如果提交失败，需要手动释放上下文
            bsp_http_free_context(context);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create context for ASYNC custom request");
    }
}

//------------------ 主测试函数 ------------------

// 真正异步HTTP测试 - 不等待完成，让调用者继续运行
static void test_fire_and_forget_async(void) {
    ESP_LOGI(TAG, "Starting fire-and-forget ASYNC test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response, bsp_http_request_context_t *context) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Fire-and-forget ASYNC [ID:%lu]: %.*s", context->request_id, response->response_len, response->response_data);
        }
        // 标记为需要自动释放，让异步任务处理清理
        context->auto_cleanup = true;
    }
    
    const char *test_url = "http://httpbin.org/get?async=fire_forget";
    bsp_http_request_context_t *context = NULL;
    esp_err_t err = bsp_http_async_get(test_url, inline_callback, &context);
    
    if (err == ESP_OK && context != NULL) {
        ESP_LOGI(TAG, "Fire-and-forget ASYNC request submitted, ID: %lu", context->request_id);
        // 注意：不调用 bsp_http_async_wait_completion()，让请求在后台运行
        // 上下文将在回调函数中释放
    } else {
        ESP_LOGE(TAG, "Failed to send fire-and-forget ASYNC GET request: %s", esp_err_to_name(err));
    }
}

// 异步HTTP测试主函数 - 完整的异步测试套件
void bsp_http_test_async(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting BSP HTTP ASYNC Test Suite");
    ESP_LOGI(TAG, "========================================");
    
    // 初始化HTTP核心
    bsp_http_init();
    
    ESP_LOGI(TAG, "\n--- ASYNC HTTP Tests (Fire-and-Forget) ---");
    
    // 执行所有异步测试，全部使用Fire-and-Forget模式
    test_fire_and_forget_async();
    
    vTaskDelay(pdMS_TO_TICKS(100));  // 短暂延迟避免日志冲突
    test_async_http_get_basic();
    
    vTaskDelay(pdMS_TO_TICKS(100));
    test_async_http_post_json();
    
    vTaskDelay(pdMS_TO_TICKS(100));
    test_async_custom_request();
    
    vTaskDelay(pdMS_TO_TICKS(100));
    test_concurrent_async();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BSP HTTP ASYNC Test Suite Initiated");
    ESP_LOGI(TAG, "All tests running in background - caller can continue");
    ESP_LOGI(TAG, "Fire-and-Forget mode - auto cleanup enabled");
    ESP_LOGI(TAG, "========================================");
}