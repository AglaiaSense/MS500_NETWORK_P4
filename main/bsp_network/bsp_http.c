// HTTP核心实现 - 提供同步和异步HTTP功能的底层支持
// 此文件包含所有HTTP请求处理的核心逻辑

#include "bsp_http.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "bsp_http_core";

#define HTTP_TASK_QUEUE_SIZE 10         // 队列次数
#define HTTP_TASK_STACK_SIZE (1024 * 4) // 队列栈大小

#define HTTP_CINFIG_BUFFER_SIZE (1024 * 2) // 单次response数据大小
#define HTTP_RESPONSE_SIZE (1024 * 10)     // 总响应数据大小

// 全局管理
static QueueHandle_t http_sync_task_queue = NULL;
static TaskHandle_t http_sync_task_handle = NULL;
static bool http_initialized = false;
static uint32_t next_request_id = 1;      // 请求ID计数器
static SemaphoreHandle_t id_mutex = NULL; // ID生成互斥锁

// 同步任务处理函数声明
static void http_sync_task(void *pvParameters);

//------------------ HTTP初始化功能 ------------------

void bsp_http_init(void) {
    if (http_initialized) {
        ESP_LOGW(TAG, "HTTP already initialized");
        return;
    }

    ESP_LOGI(TAG, "Initializing HTTP core...");

    // 创建ID生成互斥锁
    id_mutex = xSemaphoreCreateMutex();
    if (id_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create ID mutex");
        return;
    }

    // 创建同步HTTP任务队列
    http_sync_task_queue = xQueueCreate(HTTP_TASK_QUEUE_SIZE, sizeof(bsp_http_request_context_t *));
    if (http_sync_task_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sync HTTP task queue");
        vSemaphoreDelete(id_mutex);
        return;
    }

    // 创建同步HTTP处理任务
    if (xTaskCreate(http_sync_task, "http_sync_task", HTTP_TASK_STACK_SIZE, NULL, 5, &http_sync_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sync HTTP task");
        vQueueDelete(http_sync_task_queue);
        vSemaphoreDelete(id_mutex);
        http_sync_task_queue = NULL;
        id_mutex = NULL;
        return;
    }

    http_initialized = true;
    ESP_LOGI(TAG, "HTTP core initialized successfully");
}

bool bsp_http_is_initialized(void) {
    return http_initialized;
}

//------------------ HTTP事件处理功能 ------------------

// HTTP事件处理回调
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    bsp_http_response_t *response = (bsp_http_response_t *)evt->user_data;

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED - Connection established");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT - Request headers sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA - Receiving data, len=%d, chunked=%s",
                 evt->data_len, esp_http_client_is_chunked_response(evt->client) ? "yes" : "no");

        // 处理响应数据（包括分块和非分块响应）
        if (evt->data_len > 0) {
            // 关键修复：验证response指针有效性
            if (response == NULL) {
                ESP_LOGE(TAG, "CRITICAL: Response pointer is NULL");
                return ESP_FAIL;
            }

            // 分配或扩展响应缓冲区
            if (response->response_data == NULL) {
                response->response_data = calloc(1, HTTP_RESPONSE_SIZE); // 使用calloc初始化为0
                if (response->response_data == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate response buffer");
                    return ESP_FAIL;
                }
                response->response_len = 0;
                ESP_LOGI(TAG, "Allocated response buffer: %d bytes", HTTP_RESPONSE_SIZE);
            }

            // 关键修复：简化缓冲区边界检查，防止闪退
            if (response->response_len > HTTP_RESPONSE_SIZE || response->response_len < 0) {
                ESP_LOGE(TAG, "CRITICAL: Response buffer length corrupted: %d > %d, abandoning request",
                         response->response_len, HTTP_RESPONSE_SIZE);

                // 安全处理：不释放可能损坏的指针，直接重置避免闪退
                response->response_data = NULL; // 不调用free，避免heap_caps_free失败
                response->response_len = 0;
                return ESP_FAIL; // 返回错误，终止请求处理
            }

            // 额外安全检查：确保数据长度合理
            if (evt->data_len > HTTP_RESPONSE_SIZE || evt->data_len < 0) {
                ESP_LOGE(TAG, "CRITICAL: Invalid data length: %d, abandoning request", evt->data_len);
                return ESP_FAIL;
            }

            // 关键修复：安全的边界检查和数据拷贝，防止闪退
            if (response->response_len + evt->data_len < HTTP_RESPONSE_SIZE &&
                evt->data != NULL && evt->data_len > 0 && response->response_data != NULL) {

                // 直接拷贝数据，简化检查
                memcpy(response->response_data + response->response_len, evt->data, evt->data_len);
                response->response_len += evt->data_len;

                // 确保有足够空间存放null终止符
                if (response->response_len < HTTP_RESPONSE_SIZE - 1) {
                    response->response_data[response->response_len] = '\0';
                }
                ESP_LOGI(TAG, "Added %d bytes to response buffer (total: %d/%d)",
                         evt->data_len, response->response_len, HTTP_RESPONSE_SIZE);
            } else {
                ESP_LOGW(TAG, "Response buffer full or invalid data (have: %d, need: %d, max: %d), truncating",
                         response->response_len, evt->data_len, HTTP_RESPONSE_SIZE);
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH - HTTP request completed");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED - Connection closed");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT - Redirect received");
        break;
    }
    return ESP_OK;
}

//------------------ 核心HTTP请求处理 ------------------

// 日志输出函数
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

// 执行HTTP请求的核心函数（同步/异步通用）
esp_err_t bsp_http_perform_request(bsp_http_request_context_t *context) {
    ESP_LOGI(TAG, "Performing HTTP request: %s", context->url);

    // 检查是否已被取消
    if (context->cancelled) {
        ESP_LOGI(TAG, "Request ID %lu was cancelled before execution", context->request_id);
        context->response->status = BSP_HTTP_STATUS_ERROR;
        context->response->error_code = ESP_ERR_INVALID_STATE;
        return ESP_ERR_INVALID_STATE;
    }

    // 设置响应状态
    context->response->status = BSP_HTTP_STATUS_IN_PROGRESS;

    // 关键修复：在长时间操作前喂狗（检查任务是否已注册）
    if (esp_task_wdt_status(NULL) == ESP_OK) {
        esp_task_wdt_reset();
    }

    esp_err_t err = ESP_OK;

    // HTTP客户端配置 - 优化超时设置防止长时间阻塞
    esp_http_client_config_t config = {
        .url = context->url,
        .event_handler = http_event_handler,
        .user_data = context->response,
        .timeout_ms = context->timeout_ms > 0 ? context->timeout_ms : 3000, // 减少默认超时到3秒
        .buffer_size = HTTP_CINFIG_BUFFER_SIZE,
        .buffer_size_tx = 1024,
        .is_async = false,          // 确保同步模式，避免内部异步处理导致的复杂性
        .keep_alive_enable = false, // 禁用keep-alive减少连接复杂性
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        context->response->status = BSP_HTTP_STATUS_ERROR;
        context->response->error_code = ESP_FAIL;
        return ESP_FAIL;
    }

    // 再次检查是否已被取消（在HTTP客户端创建后）
    if (context->cancelled) {
        ESP_LOGI(TAG, "Request ID %lu was cancelled after client init", context->request_id);
        esp_http_client_cleanup(client);
        context->response->status = BSP_HTTP_STATUS_ERROR;
        context->response->error_code = ESP_ERR_INVALID_STATE;
        return ESP_ERR_INVALID_STATE;
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
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // 执行请求
    err = esp_http_client_perform(client);

    // 关键修复：HTTP请求完成后喂狗（检查任务是否已注册）
    if (esp_task_wdt_status(NULL) == ESP_OK) {
        esp_task_wdt_reset();
    }

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

        if (context->response) {
            context->response->status = BSP_HTTP_STATUS_ERROR;
            context->response->error_code = err;
        }

        if (context->callback && context->response) {
            context->callback(context->response, context);
        }
    }

    // 清理资源
    esp_http_client_cleanup(client);

    // 统一发出完成信号（同步和异步请求都需要）
    // 关键修复：防止信号量重复Give
    if (context->complete_sem &&
        (context->response->status == BSP_HTTP_STATUS_COMPLETED ||
         context->response->status == BSP_HTTP_STATUS_ERROR)) {

        // 检查信号量计数，防止重复Give
        UBaseType_t sem_count = uxSemaphoreGetCount(context->complete_sem);
        if (sem_count == 0) {
            // 只有当信号量为空时才Give
            BaseType_t give_result = xSemaphoreGive(context->complete_sem);
            if (give_result != pdTRUE) {
                ESP_LOGW(TAG, "Failed to give completion semaphore for request ID: %lu", context->request_id);
            } else {
                ESP_LOGD(TAG, "Completion semaphore given for request ID: %lu", context->request_id);
            }
        } else {
            ESP_LOGD(TAG, "Completion semaphore already given for request ID: %lu (count: %lu)",
                     context->request_id, (unsigned long)sem_count);
        }
    }

    return err;
}

//------------------ 同步请求队列管理 ------------------

// 同步HTTP任务处理函数
static void http_sync_task(void *pvParameters) {
    bsp_http_request_context_t *context;

    ESP_LOGI(TAG, "HTTP sync task started");

    while (1) {
        // 关键修复：在长时间等待前喂狗（检查任务是否已注册）
        if (esp_task_wdt_status(NULL) == ESP_OK) {
            esp_task_wdt_reset();
        }

        // 等待队列中的同步请求
        if (xQueueReceive(http_sync_task_queue, &context, portMAX_DELAY) == pdTRUE) {
            if (context != NULL) {
                // 关键修复：检查内存完整性
                if (context->request_id == 0 || context->url == NULL) {
                    ESP_LOGE(TAG, "HTTP sync task received corrupted context: ID=%lu, URL=%p",
                             context->request_id, context->url);
                    continue; // 跳过损坏的上下文
                }

                ESP_LOGI(TAG, "HTTP sync task processing request ID: %lu, URL: %s",
                         context->request_id, context->url);

                // 只处理同步请求
                if (!context->is_async) {
                    esp_err_t result = bsp_http_perform_request(context);
                    ESP_LOGI(TAG, "HTTP sync task completed request ID: %lu with result: %s",
                             context->request_id, esp_err_to_name(result));
                } else {
                    ESP_LOGW(TAG, "HTTP sync task received async context ID: %lu, skipping", context->request_id);
                }
            } else {
                ESP_LOGE(TAG, "HTTP sync task received NULL context from queue");
            }
        } else {
            ESP_LOGE(TAG, "HTTP sync task failed to receive from queue");
        }
    }
}

// 将同步请求加入队列
esp_err_t bsp_http_queue_sync_request(bsp_http_request_context_t *context) {
    if (context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (http_sync_task_queue == NULL) {
        ESP_LOGE(TAG, "Sync HTTP task queue not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Queueing sync request ID: %lu", context->request_id);

    if (xQueueSend(http_sync_task_queue, &context, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue sync request ID: %lu", context->request_id);
        return ESP_FAIL;
    }

    return ESP_OK;
}

//------------------ 上下文管理 ------------------

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
bsp_http_request_context_t *bsp_http_create_context(void) {
    bsp_http_request_context_t *context = calloc(1, sizeof(bsp_http_request_context_t));
    if (context == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context memory");
        return NULL;
    }

    // 初始化上下文
    context->request_id = generate_request_id();
    context->complete_sem = xSemaphoreCreateBinary();
    context->response = calloc(1, sizeof(bsp_http_response_t));
    context->is_async = true;   // 默认为异步，由调用者设置
    context->cancelled = false; // 初始化取消标志

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
    if (context == NULL)
        return;

    ESP_LOGD(TAG, "Freeing context with ID: %lu", context->request_id);

    // 关键修复：安全清理字符串，防止heap_caps_free失败
    if (context->url) {
        free(context->url);
        context->url = NULL;
    }
    if (context->post_data) {
        free(context->post_data);
        context->post_data = NULL;
    }

    // 清理响应数据
    if (context->response) {
        bsp_http_response_cleanup(context->response);
        free(context->response);
    }

    // 清理同步原语
    if (context->complete_sem) {
        vSemaphoreDelete(context->complete_sem);
    }

    free(context);
}

// 清理HTTP响应数据
void bsp_http_response_cleanup(bsp_http_response_t *response) {
    if (response == NULL) {
        return;
    }

    // 关键修复：检查响应长度是否异常，异常时不释放内存防止闪退
    if (response->response_len > HTTP_RESPONSE_SIZE || response->response_len < 0) {
        ESP_LOGW(TAG, "Response length is corrupted (%d), not freeing buffer to prevent crash",
                 response->response_len);
        // 不释放可能损坏的内存，直接重置指针
        response->response_data = NULL;
        response->response_len = 0;
        return;
    }

    // 正常情况下释放内存
    if (response->response_data != NULL) {
        ESP_LOGD(TAG, "Freeing response data buffer");
        free(response->response_data);
        response->response_data = NULL;
        response->response_len = 0;
    }
}