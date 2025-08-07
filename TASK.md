
### 2025-8-6 合并LTE代码
说明：
1.lte代码地址：E:\03-MS500-P4\01.code\MS500_P4_C6_Bin\LTE_Uart\main
2.lte_boot_task任务，用于创建等待
3、user_eg25.c,文件，LTE的初始化，和进行HTTP。
4、user_uart.c,通过UART连接LTE，
需求：
1. 本次任务，主要是合并LTE代码，
2. 合并代码放入bsp_network文件夹
3. 新文件名使用bsp_let_uart,bsp_let_eg25,初始化测试等放入bsp_let_boot中。
流程：
1.先新建文件
2.移动对应代码到新文件
3.在bsp_let_boot中进行初始化和调用