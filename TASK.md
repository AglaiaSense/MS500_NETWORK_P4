
## 2025-8-7 ethernet和WIFI的STA模式

#### 说明：
1.bsp_ethernet.c：网线联网
2.bsp_wifi.c：WIFI联网
 

##### 需求：
1.在Ethernet和WIFI联网成功。在bsp_network中，进行回调，并调用bsp_mqtt_init
2.不改变原有代码的_register和handle。
3.使用EventGroupHandle_t 可以知道当前联网状态。

 

#### 流程：
1.先创建使用EventGroupHandle_t和枚举状态
2.后在bsp_network中监控联网状态
 