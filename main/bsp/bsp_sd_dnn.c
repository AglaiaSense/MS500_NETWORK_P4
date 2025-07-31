#include "bsp_sd_dnn.h"
#include "bsp_file_operate.h"
#include "esp_log.h"
#include "vc_config.h"
#include "nvs.h"
#define TAG "SD_DNN"

#define NVS_KEY_MODEL_UPDATE_PENDING "model_update"

extern nvs_handle_t my_nvs_handle;

#define SPIFFS_MOUNT_POINT "/download"
#define DNN_DIR_NAME "spiffs_download"  //  


// 检查更新状态
int check_update_status() {
    char sd_spiffs_path[128];
    snprintf(sd_spiffs_path, sizeof(sd_spiffs_path), "%s/%s", SD_MOUNT_POINT, DNN_DIR_NAME);

    struct stat st;
    if (stat(sd_spiffs_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        ESP_LOGI(TAG, "No Update SPIFFS ");
        return 0;
    }

    ESP_LOGI(TAG, "Update found, starting copy process...");
    return 1;
}
void copy_update() {
    // 复制SPIFFS内容
    char src_path[64], dst_path[64];
    snprintf(src_path, sizeof(src_path), "%s/%s", SD_MOUNT_POINT, DNN_DIR_NAME);
    snprintf(dst_path, sizeof(dst_path), "%s", SPIFFS_MOUNT_POINT);
    printf("src_path: %s\r\n", src_path);
    printf("dst_path: %s\r\n", dst_path);

    recursive_copy(src_path, dst_path);
}
// 完成更新清理
void remove_dnn_dir() {

    char sd_spiffs_path[128];
    snprintf(sd_spiffs_path, sizeof(sd_spiffs_path), "%s/%s", SD_MOUNT_POINT, DNN_DIR_NAME);

    recursive_delete(sd_spiffs_path);
}

esp_err_t clear_model_update_pending() {
    esp_err_t err = nvs_erase_key(my_nvs_handle, NVS_KEY_MODEL_UPDATE_PENDING);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_commit(my_nvs_handle);
    }
    return err;
}

static int check_nvs_model_update_pending() {
    uint8_t pending = 0;
    esp_err_t err = nvs_get_u8(my_nvs_handle, NVS_KEY_MODEL_UPDATE_PENDING, &pending);
    printf("check_nvs_model_update_pending: %d\n", pending);
    
    if (err == ESP_OK && pending == 1) {
        ESP_LOGI(TAG, "Found pending model update from USB CDC");
        return 2; // 返回2表示有待处理的模型更新
    }
    return 0;
}

int bsp_sd_dnn_check() {
    // 1. 首先检查NVS中是否有待处理的模型更新
    int nvs_status = check_nvs_model_update_pending();
    printf("nvs_status: %d\n", nvs_status);
    printf("------------------------------------------------------------------------------------\n");
    
    if (nvs_status == 2) {
        ESP_LOGI(TAG, "==================Update AI Model from USB CDC Start==================");

        return 2;
    }

    // 2. 检查SD卡更新状态
    int status = check_update_status();

    // 3. 执行更新
    if (status == 1) {
        ESP_LOGI(TAG, "==================Update AI Model from SD Card Start==================");
        copy_update();
    }
    return status;
}