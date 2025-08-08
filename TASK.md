说明：
1.bsp_mqtt_init的初始化
2.mqtt_event_handler回调响应
需求：
1.MQTT_EVENT_CONNECTED连接成功后进行订阅，使用函数统一处理订阅
2.MQTT_EVENT_DATA接收数据，使用统一方法
3.mqtt的通讯json有下面两种方式
{
  "com": "launch",
  "param": "1001",
}

{
  "com": "launch",
  "data": {
    "key": "bladder"
    "state": "1":启动
    }
  }
  4.esp_mqtt_client_subscribe(client, "/topic/qos1");抽象一个类。参数只有字符串
5.发送esp_mqtt_client_publish也抽象一个类，参数只有json字符串
6.订阅主题"/server/action/com,用于通用名
/server/action/p4_mac地址。用于当前设备
7.发布内容/device/action/com
 {
  "com": "online",
  "param": "mac地址",
}
/device/action/p4_mac地址，用于当前设备
 {
  "com": "plact",
  "param": 9q0265",
}

## 任务完成状态 - 2025-08-08
- [x] 实现统一的MQTT订阅处理函数
- [x] 实现统一的MQTT数据接收处理函数  
- [x] 实现JSON命令解析(com + param/data格式)
- [x] 抽象MQTT订阅函数，参数只需要字符串
- [x] 抽象MQTT发布函数，参数只需要JSON字符串
- [x] 实现订阅主题：/server/action/com 和 /server/action/p4_mac地址
- [x] 实现发布主题：/device/action/com 和 /device/action/p4_mac地址
- [x] 修改mqtt_event_handler支持新的事件处理逻辑

## 实现说明
新的MQTT系统提供了以下特性：
1. **统一的订阅处理**：`mqtt_handle_subscription()` 函数在连接成功后自动订阅所需主题
2. **统一的数据接收**：`mqtt_handle_data_received()` 函数处理所有接收到的MQTT数据
3. **JSON命令解析**：`mqtt_parse_json_command()` 支持两种JSON格式（param和data）
4. **抽象接口**：
   - `bsp_mqtt_subscribe(topic)` - 订阅主题，只需传入字符串
   - `bsp_mqtt_publish_json(topic, json_data)` - 发布JSON，只需传入主题和JSON字符串
5. **自动主题处理**：
   - 订阅：`/server/action/com` 和 `/server/action/p4_<MAC>`
   - 发布：`/device/action/com` 和 `/device/action/p4_<MAC>`
6. **回调机制**：`bsp_mqtt_set_json_handler()` 设置JSON命令处理回调函数