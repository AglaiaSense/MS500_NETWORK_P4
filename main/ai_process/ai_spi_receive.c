#include "ai_spi_receive.h"
#include "output_tensor_parser.h"
#include "vc_config.h"

static const char *TAG = "SPI_RECEIVE";

extern int is_sleeping;
extern int is_process_done;

/**
 * 项目流程：
 * 1.图像采集 → 预处理 → LPD（车牌定位） → 字符分割 → LPR（字符识别） → 结果输出
 * 2.通过 dnn_input_enable 使能input输出
 * 3. input: 原始图像(RGB图片)  output: 处理后的图像(车辆检查，边框)
 */

/**
 * @brief  output 车牌定位，字符识别
 */
int ssr_dnn_out_LPD_LPR(void) {

    esp_err_t ret;
    // 动态分配内存（确保对齐）
    WORD_ALIGNED_ATTR char *recvbuf = heap_caps_malloc(SLAVE_RECEIVE_BUF_SIZE, MALLOC_CAP_DMA);
  	// printf("%s(%d): \n", __func__, __LINE__);
    // check_memory();

    char *buf_bk = malloc(TENSOR2_SIZE + 1);
    if (!recvbuf || !buf_bk) {
        printf("Memory allocation failed!\n");
        vTaskDelete(NULL); // 处理分配失败
        return -1;
    }

    WORD_ALIGNED_ATTR char *tensor1_recvbuf = recvbuf;
    WORD_ALIGNED_ATTR char *tensor2_recvbuf = recvbuf + TENSOR2_BUF_OFFSET;
    spi_slave_transaction_t tensor1, tensor2;
    spi_slave_transaction_t *tt;
    char *buf;
    size_t fcnt = 1;

    memset(recvbuf, 0, SLAVE_RECEIVE_BUF_SIZE);
    memset(&tensor1, 0, sizeof(tensor1));
    memset(&tensor2, 0, sizeof(tensor2));

   while (is_sleeping == 0) {
        // Clear receive buffer, set send buffer to something sane
        memset(recvbuf, 0x00, SLAVE_RECEIVE_BUF_SIZE);


        tensor1.length = SLAVE_RECEIVE_BUF_SIZE * 8;
        tensor1.tx_buffer = NULL;
        tensor1.rx_buffer = tensor1_recvbuf;
        ret = spi_slave_queue_trans(SPI2_HOST, &tensor1, portMAX_DELAY);

        ret = spi_slave_get_trans_result(SPI2_HOST, &tt, portMAX_DELAY);
        // printf("[SPI_DEV] total transfer data size = %d\n", (*tt).trans_len);

        if (recvbuf[0] == 0x01) {
            buf = (*tt).rx_buffer;

            /*
            printf("[SPI_SLAVE] Received: [begin: ");
            for(int j = 0; j < 12; j++) {
                printf("%X ", buf[j]);
            }
            printf("\n"); */

            memset(buf_bk, 0, TENSOR2_SIZE + 1);
            memcpy(buf_bk, buf + TENSOR1_SIZE, TENSOR2_SIZE + 1);

            memset(buf + TENSOR1_SIZE, 0, SLAVE_RECEIVE_BUF_SIZE - TENSOR1_SIZE);
            memcpy(buf + TENSOR2_BUF_OFFSET, buf_bk, TENSOR2_SIZE + 1);

            recvbuf[2] = 0xE0;
            recvbuf[3] = 0x07;
            process_dnn_buffer((uint8_t *)recvbuf, SLAVE_RECEIVE_BUF_SIZE);
        }
    }
    free(recvbuf);
    free(buf_bk);

    
    is_process_done = 1;
    printf("***************************************************************\n");
    // delay_ms(1000);
    vTaskDelete(NULL);

    return 0;
}

/**
 * @brief spi_slave_receive_data
 */
void spi_slave_receive_data(void) {

    ssr_dnn_out_LPD_LPR();
}