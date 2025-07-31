#include "bsp_storage.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "vc_config.h"
#include <stdio.h>
#include <sys/stat.h>

#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "BSP_STORAGE";
nvs_handle_t my_nvs_handle;
bool is_jpg_writeing = false;

 
storage_state_t g_storage_state = {
    .wake_count = 0,
    .image_count = 0,
    .image_paths = NULL,
    .image_paths_size = 0,
    .image_paths_capacity = 0
};

// ----------------------------------  path name----------------------------------

// 添加打印状态函数
void print_storage_state() {
    ESP_LOGI(TAG, "Storage State:");
    ESP_LOGI(TAG, "  wake_count: %ld", g_storage_state.wake_count);
    ESP_LOGI(TAG, "  image_count: %ld", g_storage_state.image_count);
    ESP_LOGI(TAG, "  image_paths_size: %d", g_storage_state.image_paths_size);
    ESP_LOGI(TAG, "  image_paths_capacity: %d", g_storage_state.image_paths_capacity);

    for (size_t i = 0; i < g_storage_state.image_paths_size; i++) {
        ESP_LOGI(TAG, "  image_paths[%d]: %s", i, g_storage_state.image_paths[i]);
    }
}

// / 初始化NVS存储
esp_err_t init_storage_system() {
    esp_err_t err;

    // 初始化NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 打开NVS命名空间
    err = nvs_open("storage", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return err;
    }

    // 初始化状态
    memset(&g_storage_state, 0, sizeof(storage_state_t));

    // 从NVS读取唤醒次数
    err = nvs_get_i32(my_nvs_handle, "wake_count", &g_storage_state.wake_count);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        g_storage_state.wake_count = 0; // 默认值
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading wake_count: %s", esp_err_to_name(err));
        return err;
    }

    // 创建基础目录
    char base_dir[32];
    for (int i = 1; i <= MAX_FOLDERS; i++) {
        snprintf(base_dir, sizeof(base_dir), "%s/jpg%d", SD_MOUNT_POINT, i);
        mkdir(base_dir, 0777);
    }

    // 初始化路径数组
    g_storage_state.image_paths_capacity = MAX_IMAGES_PER_FOLDER;
    g_storage_state.image_paths = malloc(g_storage_state.image_paths_capacity * sizeof(char *));
    if (!g_storage_state.image_paths) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    // 初始化本次唤醒的图片计数
    g_storage_state.image_count = 0;

    // print_storage_state();

    return ESP_OK;
}

// 唤醒事件处理
esp_err_t handle_wake_event() {

    // 增加唤醒次数
    g_storage_state.wake_count++;

    // 保存到NVS
    esp_err_t err = nvs_set_i32(my_nvs_handle, "wake_count", g_storage_state.wake_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving wake_count: %s", esp_err_to_name(err));
        return err;
    }
    nvs_commit(my_nvs_handle);

    // 重置图片计数
    g_storage_state.image_count = 0;

    // 清空路径数组（但不释放内存）
    for (size_t i = 0; i < g_storage_state.image_paths_size; i++) {
        free(g_storage_state.image_paths[i]);
    }
    g_storage_state.image_paths_size = 0;

    return ESP_OK;
}

#if 0
/**
 * @brief 获取JPG文件路径，每次调用会自动+1
 * @param file_path 用于存储生成的文件路径
 * @param file_path_size 文件路径缓冲区大小
 * @return esp_err_t 返回操作结果
 */
esp_err_t get_jpg_file_path(char *file_path, size_t file_path_size) {
    FILE *f = NULL;
    int file_count = 0;
    const int numMax = 3000; // 最大文件数
    const int numSort = 500; // 每个目录的文件数

    // 打开或创建list_jpg.txt文件
    const char *list_file = SD_MOUNT_POINT "/list_jpg.txt";
    f = fopen(list_file, "r");
    if (f != NULL) {
        // 读取文件中的数字
        if (fscanf(f, "%d", &file_count) != 1) {
            file_count = 0;
        }
        fclose(f);
    }

    // 文件计数加1，并检查最大值
    file_count++;
    if (file_count >= numMax || file_count <= 0) {
        file_count = 1;
    }

    // 更新list_jpg.txt文件
    f = fopen(list_file, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open list file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%d", file_count);
    fclose(f);

    // 计算文件夹编号
    int folder_index = (file_count - 1) / numSort + 1;

    // 生成文件夹路径
    char folder_path[48];
    snprintf(folder_path, sizeof(folder_path), "%s/jpg%d", SD_MOUNT_POINT, folder_index);

    // 创建文件夹（如果不存在）
    struct stat st;
    if (stat(folder_path, &st) == -1) {
        mkdir(folder_path, 0777);
    }

    // 生成完整文件路径
    snprintf(file_path, file_path_size, "%s/%d.jpg", folder_path, file_count);

    return ESP_OK;
}

#endif

#if 1
// 获取新的图片路径
esp_err_t get_jpg_file_path(char *file_path, size_t file_path_size) {
    // 检查是否在唤醒状态下
    if (g_storage_state.wake_count == 0) {
        ESP_LOGE(TAG, "Not in wake session");
        return ESP_FAIL;
    }

    // 计算目录编号（1-30）
    int folder_index = ((g_storage_state.wake_count - 1) / MAX_WAKES_PER_FOLDER) % MAX_FOLDERS + 1;

    // 生成完整文件路径（格式：/sdcard/jpg<文件夹编号>/<唤醒次数>_<本次唤醒内序号>.jpg）
    snprintf(file_path, file_path_size, "%s/jpg%d/%ld_%ld.jpg",
             SD_MOUNT_POINT, folder_index, g_storage_state.wake_count, g_storage_state.image_count);

    // 添加到路径数组
    if (g_storage_state.image_paths_size >= g_storage_state.image_paths_capacity) {
        // 扩容数组（初始容量设为1，每次增加10）
        size_t new_capacity = (g_storage_state.image_paths_capacity == 0) ? 10 : (g_storage_state.image_paths_capacity + 10);
        char **new_paths = realloc(g_storage_state.image_paths, new_capacity * sizeof(char *));
        if (!new_paths) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        g_storage_state.image_paths = new_paths;
        g_storage_state.image_paths_capacity = new_capacity;
    }

    // 复制路径到数组
    char *path_copy = strdup(file_path);
    if (!path_copy) {
        ESP_LOGE(TAG, "String duplication failed");
        return ESP_ERR_NO_MEM;
    }

    g_storage_state.image_paths[g_storage_state.image_paths_size++] = path_copy;
    g_storage_state.image_count++; // 更新本次唤醒的图片计数

    return ESP_OK;
}
#endif

// 清理资源
void cleanup_storage_system() {
    nvs_close(my_nvs_handle);

    for (size_t i = 0; i < g_storage_state.image_paths_size; i++) {
        free(g_storage_state.image_paths[i]);
    }
    free(g_storage_state.image_paths);

    memset(&g_storage_state, 0, sizeof(storage_state_t));
}
// ----------------------------------  JPG----------------------------------

uint8_t *embed_metadata(const uint8_t *data, size_t len, size_t *out_len) {
    const size_t extra_len = sizeof(Plate_result_t); // 车牌结果结构体大小

    *out_len = len + extra_len;

    // uint8_t *new_data = malloc(*out_len);
    uint8_t *new_data  = (uint8_t *)heap_caps_malloc(*out_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  	// printf("%s(%d): \n", __func__, __LINE__);
    // check_memory(); // 检查内存状态

    if (!new_data) {
        printf("Memory allocation failed!\n");
        *out_len = 0;
        return NULL;
    }

    // 拷贝原图
    memcpy(new_data, data, len);

    plate_result.image_count = g_storage_state.image_count;
    // 拷贝添加的元数据
    memcpy(new_data + len, &plate_result, sizeof(Plate_result_t));

    return new_data;
}

static esp_err_t example_write_file(FILE *f, const uint8_t *data, size_t len) {
    size_t written; // 记录每次写入的字节数

    // 循环写入数据，直到所有数据写入完成或写入失败
    do {
        // 将数据写入文件，每次最多写入len字节
        written = fwrite(data, 1, len, f);

        // 如果写入失败（written为0），立即返回错误
        if (written == 0) {
            ESP_LOGE(TAG, "Failed to write data to file");
            return ESP_FAIL;
        }

        // 更新剩余需要写入的字节数
        len -= written;
        // 移动数据指针到未写入的位置
        data += written;
    } while (written && len); // 当有数据写入且还有剩余数据时继续循环

    // 刷新文件缓冲区，确保数据写入存储设备
    fflush(f);

    // 返回成功状态
    return ESP_OK;
}

bool jpg_store_time_allow(int64_t min_interval_us) {
    static int64_t last_execution_time = 0;
    int64_t now = esp_timer_get_time();

    if (now - last_execution_time < min_interval_us) {
        return false; // 时间间隔不足，不允许执行
    }

    last_execution_time = now;
    return true; // 允许执行
}

esp_err_t store_jpg_to_sd_card(const uint8_t *data, size_t len) {
  
    // 检查当前是否空闲，不空闲则跳过
    if (is_jpg_writeing == true) {
        return ESP_OK;
    }

    esp_err_t ret;

    // 100ms保存一张图片
    if (!jpg_store_time_allow(100 * 1000)) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    is_jpg_writeing = true;

    char file_name_str[48] = {0};
    struct stat st;

    ret = get_jpg_file_path(file_name_str, sizeof(file_name_str));
    if (ret != ESP_OK) {

        goto cleanup;
    }
    ESP_LOGE(TAG, "file path: %s\n", file_name_str);

    if (stat(file_name_str, &st) == 0) {
        unlink(file_name_str);
    }

    FILE *f = fopen(file_name_str, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        ret = ESP_FAIL;
        goto cleanup;
    }

    // embed_metadata(data, len);

    size_t new_len = 0;
    uint8_t *new_data = embed_metadata(data, len, &new_len);
    if (!new_data) {
        ret = ESP_FAIL;
        fclose(f);
        goto cleanup;
    }

    example_write_file(f, new_data, new_len);
    fclose(f);
    heap_caps_free(new_data);  // 使用匹配的释放函数

cleanup:

    is_jpg_writeing = false; // 重置为可用状态
    is_plate_detection = false;       // 重置车牌检测标志

    return ret;
}
