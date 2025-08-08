#include "esp_log.h"
#include "mqtt_client.h"
#include "bsp_mqtt.h"
#include "bsp_network.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "bsp_mqtt";

#define MQTT_HOST "159.75.149.126"
#define MQTT_PORT 1883
#define MQTT_USERNAME "admin"
#define MQTT_PASSWORD "public"
#define MQTT_CLIENT_ID "aglaia"

//--------------------------- 全局变量 ---------------------------

static esp_mqtt_client_handle_t g_mqtt_client = NULL;
static mqtt_json_handler_t g_json_handler = NULL;
// MAC地址字符串已在bsp_network.h中声明为全局变量

//--------------------------- 内部函数声明 ---------------------------

static void mqtt_handle_subscription(void);
static void mqtt_handle_data_received(esp_mqtt_event_handle_t event);
static void mqtt_parse_json_command(const char* json_str);
static void mqtt_publish_online_status(void);

//--------------------------- MQTT事件处理 ---------------------------

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 连接成功后进行统一订阅处理
        mqtt_handle_subscription();
        // 发布设备上线状态
        mqtt_publish_online_status();
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        // 使用统一方法处理接收到的数据
        mqtt_handle_data_received(event);
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
        
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

//--------------------------- 统一订阅处理函数 ---------------------------

static void mqtt_handle_subscription(void) {
    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }
    
    // 订阅通用主题
    bsp_mqtt_subscribe("/server/action/com");
    
    // 订阅设备特定主题
    char device_topic[64];
    snprintf(device_topic, sizeof(device_topic), "/server/action/p4/%s", g_mac_str);
    bsp_mqtt_subscribe(device_topic);
    
    ESP_LOGI(TAG, "Subscription setup completed");
}

//--------------------------- 统一数据接收处理函数 ---------------------------

static void mqtt_handle_data_received(esp_mqtt_event_handle_t event) {
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    
    // 创建以null结尾的字符串用于JSON解析
    char *json_str = malloc(event->data_len + 1);
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for JSON string");
        return;
    }
    
    memcpy(json_str, event->data, event->data_len);
    json_str[event->data_len] = '\0';
    
    // 解析JSON命令
    mqtt_parse_json_command(json_str);
    
    free(json_str);
}

//--------------------------- JSON命令解析 ---------------------------

static void mqtt_parse_json_command(const char* json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", json_str);
        return;
    }
    
    cJSON *com_item = cJSON_GetObjectItem(json, "com");
    if (com_item == NULL || !cJSON_IsString(com_item)) {
        ESP_LOGE(TAG, "Missing or invalid 'com' field in JSON");
        cJSON_Delete(json);
        return;
    }
    
    const char *command = com_item->valuestring;
    const char *param = NULL;
    const char *data = NULL;
    
    // 检查param字段（简单参数）
    cJSON *param_item = cJSON_GetObjectItem(json, "param");
    if (param_item != NULL && cJSON_IsString(param_item)) {
        param = param_item->valuestring;
    }
    
    // 检查data字段（复杂数据）
    cJSON *data_item = cJSON_GetObjectItem(json, "data");
    if (data_item != NULL) {
        char *data_str = cJSON_Print(data_item);
        if (data_str != NULL) {
            data = data_str;
        }
    }
    
    // 调用用户设置的处理回调
    if (g_json_handler != NULL) {
        g_json_handler(command, param, data);
    } else {
        ESP_LOGW(TAG, "No JSON handler set, received command: %s", command);
    }
    
    // 释放data字符串内存
    if (data != NULL && data_item != NULL) {
        free((void*)data);
    }
    
    cJSON_Delete(json);
}

//--------------------------- 发布设备上线状态 ---------------------------

static void mqtt_publish_online_status(void) {
    // 发布到通用主题
    char online_json[128];
    snprintf(online_json, sizeof(online_json), 
             "{\"com\":\"online\",\"param\":\"%s\"}", g_mac_str);
    bsp_mqtt_publish_json("/device/action/com", online_json);
    
    // 发布到设备特定主题  
    char device_topic[64];
    snprintf(device_topic, sizeof(device_topic), "/device/action/p4/%s", g_mac_str);
    
    char place_json[128];
    snprintf(place_json, sizeof(place_json), 
             "{\"com\":\"place\",\"param\":\"9q0265\"}");
    bsp_mqtt_publish_json(device_topic, place_json);
}

//--------------------------- 公共接口实现 ---------------------------

void bsp_mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = MQTT_HOST,
        .broker.address.port = MQTT_PORT,
        .credentials.client_id = MQTT_CLIENT_ID,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP
    };

    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(g_mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(g_mqtt_client);
    
    ESP_LOGI(TAG, "MQTT client initialized and started with MAC: %s", g_mac_str);
}

esp_err_t bsp_mqtt_subscribe(const char* topic) {
    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_FAIL;
    }
    
    if (topic == NULL) {
        ESP_LOGE(TAG, "Topic cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_subscribe(g_mqtt_client, topic, 1);
    if (msg_id == -1) {
        ESP_LOGE(TAG, "Failed to subscribe to topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

esp_err_t bsp_mqtt_publish_json(const char* topic, const char* json_data) {
    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_FAIL;
    }
    
    if (topic == NULL || json_data == NULL) {
        ESP_LOGE(TAG, "Topic and JSON data cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_publish(g_mqtt_client, topic, json_data, 0, 1, 0);
    if (msg_id == -1) {
        ESP_LOGE(TAG, "Failed to publish to topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Published to topic: %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

void bsp_mqtt_get_mac_string(char* mac_str, size_t mac_str_len) {
    if (mac_str == NULL || mac_str_len < 13) {
        ESP_LOGE(TAG, "Invalid MAC string buffer, need at least 13 bytes");
        return;
    }
    
    // 直接复制全局MAC地址字符串
    strncpy(mac_str, g_mac_str, mac_str_len - 1);
    mac_str[mac_str_len - 1] = '\0';
}

void bsp_mqtt_set_json_handler(mqtt_json_handler_t handler) {
    g_json_handler = handler;
    ESP_LOGI(TAG, "JSON handler %s", handler ? "set" : "cleared");
}