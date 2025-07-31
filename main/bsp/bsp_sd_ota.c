#include "bsp_sd_ota.h"

static const char *TAG = "BSP_SD_OTA";
#define OTA_BUF_SIZE (1024 * 40)
extern device_ctx_t *device_ctx;

esp_err_t sd_ota_check_file_exists(void) {
    const char *file_path = FIRMWARE_PATH;
    struct stat st;

    if (stat(file_path, &st)) {
        if (errno == ENOENT) {
            ESP_LOGI(TAG, "No Update .bin");
            return ESP_ERR_NOT_FOUND;
        } else {
            ESP_LOGE(TAG, "File check error [%d]: %s", errno, strerror(errno));
            return ESP_FAIL;
        }
    }

    if (!S_ISREG(st.st_mode)) {
        ESP_LOGE(TAG, "Not a regular file: %s", file_path);
        return ESP_ERR_INVALID_ARG;
    }

    if (st.st_size == 0) {
        ESP_LOGE(TAG, "Empty firmware file: %s", file_path);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

static esp_err_t write_ota_data(esp_ota_handle_t ota_handle, FILE *f) {
    printf("write ota data\n");

    uint8_t *buf = (uint8_t *)heap_caps_malloc(OTA_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!buf) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    size_t read_bytes;

    while ((read_bytes = fread(buf, 1, OTA_BUF_SIZE, f))) {
        ret = esp_ota_write(ota_handle, buf, read_bytes);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(ret));
            break;
        }
    }

    free(buf);
    return ret;
}

int bsp_sd_ota_check(void) {
    // 步骤1: 检查固件文件是否存在且有效
    esp_err_t file_status = sd_ota_check_file_exists();
    if (file_status != ESP_OK) {
        // 文件不存在或无效，无需继续
        return -1;
    }

    ESP_LOGI(TAG, "==================Update Firmware  Start==================");

    // 步骤2: 打开固件文件
    const char *file_path = FIRMWARE_PATH;
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open firmware file: %s", strerror(errno));
        return -1;
    }

    // 步骤3: 初始化OTA更新
    ESP_LOGI(TAG, "Starting OTA update");
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t ota_handle;

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        fclose(f);
        return -1;
    }

    // 步骤4: 写入OTA数据
    err = write_ota_data(ota_handle, f);
    fclose(f); // 无论成功与否都关闭文件

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed, aborting update");
        esp_ota_abort(ota_handle);
        return -1;
    }

    // 步骤5: 完成OTA更新
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA end failed: %s", esp_err_to_name(err));
        return -1;
    }

    // 步骤6: 设置启动分区
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Set boot partition failed: %s", esp_err_to_name(err));
        return -1;
    }

    // 步骤7: 清理并重启
    if (remove(file_path)) {
        ESP_LOGW(TAG, "Delete firmware failed: %s", strerror(errno));
    } else {
        ESP_LOGI(TAG, "Firmware file deleted");
    }

    return 1; // OTA更新成功

}