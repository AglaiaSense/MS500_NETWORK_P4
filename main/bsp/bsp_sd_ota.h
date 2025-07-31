#ifndef BSP_SD_OTA_H
#define BSP_SD_OTA_H

#include <stdio.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "vc_config.h"


int bsp_sd_ota_check(void)  ;

#endif // BSP_SD_OTA_H
