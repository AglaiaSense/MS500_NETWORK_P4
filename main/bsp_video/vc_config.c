// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/vc_config.c

#include "vc_config.h"

static const char *TAG = "BSP_CONFIG";

boot_mode_t boot_mode = BOOT_LAUNCH_FLASH; // 默认正常启动

void delay_ms(int nms) {
    vTaskDelay(pdMS_TO_TICKS(nms));
}

void check_memory(void) {
    // 总剩余堆内存 (转换为KB)
    // ESP_LOGI(TAG, "Total free heap memory: %u KB", (unsigned int)(esp_get_free_heap_size() / 1024));

    // 历史最小剩余堆内存
    // ESP_LOGI(TAG, "Minimum ever free heap memory: %u KB", (unsigned int)(esp_get_minimum_free_heap_size() / 1024));

    // SPIRAM剩余内存
    ESP_LOGI(TAG, "Free SPIRAM memory: %u KB", (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
}

void bsp_restart_print(void) {
    printf("Restarting system in 5 seconds...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
}