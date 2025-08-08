#include "bsp_http.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "bsp_http_test";

//------------------ HTTP测试函数 ------------------

// HTTP GET测试 - 简单GET请求
static void test_http_get_basic(void) {
    ESP_LOGI(TAG, "Starting basic GET test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response) {
        if (response->response_data) {
            ESP_LOGI(TAG, "GET Test Response: %.*s", response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/get";
    esp_err_t err = bsp_http_get(test_url, inline_callback);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send GET request: %s", esp_err_to_name(err));
    }
}

// HTTP GET测试 - 获取JSON数据
static void test_http_get_json(void) {
    ESP_LOGI(TAG, "Starting JSON GET test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response) {
        if (response->response_data) {
            ESP_LOGI(TAG, "JSON Test Response: %.*s", response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/json";
    esp_err_t err = bsp_http_get(test_url, inline_callback);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send JSON GET request: %s", esp_err_to_name(err));
    }
}

// HTTP POST测试 - 发送JSON数据
static void test_http_post_json(void) {
    ESP_LOGI(TAG, "Starting POST JSON test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response) {
        if (response->response_data) {
            ESP_LOGI(TAG, "POST JSON Test Response: %.*s", response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/post";
    const char *json_data = "{\"device\":\"ESP32-P4\",\"message\":\"Hello from BSP HTTP\",\"timestamp\":123456789}";
    
    esp_err_t err = bsp_http_post(test_url, json_data, inline_callback);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send POST request: %s", esp_err_to_name(err));
    }
}

// HTTP POST测试 - 发送表单数据
static void test_http_post_form(void) {
    ESP_LOGI(TAG, "Starting POST form test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response) {
        if (response->response_data) {
            ESP_LOGI(TAG, "POST Form Test Response: %.*s", response->response_len, response->response_data);
        }
    }
    
    const char *test_url = "http://httpbin.org/post";
    const char *form_data = "name=ESP32-P4&version=1.0&test=form_data";
    
    esp_err_t err = bsp_http_post(test_url, form_data, inline_callback);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send POST form request: %s", esp_err_to_name(err));
    }
}

// 天气API测试
static void test_weather_api(void) {
    ESP_LOGI(TAG, "Starting Weather API test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Weather API Response: %.*s", response->response_len, response->response_data);
        }
    }
    
    const char *weather_url = "http://api.openweathermap.org/data/2.5/weather?q=Beijing&appid=demo&units=metric";
    esp_err_t err = bsp_http_get(weather_url, inline_callback);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send weather API request: %s", esp_err_to_name(err));
    }
}

// 自定义请求测试
static void test_custom_request(void) {
    ESP_LOGI(TAG, "Starting custom request test...");
    
    // 内联回调函数
    void inline_callback(bsp_http_response_t *response) {
        if (response->response_data) {
            ESP_LOGI(TAG, "Custom Request Response: %.*s", response->response_len, response->response_data);
        }
    }
    
    bsp_http_request_t request = {
        .url = "http://httpbin.org/headers",
        .method = BSP_HTTP_METHOD_GET,
        .post_data = NULL,
        .post_data_len = 0,
        .headers = "User-Agent: ESP32-P4-BSP-HTTP/1.0",
        .timeout_ms = 10000,
        .callback = inline_callback
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
    
    // 测试序列
    test_http_get_basic();
    test_http_get_json();
    test_http_post_json();
    test_http_post_form();
    test_custom_request();
    test_weather_api();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BSP HTTP Test Suite Completed");
    ESP_LOGI(TAG, "========================================");
}