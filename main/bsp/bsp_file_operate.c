#include "bsp_file_operate.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#define TAG "FILE_OPS"
#define BUFFER_SIZE  (1024 *16)

// 创建目录（包括父目录）
static int create_dir_recursive(const char *path) {
    char *tmp = strdup(path);
    char *p = tmp;
    int ret = 0;

    while (*p != '\0') {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0775)) {
                if (errno != EEXIST) {
                    ret = -1;
                    break;
                }
            }
            *p = '/';
        }
        p++;
    }

    if (ret == 0 && mkdir(tmp, 0775)) {
        if (errno != EEXIST)
            ret = -1;
    }

    free(tmp);
    return ret;
}

uint8_t file_copy(copy_callback_t callback,
                  const char *src_path,
                  const char *dst_path,
                  uint32_t total_size,
                  uint32_t copied_size,
                  uint8_t overwrite) {

    FILE *src = fopen(src_path, "rb");
    if (!src) {
        ESP_LOGE(TAG, "Failed to open source: %s", src_path);
        return 1;
    }

    // 创建目标目录
    char *dir_path = strdup(dst_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        create_dir_recursive(dir_path);
    }
    free(dir_path);

    FILE *dst = fopen(dst_path, overwrite ? "wb" : "wx");
    if (!dst) {
        fclose(src);
        ESP_LOGE(TAG, "Failed to open destination: %s", dst_path);
        return 2;
    }

    // 获取文件大小
    fseek(src, 0, SEEK_END);
    long file_size = ftell(src);
    fseek(src, 0, SEEK_SET);

    if (total_size == 0) {
        total_size = file_size;
    }

    // uint8_t *buffer = (uint8_t *)pvPortMalloc(BUFFER_SIZE);
    uint8_t *buffer  = (uint8_t *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!buffer) {
        fclose(src);
        fclose(dst);
        return 100;
    }

    uint8_t result = 0;
    uint32_t bytes_copied = 0;
    uint8_t last_percent = 0;

    while (bytes_copied < file_size) {
        size_t to_read = (file_size - bytes_copied > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - bytes_copied);
        size_t read = fread(buffer, 1, to_read, src);
        if (read <= 0)
            break;

        size_t written = fwrite(buffer, 1, read, dst);
        if (written != read) {
            result = 3;
            break;
        }

        bytes_copied += written;
        copied_size += written;

        uint8_t percent = (copied_size * 100) / total_size;
        if (percent != last_percent) {
            last_percent = percent;
            ESP_LOGI(TAG, "progress [%s]: %d%%", src_path, percent);
            if (callback && callback(src_path, percent, 0x02)) {
                result = 0xFF;
                break;
            }
        }

        // 加一点延迟可避免 Watchdog 重启
        vTaskDelay(1);
    }

    fclose(src);
    fclose(dst);
    vPortFree(buffer);
    return result;
}

// 递归复制目录
void recursive_copy(const char *src_base, const char *dst_base) {
    DIR *dir = opendir(src_base);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open dir: %s", src_base);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过特殊目录
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 分配路径缓冲区
        char *src_path = malloc(512);
        char *dst_path = malloc(512);
        if (!src_path || !dst_path) {
            ESP_LOGE(TAG, "Memory allocation failed");
            free(src_path);
            free(dst_path);
            closedir(dir);
            return;
        }

        snprintf(src_path, 512, "%s/%s", src_base, entry->d_name);
        snprintf(dst_path, 512, "%s/%s", dst_base, entry->d_name);

        printf("src_path: %s\n", src_path);
        printf("dst_path: %s\n", dst_path);

        struct stat stat_buf;
        if (stat(src_path, &stat_buf) != 0) {
            free(src_path);
            free(dst_path);
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            // 创建目标目录
            create_dir_recursive(dst_path);
            // 递归复制子目录
            recursive_copy(src_path, dst_path);
        } else {
            ESP_LOGI(TAG, "Copying %s -> %s", src_path, dst_path);
            uint8_t ret = file_copy(NULL, src_path, dst_path, 0, 0, 1);
            if (ret != 0) {
                ESP_LOGE(TAG, "Copy failed: 0x%02X", ret);
            }
        }

        free(src_path);
        free(dst_path);
    }

    closedir(dir);
}

// 递归删除目录
void recursive_delete(const char *path) {
    DIR *dir = opendir(path);
    if (!dir)
        return;
    char full_path[512];
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat stat_buf;
        if (stat(full_path, &stat_buf) != 0)
            continue;

        if (S_ISDIR(stat_buf.st_mode)) {
            recursive_delete(full_path);
        } else {
            if (unlink(full_path)) {
                ESP_LOGE(TAG, "Failed to delete: %s", full_path);
            }
        }
    }

    closedir(dir);

    // 删除目录本身
    if (rmdir(path)) {
        ESP_LOGE(TAG, "Failed to delete dir: %s", path);
    }
}