
#ifndef VC_CONFIG_H
#define VC_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include <fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/time.h>

#include <esp_heap_caps.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_video_device.h"
#include "esp_video_init.h"
#include "linux/videodev2.h"

#include "usb_device_uvc.h"
#include "uvc_frame_config.h"

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#include "esp_flash_partitions.h"
#include "esp_ota_ops.h"

#include "as_tools.h"

//----------------------------------------------- run -------------------------------------------------
 

/* SPI_BOOT, FLASH_BOOT, FLASH_UPDATE智能三者选择其一 */
typedef enum {
    BOOT_LAUNCH_FLASH = 0, // 正常启动
    BOOT_UPDATE_FLASH,     // 更新模式 (更新AI模型)
    BOOT_LAUNCH_SPI,       // spi启动
} boot_mode_t;

extern boot_mode_t boot_mode; // 添加全局变量声明


typedef enum {
    UVC_MODEL_VIDEO,  // 视频流模式
    UVC_MODEL_JPG,    // 图像流模式
    UVC_MODEL_ALL     // 开启两个UVC
} uvc_model_t;


typedef enum {
    CAPTURE_MODE_UNKNOWN = 0, // 未知模式
    CAPTURE_MODE_SINGLE_SHOT, // 单次拍照模式
    CAPTURE_MODE_CONTINUOUS   // 连续拍照模式
} capture_mode_t;



//----------------------------------------------- SD Card -------------------------------------------------

// SD卡相关
#define SD_MOUNT_POINT "/sdcard"
#define FIRMWARE_NAME "ms500_p4.bin"
#define FIRMWARE_PATH SD_MOUNT_POINT "/" FIRMWARE_NAME



/**
 * @brief SD card encoder image data format
 */
typedef enum {
    SD_IMG_FORMAT_JPEG, /*!< JPEG format */
    SD_IMG_FORMAT_H264, /*!< H264 format */
} sd_image_format_t;

/* SD Card framebuffer type */
typedef struct sd_card_fb {
    uint8_t *buf;
    size_t buf_bytesused;
    sd_image_format_t fmt;
    size_t width;             /*!< Width of the image frame in pixels */
    size_t height;            /*!< Height of the image frame in pixels */
    struct timeval timestamp; /*!< Timestamp since boot of the frame */
} sd_card_fb_t;

// 文件名想想
#define MAX_IMAGES_PER_FOLDER 100 * 5 // 每个目录最多存储100张图片
#define MAX_FOLDERS 30                // 最多30个目录（jpg1-jpg30）
#define MAX_WAKES_PER_FOLDER 100      // 每个目录存储100次唤醒的图片


// 全局状态结构体
typedef struct {
    int32_t wake_count;          // 当前唤醒次数（NVS保存）
    int32_t image_count;         // 本次唤醒中的图片计数
    char **image_paths;          // 存储图片路径的数组
    size_t image_paths_size;     // 当前存储的图片数量
    size_t image_paths_capacity; // 数组容量
} storage_state_t;

 
  
//----------------------------------------------- Video UVC -------------------------------------------------
// Video
#define BUFFER_COUNT 2
#define CAM_DEV_PATH ESP_VIDEO_MIPI_CSI_DEVICE_NAME

#define SKIP_STARTUP_FRAME_COUNT 2

#define CONFIG_EXAMPLE_H264_I_PERIOD 120
#define CONFIG_EXAMPLE_H264_BITRATE 1000000
#define CONFIG_EXAMPLE_H264_MIN_QP 25
#define CONFIG_EXAMPLE_H264_MAX_QP 26

#define CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY 80

// uvc相关
#if CONFIG_FORMAT_MJPEG_CAM1
#define ENCODE_DEV_PATH ESP_VIDEO_JPEG_DEVICE_NAME
#define UVC_OUTPUT_FORMAT V4L2_PIX_FMT_JPEG
#elif CONFIG_FORMAT_H264_CAM1

#define ENCODE_DEV_PATH ESP_VIDEO_H264_DEVICE_NAME
#define UVC_OUTPUT_FORMAT V4L2_PIX_FMT_H264
#endif

typedef struct device_ctx {
    int cap_fd;
    uint32_t format;
    uint8_t *cap_buffer[BUFFER_COUNT];

    int m2m_fd;
    uint8_t *m2m_cap_buffer;

    uvc_fb_t fb;

    sd_card_fb_t sd_fb;
    sdmmc_card_t *card;

    const esp_video_init_config_t *cam_config; // 配置指针

#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_handle_t pwr_ctrl_handle; /*!< Power control handle */
#endif

} device_ctx_t;



//----------------------------------------------- funtion -------------------------------------------------

// 函数声明
void delay_ms(int nms);
void check_memory(void) ;

void bsp_restart_print(void) ;
#endif // VC_CONFIG_H
