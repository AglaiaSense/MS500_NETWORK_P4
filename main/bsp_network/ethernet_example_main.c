/* Ethernet Basic Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "ethernet_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#include "esp_ping.h"
#include "ping/ping_sock.h"
#include "lwip/inet.h"
 


#include "esp_log.h"
#include "mqtt_client.h"
static const char *TAG = "eth_example";

// ------------------------------------     mqtt       ------------------------------------

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

#define bast_host "159.75.149.126"
#define bast_port 1883
#define mqtt_username "admin"
#define mqtt_password "public"
#define CLIENT_ID "aglaia"

static void mqtt_app_start(void) {

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = bast_host,
        .broker.address.port = bast_port,
        .credentials.client_id = CLIENT_ID,
        .credentials.username = mqtt_username,
        .credentials.authentication.password = mqtt_password,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP};

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// ------------------------------------      Ethernet      ------------------------------------

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:

        uint8_t mac_addr[6] = {0};
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

        uint32_t phy_addr = 0;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_PHY_ADDR, &phy_addr);
        ESP_LOGI(TAG, "ETH PHY Addr: %d", phy_addr);

        uint32_t speed = 0;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
        ESP_LOGI(TAG, "ETH Speed: %d Mbps", speed);

        uint32_t duplex = 0;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex);
        ESP_LOGI(TAG, "ETH Duplex Mode: %s", duplex ? "Full" : "Half");

        bool autonego = false;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_AUTONEGO, &autonego);
        ESP_LOGI(TAG, "ETH Autonegotiation: %s", autonego ? "Enabled" : "Disabled");

        // ... existing code ...
        uint32_t reg_val = 0;
        esp_eth_phy_reg_rw_data_t phy_reg_read = {
            .reg_addr = 0x01,
            .reg_value_p = &reg_val};
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "Status Reg: 0x%04x", reg_val);

        phy_reg_read.reg_addr = 0x05;
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "Link Partner Ability: 0x%04x", reg_val);

        phy_reg_read.reg_addr = 0x00; // 读取控制寄存器
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "Control Reg: 0x%04x", reg_val);

        phy_reg_read.reg_addr = 0x04; // 读取自协商广告
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "AN Advertisement: 0x%04x", reg_val);

        phy_reg_read.reg_addr = 0x18; // 状态监控
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "AN check : 0x%04x", reg_val);

        phy_reg_read.reg_addr = 0x19;
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "Spec Control/Status Reg 0x19: 0x%04x", reg_val);

        phy_reg_read.reg_addr = 0x1A;
        esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg_read);
        ESP_LOGI("PHY", "Interrupt Status Reg 0x1A: 0x%04x", reg_val);


        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;

    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {

    // 现有代码...
    ESP_LOGI(TAG, "------------------------------------------\n");
    printf("event_base = %s, event_id = %d\n", event_base, event_id);
    printf("event_data = %p\n", event_data);

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "IP Check: " IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    mqtt_app_start();
}
// 定义PING成功回调函数
static void ping_success_cb(esp_ping_handle_t hdl, void *args) {
    ESP_LOGI(TAG, "Ping successful");
    // 这里可以添加PING成功后的处理逻辑
}

// 定义PING结束回调函数  
static void ping_end_cb(esp_ping_handle_t hdl, void *args) {
    ESP_LOGI(TAG, "Ping session ended");
    esp_ping_delete_session(hdl);
}
void app_main_ethernet(void) {

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    esp_netif_t *eth_netifs[eth_port_cnt];
    esp_eth_netif_glue_handle_t eth_netif_glues[eth_port_cnt];

    // Create instance(s) of esp-netif for Ethernet(s)
    if (eth_port_cnt == 1) {
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);
        eth_netif_glues[0] = esp_eth_new_netif_glue(eth_handles[0]);
        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
        // ESP_ERROR_CHECK(esp_netif_dhcpc_start(eth_netifs[0]));

        // 停掉 DHCP，设置静态 IP
esp_netif_ip_info_t ip_info;
ip_info.ip.addr = ipaddr_addr("192.168.0.100");         // 静态 IP：你手动选的未被占用的地址
ip_info.netmask.addr = ipaddr_addr("255.255.255.0");    // 子网掩码：保持不变
ip_info.gw.addr = ipaddr_addr("192.168.0.1");           // 网关：跟 DHCP 的一致

ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netifs[0]));   // 停止 DHCP
ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netifs[0], &ip_info));  // 设置静态 IP


    } else {
        // Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
        // esp-netif configuration parameters for each interface (name, priority, etc.).
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg_spi = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++) {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i * 5;
            eth_netifs[i] = esp_netif_new(&cfg_spi);
            eth_netif_glues[i] = esp_eth_new_netif_glue(eth_handles[0]);
            // Attach Ethernet driver to TCP/IP stack
            ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[i], eth_netif_glues[i]));
            esp_netif_dhcpc_start(eth_netifs[i]);
        }
    }
    printf("eth_port_cnt = %d\n", eth_port_cnt);
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    for (int i = 0; i < eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }


    
        // ... existing code ...
printf("------------------------------------------\n");
esp_netif_ip_info_t check;
esp_netif_get_ip_info(eth_netifs[0], &check);
ESP_LOGI(TAG, "IP: " IPSTR ", Mask: " IPSTR ", GW: " IPSTR,
        IP2STR(&check.ip), IP2STR(&check.netmask), IP2STR(&check.gw));



// 修改ping代码部分
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    // inet_aton(AF_INET, "192.168.0.1", &config.target_addr);
    ipaddr_aton("192.168.0.1", &config.target_addr);
    // ipaddr_aton("192.168.0.100", &config.target_addr);
    config.count = 4; // ping 4次

    esp_ping_callbacks_t cbs = {
        .on_ping_success = ping_success_cb,
        .on_ping_timeout = NULL,
        .on_ping_end = ping_end_cb,
        .cb_args = NULL,
    };

    esp_ping_handle_t ping;
    ESP_ERROR_CHECK(esp_ping_new_session(&config, &cbs, &ping));
    ESP_ERROR_CHECK(esp_ping_start(ping));

#if CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S >= 0
    // For demonstration purposes, wait and then deinit Ethernet network
    vTaskDelay(pdMS_TO_TICKS(CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S * 1000));
    ESP_LOGI(TAG, "stop and deinitialize Ethernet network...");
    // Stop Ethernet driver state machine and destroy netif
    for (int i = 0; i < eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_stop(eth_handles[i]));
        ESP_ERROR_CHECK(esp_eth_del_netif_glue(eth_netif_glues[i]));
        esp_netif_destroy(eth_netifs[i]);
    }
    esp_netif_deinit();
    ESP_ERROR_CHECK(example_eth_deinit(eth_handles, eth_port_cnt));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler));
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
#endif // EXAMPLE_ETH_DEINIT_AFTER_S > 0
}
