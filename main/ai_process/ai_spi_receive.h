#ifndef __AI_SPI_RECEIVE_H__
#define __AI_SPI_RECEIVE_H__

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/spi_slave.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"


 #if 0
 //288*288
 //#define SLAVE_RECEIVE_BUF_SIZE  (13644)   //(13643)  // 7280 + 1260 + 5103 + 1
 
 //4*1701 + 1260 + 1701*3 
 //4*1701 + 476 + 1701*3
 //#define TENSOR1_SIZE            (7280)
 //#define TENSOR2_SIZE            (5103)      
 //#define TENSOR2_BUF_OFFSET      (8540)   // 7280 + 1260
 
 #define TENSOR1_SIZE            (7552)
 #define TENSOR2_SIZE            (3402)      
 #define TENSOR2_BUF_OFFSET      (8812)   // 7280 + 1260
 #endif
 
 #if 1
 // vehicle 5: 416*416
 // header | tensor1 | align size | tensor2
 #define TENSOR1_SIZE            (14944)     // 12 + 448 + 3549*4 = 14656
 #define TENSOR2_SIZE            (7104)     // 1)4 class: 3549*4=14196, 2)5 class:3549*5=17745
 #define TENSOR1_ALIGN_SIZE      (1932)      // 8*2016-3549*4=1932
 #define TENSOR2_BUF_OFFSET      (16876)   // 14656 + 1932
//  #define SLAVE_RECEIVE_BUF_SIZE  (23974 + 1)  // 14656 + 1932 + 17745 = 34333
 #define SLAVE_RECEIVE_BUF_SIZE (24000)  // 14656 + 1932 + 17745 = 34333
//  #define SLAVE_RECEIVE_BUF_SIZE  (24000)  // 14656 + 1932 + 17745 = 34333
 
 #endif
 
 #if 0
 // nota model: 480*480
 #define TENSOR1_SIZE            (22124)     // 12 + 512 + 30*30*24
 #define TENSOR1_ALIGN_SIZE      (576)       // 11*2016-30*30*24
 #define TENSOR2_SIZE            (5400)       // 15*15*24
 #define TENSOR2_BUF_OFFSET      (TENSOR1_SIZE + TENSOR1_ALIGN_SIZE)
 #define SLAVE_RECEIVE_BUF_SIZE  (TENSOR2_BUF_OFFSET + TENSOR2_SIZE)
 #endif
 
 // #define LPD_TRANS_BANK_SIZE          (12416)  // 必须是32的倍数
 // #define LPD_TRANS_BANK_CYCLE         (10)     // 20 / 2 因为每次循环读取两次
 // #define LPD_TRANS_LAST_SIZE          (12216)
 // #define LPD_DNN_IN_LAST_SIZE         (1260)   // input DNN在最后一个包中的数据大小,计算数据是1260,但逻辑分析仪抓取到的是1259，反而output dnn多了一个byte变为10957
 // #define LPD_LAST_BUF_SIZE            (13644)  // 为了为了保证后续copy和处理 
 
 #define LPD_TRANS_BANK_SIZE          (12416)  // 必须是32的倍数
 #define LPD_TRANS_BANK_CYCLE         (21)     // 20 / 2 因为每次循环读取两次
 #define LPD_TRANS_LAST_SIZE          (5728)
 // #define LPD_DNN_IN_LAST_SIZE         (8844)   // 480模型的余数 
 #define LPD_DNN_IN_LAST_SIZE         (10860)   // 416模型的余数
 #define LPD_LAST_BUF_SIZE            (7187)  // 为了为了保证后续copy和处理 
 
 #define LPR_TRANS_BANK_SIZE          (12416)  // 必须是32的倍数
 #define LPR_TRANS_BANK_CYCLE         (1)     // 2 / 2 因为每次循环读取两次
 #define LPR_TRANS_LAST_SIZE          (2496)
 #define LPR_DNN_IN_LAST_SIZE         (108)   // input DNN在最后一个包中的数据大小,计算数据是1260,但逻辑分析仪抓取到的是1259，反而output dnn多了一个byte变为10957
 #define LPR_LAST_BUF_SIZE            (13644)  // 为了为了保证后续copy和处理 
 
 
 
// 主要接收函数
void spi_slave_receive_data(void);
int ssr_dnn_out_LPD_LPR(void);

#endif // __AI_SPI_RECEIVE_H__
