/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#include "ai_spi_dev.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "nvs_flash.h"

static const char *TAG = "AI SPI DEV";

spi_device_handle_t spi;

/**
 * @brief spi_master_dev_write
 */
int spi_master_dev_write(uint8_t *data, int len) {
    esp_err_t ret;
    spi_transaction_t t;

    if (len == 0) // 数据长度校验，长度为0则直接返回错误
        return -1;

    memset(&t, 0, sizeof(t)); // 初始化SPI事务结构体

    t.length = len * 8;                         // 将字节长度转换为比特长度（SPI协议按位传输）
    t.tx_buffer = data;                         // 设置发送数据缓冲区
    t.user = (void *)1;                         // 设置用户数据，通常用于DC信号控制
    ret = spi_device_polling_transmit(spi, &t); // 执行阻塞式SPI传输
    assert(ret == ESP_OK);                      // 断言检查传输结果，确保无错误
    return 0; // 返回成功状态
}

/**
 * @brief spi_master_dev_read
 */
void spi_master_dev_read(uint8_t *data) {
    spi_transaction_t t;

    // gpio_set_level(PIN_NUM_CS, 0);  // 手动控制CS引脚（已注释）

    memset(&t, 0, sizeof(t)); // 初始化SPI事务结构体

    t.length = 8;                                         // 设置传输长度为8位（1字节）
    t.flags = SPI_TRANS_USE_RXDATA;                       // 使用内部接收缓冲区
    t.user = (void *)1;                                   // 设置用户数据（通常用于DC信号控制）
    esp_err_t ret = spi_device_polling_transmit(spi, &t); // 执行阻塞式SPI读取
    assert(ret == ESP_OK);                                // 断言检查传输结果，确保无错误
    *data = t.rx_data[0];                                 // 将接收到的数据存入输出参数

    // gpio_set_level(PIN_NUM_CS, 1);  // 手动释放CS引脚（已注释）
}


void spi_master_test() {

    uint8_t tx_data[1024];
    for (int i = 0; i < 1024; i++) {
        tx_data[i] = 0x55;
    }

    spi_transaction_t trans = {
        .length = 1024 * 8, // 以bit为单位
        .tx_buffer = tx_data,
    };

    // 发送数据
    ESP_ERROR_CHECK(spi_device_transmit(spi, &trans));
}

/**
 * @brief spi_master_dev_init
 */
void spi_master_dev_init(void) {
    esp_err_t ret;


    // SPI总线配置结构体
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_MASTER_MISO,                                  // 主设备MISO引脚
        .mosi_io_num = PIN_MASTER_MOSI,                                  // 主设备MOSI引脚
        .sclk_io_num = PIN_MASTER_CLK,                                   // 主设备时钟引脚
        .quadwp_io_num = -1,                                             // QSPI WP引脚（未使用）
        .quadhd_io_num = -1,                                             // QSPI HD引脚（未使用）
        .max_transfer_sz = MASTER_MAX_TRANS_SIZE,                        // 最大传输数据大小
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS, // 主设备模式，使用GPIO引脚
    };

    // SPI设备接口配置结构体
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,                     // 命令位长度（未使用）
        .address_bits = 0,                     // 地址位长度（未使用）
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // SPI时钟频率40MHz
        .mode = 3,                             // SPI模式3（CPOL=1, CPHA=1）
        .spics_io_num = PIN_MASTER_CS,         // 片选引脚
        .cs_ena_pretrans = 24,                 // 片选信号提前时间
        .cs_ena_posttrans = 3,                 // 片选信号保持时间
        .queue_size = 31,                      // 传输队列大小
        .flags = SPI_DEVICE_NO_DUMMY,          // 无虚位传输标志
    };

    // 初始化SPI总线
    ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret); // 检查初始化结果

    // 添加SPI设备
    ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret); // 检查添加设备结果


    // printf("[SPI_MASTER] init success\n"); // 打印初始化成功信息
}

/**
 * @brief spi_master_dev_destroy
 */
void spi_master_dev_destroy(void) {
    esp_err_t ret;

    // 从SPI总线移除设备
    ret = spi_bus_remove_device(spi);
    ESP_ERROR_CHECK(ret); // 检查移除操作是否成功

    // 释放SPI总线资源
    ret = spi_bus_free(SPI3_HOST);
    ESP_ERROR_CHECK(ret); // 检查释放操作是否成功

    // 打印销毁成功信息
    // printf("[SPI_MASTER] destroy success\n");
}

/**
 * @brief spi_slave_post_trans_cb
 */
void spi_slave_post_trans_cb(spi_slave_transaction_t *trans) {
    printf("#####################\n");
    return;

    char *recvbuf = trans->rx_buffer;

    printf("[SPI_SLAVE] callback received: ");
    for (int j = 0; j < 20; j++) {
        printf("0x%X ", recvbuf[j]);
    }

    printf("\n");
}


void my_post_setup_cb(spi_slave_transaction_t *trans)
{
    printf("my_post_setup_cb\n");
}

//Called after transaction is sent/received. We use this to set the handshake line low.
void my_post_trans_cb(spi_slave_transaction_t *trans)
{
    printf("my_post_trans_cb\n");
}

/**
 * @brief spi_slave_dev_init
 */
void spi_slave_dev_init(void) {
    esp_err_t ret;

    // SPI总线配置结构体
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_SLAVE_MISO,             // 从设备MISO引脚
        .miso_io_num = PIN_SLAVE_MOSI,             // 从设备MOSI引脚
        .sclk_io_num = PIN_SLAVE_CLK,              // 从设备时钟引脚
        .quadwp_io_num = -1,                       // QSPI WP引脚（未使用）
        .quadhd_io_num = -1,                       // QSPI HD引脚（未使用）
        .max_transfer_sz = LPD_LPR_INOUT_MAX_SIZE, // 最大传输数据大小
    };

    // 从设备接口配置结构体
    spi_slave_interface_config_t slvcfg = {
        .mode = 3,                    // SPI模式3（CPOL=1, CPHA=1）
        .spics_io_num = PIN_SLAVE_CS, // 从设备片选引脚
        .queue_size = 30,             // 传输队列大小
        .flags = 0,                   // 特殊标志位（未使用）
        .post_setup_cb = NULL,        // 传输后回调函数（未使用）
        //.post_trans_cb = spi_slave_post_trans_cb  // 传输完成回调（已注释）
    };

    // 初始化SPI从设备接口
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK); // 断言检查初始化结果


}
