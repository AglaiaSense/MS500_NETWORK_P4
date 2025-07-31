#include "bsp_spiff.h"
#include "esp_log.h"
#include "esp_partition.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "wear_levelling.h"

static const char *TAG = "BSP_SPIFF";

// ----------------------- print ------------------------------------------

void list_files_in_spiffs(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                ESP_LOGI(TAG, "File: %s (%ld bytes)", full_path, st.st_size);
            } else if (S_ISDIR(st.st_mode)) {
                ESP_LOGI(TAG, "Directory: %s/", full_path);
                // 递归进入子目录（可选）
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    list_files_in_spiffs(full_path);
                }
            }
        } else {
            ESP_LOGW(TAG, "Failed to stat: %s", full_path);
        }
    }

    closedir(dir);
}

// 分区使用统计
void print_spiffs_usage(const char *partition_label) {
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get info for %s: %s",
                 partition_label, esp_err_to_name(ret));
        return;
    }
    ESP_LOGW(TAG, "Partition %s: Total=%dKB, Used=%dKB, Free=%dKB",
             partition_label,
             total / 1024,
             used / 1024,
             (total - used) / 1024);
}

#include "esp_vfs_fat.h"

void log_fatfs_info(const char *mount_path) {
    uint64_t total_bytes = 0, free_bytes = 0;

    esp_err_t err = esp_vfs_fat_info(mount_path, &total_bytes, &free_bytes);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_fat_info failed for %s: %s", mount_path, esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "FATFS mount: %s", mount_path);
    ESP_LOGI(TAG, "Total: %llu KB", total_bytes / 1024);
    ESP_LOGI(TAG, "Free: %llu KB", free_bytes / 1024);
}

void print_all_spiffs_usage(void) {
    print_spiffs_usage("storage");
    list_files_in_spiffs("/spiffs");

    log_fatfs_info("/download");

    list_files_in_spiffs("/download");
}

// ----------------------- init ------------------------------------------
// 初始化常规存储分区
void bsp_spiff_init_main(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false};

    ESP_LOGW(TAG, "Initializing MAIN storage partition");

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "%s partition mount failed: %s",
                 conf.partition_label, esp_err_to_name(ret));
    }
}

// 初始化下载专用分区
// void bsp_spiff_init_download(void) {
//     esp_vfs_spiffs_conf_t conf = {
//         .base_path = "/download",
//         .partition_label = "storage_dl",
//         .max_files = 2, // 下载分区只需要2个文件
//         .format_if_mount_failed = false};

//     ESP_LOGW(TAG, "Initializing DOWNLOAD partition");

//     esp_err_t ret = esp_vfs_spiffs_register(&conf);
//     if (ret != ESP_OK) {
//         ESP_LOGW(TAG, "%s partition mount failed: %s",
//                  conf.partition_label, esp_err_to_name(ret));
//     }else{
//         ESP_LOGW(TAG, "%s partition mounted", conf.partition_label);
//         //   mkdir("/download", 0777);
//     }
// }
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
void bsp_fatfs_init_download(void) {
    ESP_LOGW(TAG, "Mounting FATFS on /download (storage_dl)");

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,    // 是否自动格式化（初次使用建议 true）
        .disk_status_check_enable = false, // 禁用状态检查
        .max_files = 5,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};

    esp_err_t ret = esp_vfs_fat_spiflash_mount("/download", "storage_dl", &mount_config, &s_wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATFS mount failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGW(TAG, "FATFS mounted on /download");
    }
}

// 主初始化函数
void bsp_spiff_init(void) {
    bsp_spiff_init_main();
    bsp_fatfs_init_download();

    print_all_spiffs_usage();
}

// 卸载所有分区
void bsp_spiff_uninit(void) {
    esp_vfs_spiffs_unregister("storage");
    // esp_vfs_spiffs_unregister("storage_dl");
    esp_vfs_fat_spiflash_unmount("/download", s_wl_handle);

    ESP_LOGW(TAG, "All SPIFFS partitions unmounted");
}

// ----------------------- format ------------------------------------------
// 清空下载分区
void bsp_spiff_format_download(void) {
    ESP_LOGW(TAG, "Formatting download partition...");

    esp_spiffs_format("storage_dl");

    // 重新挂载分区
    esp_vfs_spiffs_unregister("storage_dl");
    bsp_fatfs_init_download();

    ESP_LOGW(TAG, "Download partition reformatted");
}