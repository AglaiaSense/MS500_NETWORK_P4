#include "bsp_http.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "bsp_http";

#define HTTP_TASK_QUEUE_SIZE 10       // 队列次数
#define HTTP_TASK_STACK_SIZE (1024 * 4)   // 队列栈大小
#define HTTP_ASYNC_TASK_STACK_SIZE (1024 * 6)  // 异步任务栈大小

#define HTTP_CINFIG_BUFFER_SIZE (1024 * 2) // 单次response数据大小
#define HTTP_RESPONSE_SIZE (1024 * 10)     // 总响应数据大小

// 全局管理
static QueueHandle_t http_task_queue = NULL;
static TaskHandle_t http_task_handle = NULL;
static bool http_initialized = false;
static uint32_t next_request_id = 1;           // 请求ID计数器
static SemaphoreHandle_t id_mutex = NULL;      // ID生成互斥锁

// HTTP任务处理函数
static void http_task(void *pvParameters);

// 日志输出函数声明
static void log_http_request_response_context(bsp_http_request_context_t *context);

//------------------ HTTP初始化功能 ------------------

// HTTP客户端初始化
void bsp_http_init(void) {
    if (http_initialized) {
        ESP_LOGW(TAG, "HTTP already initialized");
        return;
    }

    ESP_LOGI(TAG, "Initializing HTTP client...");

    // 创建ID生成互斥锁
    id_mutex = xSemaphoreCreateMutex();
    if (id_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create ID mutex");
        return;
    }

    // 创建HTTP任务队列
    http_task_queue = xQueueCreate(HTTP_TASK_QUEUE_SIZE, sizeof(bsp_http_request_context_t *));
    if (http_task_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create HTTP task queue");
        vSemaphoreDelete(id_mutex);
        return;
    }

    // 创建HTTP处理任务
    if (xTaskCreate(http_task, "http_task", HTTP_TASK_STACK_SIZE, NULL, 5, &http_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTP task");
        vQueueDelete(http_task_queue);
        vSemaphoreDelete(id_mutex);
        http_task_queue = NULL;
        id_mutex = NULL;
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
                response->response_data = malloc(HTTP_RESPONSE_SIZE);
                if (response->response_data == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate response buffer");
                    return ESP_FAIL;
                }
                response->response_len = 0;
            }

            // 检查缓冲区空间
            if (response->response_len + evt->data_len < HTTP_RESPONSE_SIZE - 1) {
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

// 新增函数：打印HTTP请求和响应信息 (已弃用，使用log_http_request_response_context)
// static void log_http_request_response(bsp_http_request_t *request, bsp_http_response_t *response) {
//     printf("-----------------http_request_response--------------------\n");
//     ESP_LOGI(TAG, "Request URL: %s", request->url);
//     ESP_LOGI(TAG, "Request Method: %d", request->method);
//     ESP_LOGI(TAG, "Response Status: %d", response->status_code);
//     ESP_LOGI(TAG, "Response Data Length: %d", response->response_len);
//     if (response->response_data && response->response_len > 0) {
//         ESP_LOGI(TAG, "Response Data: %.*s", response->response_len, response->response_data);
//     }
//     printf(" ======================================================= \n");
// }

// 执行HTTP请求的核心函数（适用于上下文结构）
static esp_err_t perform_http_request_context(bsp_http_request_context_t *context) {
    ESP_LOGI(TAG, "Performing HTTP request: %s", context->url);

    // 设置响应状态
    context->response->status = BSP_HTTP_STATUS_IN_PROGRESS;

    esp_err_t err = ESP_OK;

    // HTTP客户端配置
    esp_http_client_config_t config = {
        .url = context->url,
        .event_handler = http_event_handler,
        .user_data = context->response,
        .timeout_ms = context->timeout_ms > 0 ? context->timeout_ms : 5000,
        .buffer_size = HTTP_CINFIG_BUFFER_SIZE,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        context->response->status = BSP_HTTP_STATUS_ERROR;
        context->response->error_code = ESP_FAIL;
        return ESP_FAIL;
    }

    // 设置HTTP方法
    switch (context->method) {
    case BSP_HTTP_METHOD_GET:
        esp_http_client_set_method(client, HTTP_METHOD_GET);
        break;
    case BSP_HTTP_METHOD_POST:
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        if (context->post_data && context->post_data_len > 0) {
            esp_http_client_set_post_field(client, context->post_data, context->post_data_len);
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
    if (context->headers) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
    }

    // 执行请求
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        context->response->status_code = esp_http_client_get_status_code(client);
        context->response->status = BSP_HTTP_STATUS_COMPLETED;
        context->response->error_code = ESP_OK;

        log_http_request_response_context(context);

        // 调用回调函数处理响应
        if (context->callback) {
            context->callback(context->response, context);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        context->response->status = BSP_HTTP_STATUS_ERROR;
        context->response->error_code = err;
    }

    // 清理资源
    esp_http_client_cleanup(client);

    return err;
}

// HTTP任务处理函数
static void http_task(void *pvParameters) {
    bsp_http_request_context_t *context;

    ESP_LOGI(TAG, "HTTP task started");

    while (1) {
        // 等待队列中的请求
        if (xQueueReceive(http_task_queue, &context, portMAX_DELAY) == pdTRUE) {
            if (context != NULL) {
                // 执行HTTP请求（同步）
                perform_http_request_context(context);

                // 释放上下文内存（仅对同步请求）
                if (!context->is_async) {
                    bsp_http_free_context(context);
                }
                // 异步请求的上下文由调用者管理
            }
        }
    }
}

//------------------ 公共API函数 ------------------

// 执行HTTP GET请求（同步）
esp_err_t bsp_http_get(const char *url, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for GET request");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    (*context)->method = BSP_HTTP_METHOD_GET;
    (*context)->callback = callback;
    (*context)->timeout_ms = 5000;
    (*context)->is_async = false; // 同步请求

    // 将请求加入队列
    if (xQueueSend(http_task_queue, context, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue GET request");
        bsp_http_free_context(*context);
        *context = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

// 执行HTTP POST请求（同步）
esp_err_t bsp_http_post(const char *url, const char *post_data, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for POST request");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    (*context)->method = BSP_HTTP_METHOD_POST;
    (*context)->callback = callback;
    (*context)->timeout_ms = 5000;
    (*context)->is_async = false; // 同步请求

    if (post_data) {
        (*context)->post_data = strdup(post_data);
        (*context)->post_data_len = strlen(post_data);
    }

    // 将请求加入队列
    if (xQueueSend(http_task_queue, context, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue POST request");
        bsp_http_free_context(*context);
        *context = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

// 执行通用HTTP请求（同步）
esp_err_t bsp_http_request(bsp_http_request_context_t *context) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (context == NULL || context->url == NULL || context->callback == NULL) {
        ESP_LOGE(TAG, "Invalid request parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // 设置为同步请求
    context->is_async = false;

    // 将请求加入队列
    if (xQueueSend(http_task_queue, &context, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue request");
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

//------------------ 异步功能实现 ------------------

// 生成唯一请求ID
static uint32_t generate_request_id(void) {
    uint32_t id = 0;
    if (xSemaphoreTake(id_mutex, portMAX_DELAY) == pdTRUE) {
        id = next_request_id++;
        xSemaphoreGive(id_mutex);
    }
    return id;
}

// 创建请求上下文
bsp_http_request_context_t* bsp_http_create_context(void) {
    bsp_http_request_context_t *context = calloc(1, sizeof(bsp_http_request_context_t));
    if (context == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context memory");
        return NULL;
    }

    // 初始化上下文
    context->request_id = generate_request_id();
    context->complete_sem = xSemaphoreCreateBinary();
    context->response = calloc(1, sizeof(bsp_http_response_t));
    context->is_async = true;
    
    if (context->complete_sem == NULL || context->response == NULL) {
        ESP_LOGE(TAG, "Failed to create context resources");
        bsp_http_free_context(context);
        return NULL;
    }

    context->response->status = BSP_HTTP_STATUS_PENDING;
    ESP_LOGD(TAG, "Created context with ID: %lu", context->request_id);
    
    return context;
}

// 释放请求上下文
void bsp_http_free_context(bsp_http_request_context_t *context) {
    if (context == NULL) return;

    // 清理字符串
    if (context->url) free(context->url);
    if (context->post_data) free(context->post_data);
    if (context->headers) free(context->headers);
    
    // 清理响应数据
    if (context->response) {
        bsp_http_response_cleanup(context->response);
        free(context->response);
    }
    
    // 清理同步原语
    if (context->complete_sem) {
        vSemaphoreDelete(context->complete_sem);
    }

    ESP_LOGD(TAG, "Freed context with ID: %lu", context->request_id);
    free(context);
}

// 异步HTTP任务处理函数
static void http_async_task(void *pvParameters) {
    bsp_http_request_context_t *context = (bsp_http_request_context_t *)pvParameters;
    if (context == NULL) {
        ESP_LOGE(TAG, "Async task received NULL context");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Starting async HTTP request ID: %lu, URL: %s", 
             context->request_id, context->url);

    context->response->status = BSP_HTTP_STATUS_IN_PROGRESS;

    // 执行HTTP请求 (复用统一的处理函数)
    esp_err_t err = perform_http_request_context(context);

    // 通知请求完成
    xSemaphoreGive(context->complete_sem);
    
    ESP_LOGI(TAG, "Async HTTP request ID %lu completed with status: %d", 
             context->request_id, context->response->status);
    
    // 任务结束，不释放context(由调用者负责)
    vTaskDelete(NULL);
}

// 日志输出函数(针对异步上下文)
static void log_http_request_response_context(bsp_http_request_context_t *context) {
    printf("-----------------response[ID:%lu]--------------------\n", context->request_id);
    ESP_LOGI(TAG, "Request URL: %s", context->url);
    ESP_LOGI(TAG, "Request Method: %d", context->method);
    ESP_LOGI(TAG, "Response Status: %d", context->response->status_code);
    ESP_LOGI(TAG, "Response Data Length: %d", context->response->response_len);
    if (context->response->response_data && context->response->response_len > 0) {
        ESP_LOGI(TAG, "Response Data: %.*s", context->response->response_len, context->response->response_data);
    }
    printf(" ======================================================= \n");
}

//------------------ 异步API函数 ------------------

// 异步GET请求
esp_err_t bsp_http_get_async(const char *url, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for async GET request");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    (*context)->method = BSP_HTTP_METHOD_GET;
    (*context)->callback = callback;
    (*context)->timeout_ms = 5000;

    return bsp_http_request_async(*context);
}

// 异步POST请求  
esp_err_t bsp_http_post_async(const char *url, const char *post_data, bsp_http_callback_t callback, bsp_http_request_context_t **context) {
    if (!http_initialized) {
        ESP_LOGE(TAG, "HTTP not initialized");
        return ESP_FAIL;
    }

    if (url == NULL || callback == NULL || context == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for async POST request");
        return ESP_ERR_INVALID_ARG;
    }

    // 创建上下文
    *context = bsp_http_create_context();
    if (*context == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 设置参数
    (*context)->url = strdup(url);
    (*context)->method = BSP_HTTP_METHOD_POST;
    (*context)->callback = callback;
    (*context)->timeout_ms = 5000;

    if (post_data) {
        (*context)->post_data = strdup(post_data);
        (*context)->post_data_len = strlen(post_data);
    }

    return bsp_http_request_async(*context);
}

// 执行异步请求
esp_err_t bsp_http_request_async(bsp_http_request_context_t *context) {
    if (context == NULL) {
        ESP_LOGE(TAG, "Invalid context for async request");
        return ESP_ERR_INVALID_ARG;
    }

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

// 等待请求完成
esp_err_t bsp_http_wait_completion(bsp_http_request_context_t *context, uint32_t timeout_ms) {
    if (context == NULL || context->complete_sem == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xSemaphoreTake(context->complete_sem, timeout_ticks) == pdTRUE) {
        return ESP_OK;  // 完成
    } else {
        return ESP_ERR_TIMEOUT;  // 超时
    }
}

// 取消请求
esp_err_t bsp_http_cancel_request(bsp_http_request_context_t *context) {
    if (context == NULL || context->task_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 删除任务
    vTaskDelete(context->task_handle);
    context->task_handle = NULL;
    context->response->status = BSP_HTTP_STATUS_ERROR;
    context->response->error_code = ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "Cancelled async HTTP request ID: %lu", context->request_id);
    return ESP_OK;
}

// 获取请求状态
bsp_http_status_t bsp_http_get_status(bsp_http_request_context_t *context) {
    if (context == NULL || context->response == NULL) {
        return BSP_HTTP_STATUS_ERROR;
    }
    return context->response->status;
}