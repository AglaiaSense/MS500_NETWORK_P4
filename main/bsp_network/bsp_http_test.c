#include "bsp_http.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "bsp_http_test";

//------------------ HTTP测试回调函数 ------------------

// GET请求测试回调
static void http_get_test_callback(bsp_http_response_t *response, void *user_data) {
    const char *test_name = (const char *)user_data;
    
    ESP_LOGI(TAG, "=== %s GET Test Result ===", test_name);
    ESP_LOGI(TAG, "Status Code: %d", response->status_code);
    ESP_LOGI(TAG, "Content Length: %d", response->content_length);
    ESP_LOGI(TAG, "Response Length: %d", response->response_len);
    
    if (response->response_data && response->response_len > 0) {
        // 限制显示的响应内容长度
        int display_len = response->response_len > 200 ? 200 : response->response_len;
        ESP_LOGI(TAG, "Response Data (first %d bytes): %.*s", display_len, display_len, response->response_data);
        
        if (response->response_len > 200) {
            ESP_LOGI(TAG, "... (response truncated for display)");
        }
    } else {
        ESP_LOGW(TAG, "No response data received");
    }
    
    ESP_LOGI(TAG, "================================");
}

// POST请求测试回调
static void http_post_test_callback(bsp_http_response_t *response, void *user_data) {
    const char *test_name = (const char *)user_data;
    
    ESP_LOGI(TAG, "=== %s POST Test Result ===", test_name);
    ESP_LOGI(TAG, "Status Code: %d", response->status_code);
    ESP_LOGI(TAG, "Content Length: %d", response->content_length);
    ESP_LOGI(TAG, "Response Length: %d", response->response_len);
    
    if (response->response_data && response->response_len > 0) {
        int display_len = response->response_len > 300 ? 300 : response->response_len;
        ESP_LOGI(TAG, "Response Data (first %d bytes): %.*s", display_len, display_len, response->response_data);
        
        if (response->response_len > 300) {
            ESP_LOGI(TAG, "... (response truncated for display)");
        }
    } else {
        ESP_LOGW(TAG, "No response data received");
    }
    
    ESP_LOGI(TAG, "================================");
}

// 天气API测试回调
static void weather_api_callback(bsp_http_response_t *response, void *user_data) {
    ESP_LOGI(TAG, "=== Weather API Test Result ===");
    ESP_LOGI(TAG, "Status Code: %d", response->status_code);
    
    if (response->status_code == 200 && response->response_data) {
        ESP_LOGI(TAG, "Weather Data: %.*s", response->response_len, response->response_data);
    } else {
        ESP_LOGW(TAG, "Weather API request failed or no data");
    }
    
    ESP_LOGI(TAG, "==============================");
}

//------------------ HTTP测试函数 ------------------

// HTTP GET测试 - 简单GET请求
static void test_http_get_basic(void) {
    ESP_LOGI(TAG, "Starting basic GET test...");
    
    const char *test_url = "http://httpbin.org/get";
    esp_err_t err = bsp_http_get(test_url, http_get_test_callback, (void *)"Basic");
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send GET request: %s", esp_err_to_name(err));
    }
}

// HTTP GET测试 - 获取JSON数据
static void test_http_get_json(void) {
    ESP_LOGI(TAG, "Starting JSON GET test...");
    
    // 内联回调函数处理JSON响应
    void inline_json_callback(bsp_http_response_t *response, void *user_data) {
        ESP_LOGI(TAG, "=== JSON GET Test Result ===");
        ESP_LOGI(TAG, "Status Code: %d", response->status_code);
        ESP_LOGI(TAG, "Content Length: %d", response->content_length);
        ESP_LOGI(TAG, "Response Length: %d", response->response_len);
        
        if (response->response_data && response->response_len > 0) {
            // 显示JSON数据，限制显示长度
            int display_len = response->response_len > 300 ? 300 : response->response_len;
            ESP_LOGI(TAG, "JSON Response Data (first %d bytes):", display_len);
            ESP_LOGI(TAG, "%.*s", display_len, response->response_data);
            
            if (response->response_len > 300) {
                ESP_LOGI(TAG, "... (JSON response truncated for display)");
            }
            
            // 简单的JSON解析示例 - 查找特定字段
            char *slideshow_pos = strstr(response->response_data, "\"slideshow\"");
            if (slideshow_pos) {
                ESP_LOGI(TAG, "Found 'slideshow' field in JSON response");
            }
            
            char *title_pos = strstr(response->response_data, "\"title\"");
            if (title_pos) {
                ESP_LOGI(TAG, "Found 'title' field in JSON response");
            }
            
        } else {
            ESP_LOGW(TAG, "No JSON response data received");
        }
        
        ESP_LOGI(TAG, "=============================");
    }
    
    const char *test_url = "http://httpbin.org/json";
    esp_err_t err = bsp_http_get(test_url, inline_json_callback, NULL);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send JSON GET request: %s", esp_err_to_name(err));
    }
}

// HTTP POST测试 - 发送JSON数据
static void test_http_post_json(void) {
    ESP_LOGI(TAG, "Starting POST JSON test...");
    
    const char *test_url = "http://httpbin.org/post";
    const char *json_data = "{\"device\":\"ESP32-P4\",\"message\":\"Hello from BSP HTTP\",\"timestamp\":123456789}";
    
    esp_err_t err = bsp_http_post(test_url, json_data, http_post_test_callback, (void *)"JSON");
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send POST request: %s", esp_err_to_name(err));
    }
}

// HTTP POST测试 - 发送表单数据
static void test_http_post_form(void) {
    ESP_LOGI(TAG, "Starting POST form test...");
    
    const char *test_url = "http://httpbin.org/post";
    const char *form_data = "name=ESP32-P4&version=1.0&test=form_data";
    
    esp_err_t err = bsp_http_post(test_url, form_data, http_post_test_callback, (void *)"Form");
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send POST form request: %s", esp_err_to_name(err));
    }
}

// 天气API测试（使用免费的天气服务）
static void test_weather_api(void) {
    ESP_LOGI(TAG, "Starting Weather API test...");
    
    // 使用免费的天气API（不需要API密钥）
    const char *weather_url = "http://api.openweathermap.org/data/2.5/weather?q=Beijing&appid=demo&units=metric";
    
    esp_err_t err = bsp_http_get(weather_url, weather_api_callback, NULL);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send weather API request: %s", esp_err_to_name(err));
    }
}

// 自定义请求测试
static void test_custom_request(void) {
    ESP_LOGI(TAG, "Starting custom request test...");
    
    bsp_http_request_t request = {
        .url = "http://httpbin.org/headers",
        .method = BSP_HTTP_METHOD_GET,
        .post_data = NULL,
        .post_data_len = 0,
        .headers = "User-Agent: ESP32-P4-BSP-HTTP/1.0",
        .timeout_ms = 10000,
        .callback = http_get_test_callback,
        .user_data = (void *)"Custom"
    };
    
    esp_err_t err = bsp_http_request(&request);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send custom request: %s", esp_err_to_name(err));
    }
}

//------------------ 主测试函数 ------------------

// HTTP测试主函数
void bsp_http_test(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting BSP HTTP Test Suite");
    ESP_LOGI(TAG, "========================================");
    
    // 测试序列 - 每个测试之间有延迟
    test_http_get_basic();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    test_http_get_json();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    test_http_post_json();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    test_http_post_form();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    test_custom_request();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 注释掉天气API测试，因为可能需要有效的API密钥
    test_weather_api();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BSP HTTP Test Suite Completed");
    ESP_LOGI(TAG, "Check logs above for individual test results");
    ESP_LOGI(TAG, "========================================");
}