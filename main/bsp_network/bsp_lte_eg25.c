#include "driver/uart.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp_lte_eg25.h"
#include "bsp_lte_uart.h"

char *result, *at_command;
char at_cmd_arr[RX1_BUF_SIZE];
int errcount = 0; // 错误计数器

// --------------------------   网络激活 ------------------------------------------

// 配置 LTE 基础信息
void bsp_lte_net_config(void) {

    printf("start init LET\r\n");

    // 显示 MT 的 ID 信息)
    at_command = "ATI\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    while (result == NULL) {
        // bsp_lte_clear_buffer();
        printf("Module initializing...\r\n");

        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"OK"); // 检查OK
    }
    bsp_lte_clear_buffer();
    printf("****Module initialization successful*****\r\n");

    // 设置命令回显模式
    at_command = "ATE1\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    while (result == NULL) {
        // bsp_lte_clear_buffer();
        printf("%s",at_command);
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"OK"); // 检查OK
    }
    bsp_lte_clear_buffer();

    // 检查功能模式
    at_command = "AT+CFUN=1\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"OK"); 
    while (result == NULL) {
        // bsp_lte_clear_buffer();
        printf("%s",at_command);
       
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
       
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    }
    bsp_lte_clear_buffer();

    // 获取SIM卡信息
    at_command = "AT+CIMI\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"460"); // 检查460标识
    while (result == NULL) {
        // bsp_lte_clear_buffer();
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        printf("%s",at_command);

        bsp_lte_delay_uart_rx();

        result = strstr((const char *)uart1_rx_data, (const char *)"460");
    }
    bsp_lte_clear_buffer();

    // 检查附着状态
    at_command = "AT+CGATT?\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"+CGATT: 1");
    while (result == NULL) {
        // bsp_lte_clear_buffer();
        printf("%s",at_command);

        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();

        result = strstr((const char *)uart1_rx_data, (const char *)"+CGATT: 1");
    
    }
    bsp_lte_clear_buffer();

    // / 检查信号质量
    at_command = "AT+CSQ\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"+CSQ:");
    if (result) {
        printf("signal strength: %s\r\n", uart1_rx_data);
    }
    bsp_lte_clear_buffer();
}

// 配置激活PDP(联网)
void bsp_lte_act_config(void) {
    // 查询QICSGP激活状态
    at_command = "AT+QIACT?\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    result = strstr((const char *)uart1_rx_data, (const char *)"+QIACT: 1"); // 返1
    bsp_lte_clear_buffer();
    while (result == NULL) {
        bsp_lte_clear_buffer();

        // 配置QICSGP上下文
        at_command = "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        while (result == NULL) {
            errcount++;
            result = strstr((const char *)uart1_rx_data, (const char *)"OK");
            //   超时循环，表示失败
            if (errcount > 100) {
                errcount = 0;
                break;
            }
        }
        if (result) {
            printf("AT+QICSGP ok\r\n");
        } else {
            printf("AT+QICSGP fail\r\n");
        }

        // 激活QIACT =1
        at_command = "AT+QIACT=1\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");

        if (result) {
            printf("AT+QIACT ok\r\n");
        } else {
            printf("AT+QIACT fail\r\n");
        }

        // 查询QICSGP激活状态
        at_command = "AT+QIACT?\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_ms(1000);
        result = strstr((const char *)uart1_rx_data, (const char *)"+QIACT:");
    }
}

// 配置 HTTP(S)服务器参数
void bsp_lte_http_config(void) {

    // PDP 上下文 ID
    at_command = "AT+QHTTPCFG=\"contextid\",1\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    if (result) {
        printf("contextid congif succeed\r\n");
    }
    bsp_lte_clear_buffer();

    //  内容类型  0 application/x-www-form-urlencoded 4 application/json
    at_command = "AT+QHTTPCFG=\"contenttype\",4\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    if (result) {
        printf("contenttype congif  succeed\r\n");
    }
    bsp_lte_clear_buffer();

    //  设置header头
    at_command = "AT+QHTTPCFG=\"custom_header\",\"Authorization: Token 60e5c2a244f629d33290d23d75a63ba8e7f55555\"\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    if (result) {
        printf("contenttype congif  succeed\r\n");
    }
    bsp_lte_clear_buffer();

}

// --------------------------   网络请求 ------------------------------------------

// 同步网络时间
void bsp_lte_get_net_time(char *time_buffer) {
    // QLTS 查询本地时间
    const char *at_command = "AT+QLTS=2\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 查找"+QLTS:"字符串
    char *result = strstr((const char *)uart1_rx_data, (const char *)"+QLTS:");
    while (result == NULL) {
        bsp_lte_clear_buffer();

        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"+QLTS:");
    }

    // 查找第一个引号的位置
    char *start_quote = strchr(result, '"');
    if (start_quote != NULL) {
        // 查找第二个引号的位置
        char *end_quote = strchr(start_quote + 1, '"');
        if (end_quote != NULL) {
            // 计算时间字符串的起始位置和长度
            size_t length = end_quote - start_quote - 1;

            // 确保缓冲区足够大
            if (length < 64) {
                strncpy(time_buffer, start_quote + 1, length);
                time_buffer[length] = '\0'; // 添加字符串结束符
            } else {
                printf("Buffer size is too small");
            }
        }
    }

    bsp_lte_clear_buffer();

}

// 发送 HTTP GET 请求并获取响应数据
esp_err_t bsp_lte_http_get_request(const char *request_data, char *respond_data) {

    printf("get_url: %s\r\n", request_data);

    // 发送URL设置请求  (30：超时时间)
    sprintf(at_cmd_arr, "AT+QHTTPURL=%d,30\r\n", strlen(request_data));
    at_command = at_cmd_arr;
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"CONNECT"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送URL
    uart_write_bytes(UART_NUM_2, request_data, strlen(request_data));
    bsp_lte_delay_uart_rx();

    // 等待"OK"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    errcount = 0;
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送 GET请求
    at_command = "AT+QHTTPGET=30\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"+QHTTPGET: 0""响应
    result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPGET: 0");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPGET: 0");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送读取请求
    at_command = "AT+QHTTPREAD=30\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"+QHTTPREAD: 0"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPREAD: 0");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPREAD: 0");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;

            bsp_lte_clear_buffer();
            return ESP_FAIL; // 返回失败
        }
    }
    // 提取 JSON 响应数据
    if (result != NULL) {
        char *json_start = strchr(uart1_rx_data, '{');
        char *json_end = strrchr(uart1_rx_data, '}');
        if (json_start != NULL && json_end != NULL && json_end > json_start) {
            size_t json_length = json_end - json_start + 1;
            if (json_length < RX1_BUF_SIZE) {
                strncpy(respond_data, json_start, json_length);
                respond_data[json_length] = '\0'; // 确保字符串以 null 结尾
                bsp_lte_clear_buffer();
                return ESP_OK;
            } else {
                bsp_lte_clear_buffer();
                return ESP_ERR_NO_MEM; // 返回内存不足错误
            }
        } else {

            bsp_lte_clear_buffer();
            return ESP_FAIL; // 返回失败
        }
    } else {
        bsp_lte_clear_buffer();
        return ESP_FAIL; // 返回失败
    }
}

// 发送 HTTP POST 请求并获取响应数据
esp_err_t bsp_lte_http_post_request(const char *request_url, const char *request_params, char *respond_data) {

    printf("post_url: %s\r\n", request_url);
    printf("post_params: %s\r\n", request_params);

    // 发送URL设置请求  (30：超时时间)
    sprintf(at_cmd_arr, "AT+QHTTPURL=%d,30\r\n", strlen(request_url));
    at_command = at_cmd_arr;
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"CONNECT"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送URL
    uart_write_bytes(UART_NUM_2, request_url, strlen(request_url));
    bsp_lte_delay_uart_rx();

    // 等待"OK"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    errcount = 0;
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送 POST请求
    sprintf(at_cmd_arr, "AT+QHTTPPOST=%d,30,30\r\n", strlen(request_params));
    at_command = at_cmd_arr;
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"CONNECT"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送 Params
    uart_write_bytes(UART_NUM_2, request_params, strlen(request_params));
    bsp_lte_delay_uart_rx();

    // 等待"+QHTTPPOST: 0"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPPOST: 0");
    errcount = 0;
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPPOST: 0");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送读取请求
    at_command = "AT+QHTTPREAD=30\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"+QHTTPREAD: 0"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPREAD: 0");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPREAD: 0");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;

            bsp_lte_clear_buffer();
            return ESP_FAIL; // 返回失败
        }
    }

    // 提取 JSON 响应数据
    if (result != NULL) {
        char *json_start = strchr(uart1_rx_data, '{');
        char *json_end = strrchr(uart1_rx_data, '}');

        if (json_start != NULL && json_end != NULL && json_end > json_start) {
            size_t json_length = json_end - json_start + 1;
            if (json_length < RX1_BUF_SIZE) {
                strncpy(respond_data, json_start, json_length);
                respond_data[json_length] = '\0';
                bsp_lte_clear_buffer();
                return ESP_OK;
            } else {
                bsp_lte_clear_buffer();
                return ESP_ERR_NO_MEM; // 返回内存不足错误
            }
        } else {

            bsp_lte_clear_buffer();
            return ESP_FAIL; // 返回失败
        }
    } else {
        bsp_lte_clear_buffer();
        return ESP_FAIL; // 返回失败
    }
}
// 发送 HTTP PUT 请求并获取响应数据
esp_err_t bsp_lte_http_put_request(const char *request_url, const char *request_params, char *respond_data) {

    printf("put_url: %s\r\n", request_url);
    printf("put_params: %s\r\n", request_params);

    // 发送URL设置请求  (30：超时时间)
    sprintf(at_cmd_arr, "AT+QHTTPURL=%d,30\r\n", strlen(request_url));
    at_command = at_cmd_arr;
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"CONNECT"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送URL
    uart_write_bytes(UART_NUM_2, request_url, strlen(request_url));
    bsp_lte_delay_uart_rx();

    // 等待"OK"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"OK");
    errcount = 0;
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送 PUT 请求
    sprintf(at_cmd_arr, "AT+QHTTPPUT=%d,30,30\r\n", strlen(request_params));
    at_command = at_cmd_arr;
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"CONNECT"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"CONNECT");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送 Params
    uart_write_bytes(UART_NUM_2, request_params, strlen(request_params));
    bsp_lte_delay_uart_rx();

    // 等待"+QHTTPPUT: 0"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPPUT: 0");
    errcount = 0;
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPPUT: 0");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            return ESP_FAIL; // 返回失败
        }
    }
    bsp_lte_clear_buffer();

    // 发送读取请求
    at_command = "AT+QHTTPREAD=30\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    // 等待"+QHTTPREAD: 0"响应
    result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPREAD: 0");
    while (result == NULL) {
        errcount++;
        result = strstr((const char *)uart1_rx_data, (const char *)"+QHTTPREAD: 0");
        bsp_lte_delay_uart_rx();
        if (errcount > 100) {
            errcount = 0;
            bsp_lte_clear_buffer();
            return ESP_FAIL; // 返回失败
        }
    }

    // 提取 JSON 响应数据
    if (result != NULL) {
        char *json_start = strchr(uart1_rx_data, '{');
        char *json_end = strrchr(uart1_rx_data, '}');

        if (json_start != NULL && json_end != NULL && json_end > json_start) {
            size_t json_length = json_end - json_start + 1;
            if (json_length < RX1_BUF_SIZE) {
                strncpy(respond_data, json_start, json_length);
                respond_data[json_length] = '\0';
                bsp_lte_clear_buffer();
                return ESP_OK;
            } else {
                bsp_lte_clear_buffer();
                return ESP_ERR_NO_MEM; // 返回内存不足错误
            }
        } else {
            bsp_lte_clear_buffer();
            return ESP_FAIL; // 返回失败
        }
    } else {
        bsp_lte_clear_buffer();
        return ESP_FAIL; // 返回失败
    }
}

// --------------------------  功能模式 ------------------------------------------

// 最低功耗模式
void bsp_lte_set_low_power(void) {
    // 检查是否最低功能
    at_command = "AT+CFUN?\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    result = strstr((const char *)uart1_rx_data, (const char *)"+CFUN: 0");
    bsp_lte_clear_buffer();
    while (result == NULL) {
        // 设置 
        at_command = "AT+CFUN=0\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");
        while (result == NULL) {
            errcount++;
            result = strstr((const char *)uart1_rx_data, (const char *)"OK");
            if (errcount > 100) {
                errcount = 0;
                break;
            }
        }
        bsp_lte_clear_buffer();

        //  查询COPS
        at_command = "AT+COPS?\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"+COPS: 2");
        if (result) {
            printf("COPS: %s\r\n", uart1_rx_data);
        }
        bsp_lte_clear_buffer();

        //  查询
        at_command = "AT+CFUN?\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"+CFUN: 0");
        bsp_lte_clear_buffer();
    }
}

void bsp_lte_set_all_power(void) {
    // 检查功能模式
    at_command = "AT+CFUN?\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_uart_rx();

    result = strstr((const char *)uart1_rx_data, (const char *)"+CFUN: 1");
    bsp_lte_clear_buffer();
    while (result == NULL) {
        // 设置功能
        at_command = "AT+CFUN=1\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"OK");
        while (result == NULL) {
            errcount++;
            result = strstr((const char *)uart1_rx_data, (const char *)"OK");
            if (errcount > 100) {
                errcount = 0;
                break;
            }
        }
        bsp_lte_clear_buffer();

        // 查询 
        at_command = "AT+CFUN?\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"+CFUN: 1");
        bsp_lte_clear_buffer();
    }
}

void bsp_lte_set_close_power(void) {
    // 关机
    at_command = "AT+QPOWD\r\n";
    uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
    bsp_lte_delay_ms(1000);
    result = strstr((const char *)uart1_rx_data, (const char *)"POWERED DOWN");
    bsp_lte_clear_buffer();

    while (result == NULL) {
        // 设置功能
        at_command = "AT+QPOWD\r\n";
        uart_write_bytes(UART_NUM_2, at_command, strlen(at_command));
        bsp_lte_delay_uart_rx();
        result = strstr((const char *)uart1_rx_data, (const char *)"POWERED DOWN");
        while (result == NULL) {
            errcount++;
            result = strstr((const char *)uart1_rx_data, (const char *)"POWERED DOWN");
            if (errcount > 10) {
                errcount = 0;
                break;
            }
        }
        bsp_lte_clear_buffer();
    }
}

// --------------------------  初始化 ------------------------------------------
void bsp_lte_eg25_init(void) {
    bsp_lte_net_config();
    bsp_lte_act_config();
    bsp_lte_http_config();
}
