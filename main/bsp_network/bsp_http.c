#include "bsp_http.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "bsp_http";

// HTTP任务队列大小
#define HTTP_TASK_QUEUE_SIZE 10
#define HTTP_TASK_STACK_SIZE (1024 * 4)
#define HTTP_MAX_RESPONSE_SIZE (1024 * 16)

// HTTP任务队列
static QueueHandle_t http_task_queue = NULL;
static TaskHandle_t http_task_handle = NULL;
static bool http_initialized = false;

// HTTP任务处理函数
static void http_task(void *pvParameters);

//------------------ HTTP初始化功能 ------------------

// HTTP客户端初始化
void bsp_http_init(void) {
    if (http_initialized) {
        ESP_LOGW(TAG, "HTTP already initialized");
        return;
    }

    ESP_LOGI(TAG, "Initializing HTTP client...");
    
    // 创建HTTP任务队列
    http_task_queue = xQueueCreate(HTTP_TASK_QUEUE_SIZE, sizeof(bsp_http_request_t*));
    if (http_task_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create HTTP task queue");
        return;
    }

    // 创建HTTP处理任务
    if (xTaskCreate(http_task, "http_task", HTTP_TASK_STACK_SIZE, NULL, 5, &http_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTP task");
        vQueueDelete(http_task_queue);
        http_task_queue = NULL;
        return;
    }

    http_initialized = true;
    ESP_LOGI(TAG, "HTTP client initialized successfully");
}

//------------------ HTTP事件处理功能 ------------------

// HTTP事件处理回调
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    bsp_http_response_t *response = (bsp_http_response_t *)evt->user_data;
    
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // 分配或扩展响应缓冲区
                if (response->response_data == NULL) {
                    response->response_data = malloc(HTTP_MAX_RESPONSE_SIZE);
                    if (response->response_data == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate response buffer");
                        return ESP_FAIL;
                    }
                    response->response_len = 0;
                }
                
                // 检查缓冲区空间
                if (response->response_len + evt->data_len < HTTP_MAX_RESPONSE_SIZE - 1) {
                    memcpy(response->response_data + response->response_len, evt->data, evt->data_len);
                    response->response_len += evt->data_len;
                    response->response_data[response->response_len] = '\0';
                } else {
                    ESP_LOGW(TAG, "Response buffer full, truncating data");
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            response->is_complete = true;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

//------------------ HTTP请求处理功能 ------------------

// 执行HTTP请求的核心函数
static esp_err_t perform_http_request(bsp_http_request_t *request) {
    ESP_LOGI(TAG, "Performing HTTP request: %s", request->url);
    
    bsp_http_response_t response = {0};
    esp_err_t err = ESP_OK;

    // HTTP客户端配置
    esp_http_client_config_t config = {
        .url = request->url,
        .event_handler = http_event_handler,
        .user_data = &response,
        .timeout_ms = request->timeout_ms > 0 ? request->timeout_ms : 5000,
        .buffer_size = HTTP_MAX_RESPONSE_SIZE,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // 设置HTTP方法
    switch (request->method) {
        case BSP_HTTP_METHOD_GET:
            esp_http_client_set_method(client, HTTP_METHOD_GET);
            break;
        case BSP_HTTP_METHOD_POST:
            esp_http_client_set_method(client, HTTP_METHOD_POST);
            if (request->post_data && request->post_data_len > 0) {
                esp_http_client_set_post_field(client, request->post_data, request->post_data_len);
            }
            break;
        case BSP_HTTP_METHOD_PUT:
            esp_http_client_set_method(client, HTTP_METHOD_PUT);
            break;
        case BSP_HTTP_METHOD_DELETE:
            esp_http_client_set_method(client, HTTP_METHOD_DELETE);
            break;
    }

    // 设置请求头
    if (request->headers) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
    }

    // 执行请求
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        response.status_code = esp_http_client_get_status_code(client);
        response.content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTP request completed - Status: %d, Content-Length: %d", 
                 response.status_code, response.content_length);
        
        // 调用回调函数处理响应
        if (request->callback) {
            request->callback(&response, request->user_data);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    // 清理资源
    esp_http_client_cleanup(client);
    
    // 清理响应数据
    bsp_http_response_cleanup(&response);
    
    return err;
}

// HTTP任务处理函数
static void http_task(void *pvParameters) {
    bsp_http_request_t *request;
    
    ESP_LOGI(TAG, "HTTP task started");
    
    while (1) {
        // 等待队列中的请求
        if (xQueueReceive(http_task_queue, &request, portMAX_DELAY) == pdTRUE) {
            if (request != NULL) {
                // 执行HTTP请求
                perform_http_request(request);
                
                // 释放请求内存
                if (request->url) free(request->url);
                if (request->post_data) free(request->post_data);
                if (request->headers) free(request->headers);
                free(request);
            }
        }
    }
}

//------------------ 公共API函数 ------------------

// 执行HTTP GET请求
esp_err_t bsp_http_get(const char *url, bsp_http_callback_t callback, void *user_data) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for GET request");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建请求结构
    bsp_http_request_t *request = malloc(sizeof(bsp_http_request_t));
    if (request == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for GET request");
        return ESP_ERR_NO_MEM;
    }

    memset(request, 0, sizeof(bsp_http_request_t));
    request->url = strdup(url);
    request->method = BSP_HTTP_METHOD_GET;
    request->callback = callback;
    request->user_data = user_data;
    request->timeout_ms = 5000;

    // 将请求加入队列
    if (xQueueSend(http_task_queue, &request, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue GET request");
        free(request->url);
        free(request);
        return ESP_FAIL;
    }

    return ESP_OK;
}

// 执行HTTP POST请求
esp_err_t bsp_http_post(const char *url, const char *post_data, bsp_http_callback_t callback, void *user_data) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for POST request");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建请求结构
    bsp_http_request_t *request = malloc(sizeof(bsp_http_request_t));
    if (request == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for POST request");
        return ESP_ERR_NO_MEM;
    }

    memset(request, 0, sizeof(bsp_http_request_t));
    request->url = strdup(url);
    request->method = BSP_HTTP_METHOD_POST;
    request->callback = callback;
    request->user_data = user_data;
    request->timeout_ms = 5000;

    if (post_data) {
        request->post_data = strdup(post_data);
        request->post_data_len = strlen(post_data);
    }

    // 将请求加入队列
    if (xQueueSend(http_task_queue, &request, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue POST request");
        free(request->url);
        if (request->post_data) free(request->post_data);
        free(request);
        return ESP_FAIL;
    }

    return ESP_OK;
}

// 执行通用HTTP请求
esp_err_t bsp_http_request(bsp_http_request_t *request) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (request == NULL || request->url == NULL || request->callback == NULL) {
        ESP_LOGE(TAG, "Invalid request parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建请求副本
    bsp_http_request_t *req_copy = malloc(sizeof(bsp_http_request_t));
    if (req_copy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for request");
        return ESP_ERR_NO_MEM;
    }

    memcpy(req_copy, request, sizeof(bsp_http_request_t));
    req_copy->url = strdup(request->url);
    
    if (request->post_data) {
        req_copy->post_data = strdup(request->post_data);
    }
    
    if (request->headers) {
        req_copy->headers = strdup(request->headers);
    }

    // 将请求加入队列
    if (xQueueSend(http_task_queue, &req_copy, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue request");
        free(req_copy->url);
        if (req_copy->post_data) free(req_copy->post_data);
        if (req_copy->headers) free(req_copy->headers);
        free(req_copy);
        return ESP_FAIL;
    }

    return ESP_OK;
}

// 清理HTTP响应数据
void bsp_http_response_cleanup(bsp_http_response_t *response) {
    if (response && response->response_data) {
        free(response->response_data);
        response->response_data = NULL;
        response->response_len = 0;
    }
}