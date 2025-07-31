/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef __FW_LOADER_H__
#define __FW_LOADER_H__

 
int fw_spi_boot(void);
int fw_flash_boot(void);
int fw_flash_update(void);

extern int spi_master_dev_write(uint8_t *data, int len) ;

#endif
