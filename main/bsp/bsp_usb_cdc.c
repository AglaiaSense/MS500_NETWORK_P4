// bsp_usb_cdc.c
#include "bsp_usb_cdc.h"
#include "bsp_spiff.h"
#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/md5.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "tusb.h"
#include "vc_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *TAG = "bsp_usb_cdc";

const uint8_t itf = 0; // CDC 接口编号

extern int32_t plate_max_count;
extern nvs_handle_t my_nvs_handle;

extern int is_sleeping;


extern int runtime_max_sec; // 如果传输文件增大睡眠时间

extern void bsp_spiff_format_download(void);
extern void print_all_spiffs_usage();

// NVS keys for update status
#define NVS_KEY_MODEL_UPDATE_PENDING "model_update"

// ---------- 文件传输状态 ----------
typedef enum {
    UPDATE_TYPE_NONE,
    UPDATE_TYPE_FIRMWARE, // updata_bin - ms500_p4.bin
    UPDATE_TYPE_AI_INFO,  // down_ai_info - network_info.txt (2KB)
    UPDATE_TYPE_AI_FPK    // down_ai_fpk - network.fpk (4197KB)
} update_type_t;

typedef enum {
    UPDATE_PHASE_NONE,
    UPDATE_PHASE_DOWNLOAD, // 下载阶段
    UPDATE_PHASE_COPY,     // 复制阶段
    UPDATE_PHASE_UPDATE    // 更新阶段
} update_phase_t;

static update_type_t current_update_type = UPDATE_TYPE_NONE;
static update_phase_t current_update_phase = UPDATE_PHASE_NONE;
static bool file_transfer_mode = false;
static size_t file_expected_size = 0;
static size_t file_received_size = 0;
static char target_file_path[128] = {0}; // 只保留目标文件路径
static char expected_md5[33] = {0};      // 期望的MD5值
static char current_filename[64] = {0};  // 当前文件名

// 固件升级专用
static esp_ota_handle_t update_handle = 0;
static const esp_partition_t *update_partition = NULL;

// 模型升级专用 - PSRAM缓冲区
#define MODEL_BUFFER_SIZE (5 * 1024 * 1024) // 5MB PSRAM缓冲区
static uint8_t *model_buffer = NULL;
static size_t model_buffer_pos = 0;

// AI文件下载专用 - 内存缓冲区
static uint8_t *ai_file_buffer = NULL;
static size_t ai_file_buffer_pos = 0;
static size_t ai_file_buffer_size = 0;

// ---------- NVS 读写 ----------
static esp_err_t load_plate_max_count_from_nvs() {
    esp_err_t err = nvs_get_i32(my_nvs_handle, "plate_max_count", &plate_max_count);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        plate_max_count = 5;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read plate_max_count: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGW(TAG, "plate_max_count: %ld", plate_max_count);
    return ESP_OK;
}

static esp_err_t save_plate_max_count_to_nvs(int value) {
    plate_max_count = value;
    esp_err_t err = nvs_set_i32(my_nvs_handle, "plate_max_count", value);
    if (err == ESP_OK) {
        err = nvs_commit(my_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to commit plate_max_count: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to write plate_max_count: %s", esp_err_to_name(err));
    }
    return err;
}

// ---------- 工具函数 ----------
static const char *get_json_string(cJSON *json, const char *key) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsString(item)) {
        return item->valuestring;
    } else {
        ESP_LOGW(TAG, "Missing or invalid field: %s", key);
        return NULL;
    }
}

static void send_json_response(const char *status, const char *msg) {
    char response[256];
    snprintf(response, sizeof(response),
             "{\"status\":\"%s\",\"msg\":\"%s\"}\n", status, msg);
    tud_cdc_n_write(itf, response, strlen(response));
    tud_cdc_n_write_flush(itf);
}

// 计算内存数据的MD5
static bool calculate_memory_md5(const uint8_t *data, size_t size, char *md5_output) {
    if (!data || !md5_output || size == 0) {
        ESP_LOGE(TAG, "Invalid parameters for MD5 calculation");
        return false;
    }

    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, data, size);

    unsigned char md5_hash[16];
    mbedtls_md5_finish(&md5_ctx, md5_hash);
    mbedtls_md5_free(&md5_ctx);

    // 转换为十六进制字符串
    for (int i = 0; i < 16; i++) {
        sprintf(md5_output + (i * 2), "%02x", md5_hash[i]);
    }
    md5_output[32] = '\0';

    return true;
}

// ---------- 命令处理 ----------
static void handle_image_count(cJSON *json) {
    const char *param_str = get_json_string(json, "param");
    if (!param_str) {
        send_json_response("error", "Missing param");
        return;
    }

    char *endptr;
    long count = strtol(param_str, &endptr, 10);
    if (endptr != param_str && *endptr == '\0') {
        save_plate_max_count_to_nvs(count);
        send_json_response("ok", "plate_max_count updated");
    } else {
        send_json_response("error", "Invalid number");
    }
}

static void handle_reboot(cJSON *json) {
    send_json_response("ok", "Rebooting");
    bsp_restart_print();
}

// 通用AI文件下载处理函数
static void handle_ai_file_download(const char *command_name, const char *filename,
                                    const char *file_path, update_type_t update_type,
                                    cJSON *json) {
    const char *size_str = get_json_string(json, "size");
    const char *md5_str = get_json_string(json, "md5");

    if (!size_str || !md5_str) {
        send_json_response("error", "Missing parameters");
        return;
    }

    char *endptr;
    long size = strtol(size_str, &endptr, 10);
    if (endptr == size_str || *endptr != '\0' || size <= 0) {
        send_json_response("error", "Invalid size");
        return;
    }

    // 设置存储路径
    snprintf(target_file_path, sizeof(target_file_path), "%s", file_path);

    // 释放旧的缓冲区（如果存在）
    if (ai_file_buffer) {
        free(ai_file_buffer);
        ai_file_buffer = NULL;
    }

    // 申请内存缓冲区
    ai_file_buffer_size = size;
    ai_file_buffer = malloc(ai_file_buffer_size);
    if (!ai_file_buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for %s: %ld bytes", filename, size);
        send_json_response("error", "Failed to allocate memory");
        return;
    }
    ai_file_buffer_pos = 0;

    // 保存文件信息
    strncpy(current_filename, filename, sizeof(current_filename) - 1);
    strncpy(expected_md5, md5_str, sizeof(expected_md5) - 1);
    file_expected_size = size;
    file_received_size = 0;

    // 设置传输模式
    current_update_type = update_type;
    current_update_phase = UPDATE_PHASE_DOWNLOAD;
    file_transfer_mode = true;

    ESP_LOGI(TAG, "%s prepare: %s, size: %ld, md5: %s, buffer allocated",
             command_name, filename, size, md5_str);
    send_json_response("ok", "Ready to receive file");
}

// 处理down_ai_info命令
static void handle_down_ai_info(cJSON *json) {
    handle_ai_file_download("down_ai_info", "network_info.txt",
                            "/download/dnn/network_info.txt", UPDATE_TYPE_AI_INFO, json);
}

// 处理down_ai_fpk命令
static void handle_down_ai_fpk(cJSON *json) {
    handle_ai_file_download("down_ai_fpk", "network.fpk",
                            "/download/dnn/network.fpk", UPDATE_TYPE_AI_FPK, json);
}

// 处理updata_bin命令（固件更新）
static void handle_updata_bin(cJSON *json) {
    const char *size_str = get_json_string(json, "size");
    if (!size_str) {
        send_json_response("error", "Missing size");
        return;
    }

    char *endptr;
    long size = strtol(size_str, &endptr, 10);
    if (endptr == size_str || *endptr != '\0' || size <= 0) {
        send_json_response("error", "Invalid size");
        return;
    }

    // 获取OTA更新分区
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        send_json_response("error", "No OTA partition");
        return;
    }

    // 开始OTA更新
    esp_err_t err = esp_ota_begin(update_partition, size, &update_handle);
    if (err != ESP_OK) {
        send_json_response("error", "OTA begin failed");
        return;
    }

    current_update_type = UPDATE_TYPE_FIRMWARE;
    current_update_phase = UPDATE_PHASE_DOWNLOAD;
    file_expected_size = size;
    file_received_size = 0;
    file_transfer_mode = true;

    ESP_LOGI(TAG, "updata_bin ready, size: %ld", size);
    send_json_response("ok", "Ready to receive firmware");
}

// 处理update_ai命令（AI模型更新）- 保留以备手动触发
static void handle_update_ai(void) {
    ESP_LOGI(TAG, "Received manual update_ai command");

    // 设置模型更新标记
    uint8_t pending = 1;
    esp_err_t nvs_err = nvs_set_u8(my_nvs_handle, NVS_KEY_MODEL_UPDATE_PENDING, pending);

    if (nvs_err == ESP_OK) {
        nvs_commit(my_nvs_handle);

        printf("Manual AI model update scheduled, rebooting\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
}

// 处理数据接收
static void handle_data_reception(uint8_t *buf, size_t count) {
    if (current_update_type == UPDATE_TYPE_FIRMWARE) {
        // 固件更新：直接写入OTA分区
        esp_err_t err = esp_ota_write(update_handle, buf, count);
        if (err != ESP_OK) {
            send_json_response("error", "OTA write failed");
            esp_ota_abort(update_handle);
            file_transfer_mode = false;
            current_update_type = UPDATE_TYPE_NONE;
            return;
        }
    } else if (current_update_type == UPDATE_TYPE_AI_INFO || current_update_type == UPDATE_TYPE_AI_FPK) {
        // AI文件下载：使用内存缓冲区
        if (ai_file_buffer && (ai_file_buffer_pos + count <= ai_file_buffer_size)) {
            memcpy(ai_file_buffer + ai_file_buffer_pos, buf, count);
            ai_file_buffer_pos += count;
        } else {
            ESP_LOGE(TAG, "AI file buffer overflow or not allocated: pos=%zu, size=%zu, count=%zu",
                     ai_file_buffer_pos, ai_file_buffer_size, count);
            send_json_response("error", "AI file buffer overflow");
            if (ai_file_buffer) {
                free(ai_file_buffer);
                ai_file_buffer = NULL;
            }
            file_transfer_mode = false;
            current_update_type = UPDATE_TYPE_NONE;
            return;
        }
    }
}

// 处理下载完成
static void handle_download_complete() {
    esp_err_t err = ESP_OK;

    if (current_update_type == UPDATE_TYPE_FIRMWARE) {
        // 完成固件OTA更新
        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
            send_json_response("error", "OTA end failed");
        } else {
            err = esp_ota_set_boot_partition(update_partition);
            if (err != ESP_OK) {
                send_json_response("error", "Set boot partition failed");
            } else {
                send_json_response("ok", "Firmware update complete, rebooting");
                printf("Firmware update complete, rebooting\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
        }
    } else if (current_update_type == UPDATE_TYPE_AI_INFO || current_update_type == UPDATE_TYPE_AI_FPK) {
        // AI文件下载完成，进行MD5校验并保存到文件系统
        ESP_LOGW(TAG, "AI file download complete: %s (%zu bytes)", current_filename, file_received_size);

        if (ai_file_buffer && ai_file_buffer_pos == file_expected_size) {
            // 计算内存中数据的MD5
            char calculated_md5[33];
            if (!calculate_memory_md5(ai_file_buffer, ai_file_buffer_pos, calculated_md5)) {
                ESP_LOGE(TAG, "Failed to calculate MD5 for %s", current_filename);
                free(ai_file_buffer);
                ai_file_buffer = NULL;
                send_json_response("error", "MD5 calculation failed");
                return;
            }

            // MD5校验
            if (strcmp(calculated_md5, expected_md5) != 0) {
                ESP_LOGE(TAG, "MD5 mismatch for %s! Expected: %s, Got: %s",
                         current_filename, expected_md5, calculated_md5);
                free(ai_file_buffer);
                ai_file_buffer = NULL;
                send_json_response("error", "MD5 verification failed");
            } else {
                // MD5校验通过，保存到文件系统
                ESP_LOGI(TAG, "MD5 verification passed, saving to file system: %s", target_file_path);

                // 确保目录存在
                mkdir("/download", 0777);
                mkdir("/download/dnn", 0777);

                // 删除旧文件（如果存在）
                if (access(target_file_path, F_OK) == 0) {
                    remove(target_file_path);
                }

                // 创建文件并写入数据
                FILE *file = fopen(target_file_path, "wb");
                if (!file) {
                    ESP_LOGE(TAG, "Failed to create file: %s", target_file_path);
                    free(ai_file_buffer);
                    ai_file_buffer = NULL;
                    send_json_response("error", "Failed to create file");
                } else {
                    // 分块写入数据
                    const size_t chunk_size = 64 * 1024; // 64KB chunks
                    size_t total_written = 0;
                    size_t remaining = ai_file_buffer_pos;

                    while (remaining > 0) {
                        size_t write_size = (remaining > chunk_size) ? chunk_size : remaining;
                        size_t written = fwrite(ai_file_buffer + total_written, 1, write_size, file);

                        if (written != write_size) {
                            ESP_LOGE(TAG, "File write failed at offset %zu: wrote %zu/%zu bytes",
                                     total_written, written, write_size);
                            fclose(file);
                            remove(target_file_path);
                            free(ai_file_buffer);
                            ai_file_buffer = NULL;
                            send_json_response("error", "File write incomplete");
                            return;
                        }

                        total_written += written;
                        remaining -= written;

                        // 每写入1MB显示进度（只对大文件）
                        if (ai_file_buffer_size > 1024 * 1024 && (total_written % (1024 * 1024)) == 0) {
                            ESP_LOGW(TAG, "File write progress: %zu/%zu MB",
                                     total_written / (1024 * 1024), ai_file_buffer_pos / (1024 * 1024));
                        }
                    }

                    fclose(file);
                    free(ai_file_buffer);
                    ai_file_buffer = NULL;

                    ESP_LOGI(TAG, "File %s saved successfully: %zu bytes", current_filename, total_written);

                    // 如果是network.fpk文件，下载完成后自动进行AI模型更新
                    if (current_update_type == UPDATE_TYPE_AI_FPK) {
                        ESP_LOGI(TAG, "network.fpk saved, starting AI model update automatically");
                        send_json_response("ok", "File saved, starting AI model update");
                        handle_update_ai();
                    } else {
                        send_json_response("ok", "File download and verification complete");
                    }
                }
            }
        } else {
            ESP_LOGE(TAG, "AI file buffer error: buffer=%p, pos=%zu, expected=%zu",
                     ai_file_buffer, ai_file_buffer_pos, file_expected_size);
            if (ai_file_buffer) {
                free(ai_file_buffer);
                ai_file_buffer = NULL;
            }
            send_json_response("error", "File buffer error");
        }
    }
}
//  #define FILE_BUF_SIZE (128 * 1024)
static void handle_file_transfer_data(uint8_t *buf, size_t count) {
    // 处理数据接收
    handle_data_reception(buf, count);

    file_received_size += count;

    // 发送进度更新（每1%发送一次）
    static int last_progress = -1;
    int progress = (file_received_size * 100) / file_expected_size;
    if (progress != last_progress) {
        ESP_LOGW(TAG, "progress: %d%% (%d/%d bytes)", progress, (int)file_received_size, (int)file_expected_size);
        last_progress = progress;
    }

    // 文件下载完成
    if (file_received_size >= file_expected_size) {
        ESP_LOGW(TAG, "File download complete: %zu bytes", file_received_size);

        // 处理下载完成
        handle_download_complete();

        // 重置状态
        file_transfer_mode = false;
        current_update_type = UPDATE_TYPE_NONE;
        current_update_phase = UPDATE_PHASE_NONE;
    }
}

static void dispatch_command(const char *cmd, cJSON *json) {
    if (strcmp(cmd, "image_count") == 0) {
        handle_image_count(json);
    } else if (strcmp(cmd, "reboot") == 0) {
        handle_reboot(json);
    } else if (strcmp(cmd, "updata_bin") == 0) {
        handle_updata_bin(json);
        runtime_max_sec += 60 * 5;
    } else if (strcmp(cmd, "down_ai_info") == 0) {
        handle_down_ai_info(json);
        runtime_max_sec += 60 * 5;
    } else if (strcmp(cmd, "down_ai_fpk") == 0) {
        handle_down_ai_fpk(json);
        runtime_max_sec += 60 * 5;
    } else {
        send_json_response("error", "Unknown command");
    }
}

static void process_json_command(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        const char *err = cJSON_GetErrorPtr();
        ESP_LOGE(TAG, "JSON parse error: %s", err ? err : "Unknown");
        return;
    }

    const char *cmd = get_json_string(root, "command");
    if (cmd) {
        printf("command: %s\n", cmd);
        dispatch_command(cmd, root);
    }

    cJSON_Delete(root);
}

// ---------- CDC 任务 ----------
#define CDC_BUF_SIZE (30 * 1024) // 20KB

static void cdc_task(void *arg) {
    uint8_t *buf = malloc(CDC_BUF_SIZE);
    if (!buf) {
        ESP_LOGE("cdc_task", "Failed to allocate buffer");
        vTaskDelete(NULL);
        return;
    }

    while (is_sleeping == 0) {
        if (tud_cdc_n_available(itf)) {
            size_t count = tud_cdc_n_read(itf, buf, CDC_BUF_SIZE);
            if (count > 0) {
                // ---------- 文件传输处理 ----------
                if (file_transfer_mode) {
                    handle_file_transfer_data(buf, count);
                    continue;
                }

                // ---------- 普通命令处理 ----------
                // 为安全起见，拷贝到本地小缓冲区用于解析JSON或回显
                if (count < CDC_BUF_SIZE) {
                    buf[count] = '\0'; // 添加结尾用于字符串处理（安全前提是 count < 20KB）

                    if (buf[0] == '{' && buf[count - 1] == '}') {
                        process_json_command((const char *)buf);
                    } else {
                        printf("Unsupported format\n");
                        send_json_response("error", "Unsupported format");

                        char echo[256];
                        snprintf(echo, sizeof(echo), "Echo: %.*s", (int)count, buf);
                        printf("echo: %s\n", echo);
                    }
                } else {
                    ESP_LOGW("cdc_task", "Received data too large for command parsing");
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Never reached, but for completeness
    free(buf);
    vTaskDelete(NULL);
}

static TaskHandle_t usb_cdc_task_handle = NULL; // 全局任务句柄
void bsp_usb_cdc_init(void) {
    xTaskCreate(cdc_task, "cdc_task", 8192, NULL, 4, &usb_cdc_task_handle);

    esp_err_t err = load_plate_max_count_from_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Using default plate_max_count: %ld", plate_max_count);
    }
}

void bsp_usb_cdc_deinit(void) {
    if (usb_cdc_task_handle != NULL) {
        vTaskDelete(usb_cdc_task_handle); // 删除任务
        usb_cdc_task_handle = NULL;       // 重置句柄
    }
}