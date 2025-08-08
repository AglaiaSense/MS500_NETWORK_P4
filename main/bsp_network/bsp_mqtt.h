#ifndef BSP_MQTT_H
#define BSP_MQTT_H

#include "esp_err.h"

//--------------------------- MQTT抽象接口 ---------------------------

// MQTT初始化
void bsp_mqtt_init(void);

// 统一的MQTT订阅接口，参数只需要主题字符串
esp_err_t bsp_mqtt_subscribe(const char* topic);

// 统一的MQTT发布接口，参数只需要JSON字符串
esp_err_t bsp_mqtt_publish_json(const char* topic, const char* json_data);

// 获取设备MAC地址的字符串形式
void bsp_mqtt_get_mac_string(char* mac_str, size_t mac_str_len);

//--------------------------- JSON命令处理 ---------------------------

// JSON命令处理回调函数类型
typedef void (*mqtt_json_handler_t)(const char* command, const char* param, const char* data);

// 设置JSON命令处理回调
void bsp_mqtt_set_json_handler(mqtt_json_handler_t handler);

#endif // BSP_MQTT_H