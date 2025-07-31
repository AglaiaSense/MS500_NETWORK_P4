#include "bsp_sd_card.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "vc_config.h"

static const char *TAG = "BSP_SD_CARD";

#define EXAMPLE_MAX_CHAR_SIZE 64

#define EXAMPLE_IS_UHS1 (CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50 || CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50)

// ---------------------------------- SD卡 初始化/卸载 ----------------------------------

esp_err_t bsp_init_sd_card(device_ctx_t *sd) {

    // 定义返回值和SD卡指针
    esp_err_t ret;
    sdmmc_card_t *card = NULL;

    // 配置文件系统挂载参数
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true, // 挂载失败时格式化
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 4,                   // 最大打开文件数
        .allocation_unit_size = 16 * 1024 // 分配单元大小
    };

    // 定义挂载点路径
    const char mount_point[] = SD_MOUNT_POINT;

    // 配置SDMMC主机参数
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
#if CONFIG_EXAMPLE_SDMMC_SPEED_HS
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 高速模式
#elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_SDR50;
    host.flags &= ~SDMMC_HOST_FLAG_DDR; // 禁用DDR模式
#elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DDR50; // DDR50模式
#endif

    // 配置内部LDO电源控制（如果启用）
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return ret;
    }

    // 设置 LDO 电压， 允许范围[500, 2700]
    ret = sd_pwr_ctrl_set_io_voltage(pwr_ctrl_handle, 2700);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LDO voltage");
        return ret;
    }

    host.pwr_ctrl_handle = pwr_ctrl_handle;
    sd->pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    // 配置SD卡槽参数
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#if EXAMPLE_IS_UHS1
    slot_config.flags |= SDMMC_SLOT_FLAG_UHS1; // 启用UHS-I模式
#endif

    // 设置总线宽度
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.width = 4; // 4位总线
#else
    slot_config.width = 1; // 1位总线
#endif

    // 配置GPIO矩阵（如果启用）
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
    slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
    slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
#endif
#endif

    // 启用内部上拉电阻
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // 挂载文件系统
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    // 处理挂载错误
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return ret;
    }

    // 挂载成功
    ESP_LOGI(TAG, "Filesystem mounted");

    // 打印SD卡信息
    // sdmmc_card_print_info(stdout, card);
    sd->card = card;
    return ret;
}

void bsp_deinit_sd_card(device_ctx_t *sd) {
    // 定义挂载点路径
    const char mount_point[] = SD_MOUNT_POINT;

    // 卸载SD卡文件系统
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(mount_point, sd->card);
    if (ret == ESP_OK) {
        sd->card = NULL; // 置空指针避免重复卸载
    } else {
        ESP_LOGE("SD", "Unmount failed: %s", esp_err_to_name(ret));
    }

    // 记录日志，显示SD卡已卸载
    ESP_LOGI(TAG, "Card unmounted");

    // 如果使用了内部LDO电源控制驱动，则进行反初始化
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    // 删除内部LDO电源控制驱动
    ret = sd_pwr_ctrl_del_on_chip_ldo(sd->pwr_ctrl_handle);

    // 检查反初始化是否成功
    if (ret != ESP_OK) {
        // 如果失败，记录错误日志
        ESP_LOGE(TAG, "Failed to delete the on-chip LDO power control driver");
        return;
    }
#endif
}
// ---------------------------------- 文件读写 ----------------------------------

static esp_err_t s_example_write_file(const char *path, char *data) {
    // 记录日志，显示正在打开的文件路径
    ESP_LOGI(TAG, "Opening file %s", path);

    // 以写入模式打开文件
    FILE *f = fopen(path, "w");

    // 检查文件是否成功打开
    if (f == NULL) {
        // 如果打开失败，记录错误日志
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    // 将数据写入文件
    fprintf(f, data);

    // 关闭文件
    fclose(f);

    // 记录日志，显示文件已成功写入
    ESP_LOGI(TAG, "File written");

    // 返回成功状态
    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path) {
    // 记录日志，显示正在读取的文件路径
    ESP_LOGI(TAG, "Reading file %s", path);

    // 以只读模式打开文件
    FILE *f = fopen(path, "r");

    // 检查文件是否成功打开
    if (f == NULL) {
        // 如果打开失败，记录错误日志并返回失败状态
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    // 定义缓冲区来存储读取的内容
    char line[EXAMPLE_MAX_CHAR_SIZE];

    // 从文件中读取一行内容
    fgets(line, sizeof(line), f);

    // 关闭文件
    fclose(f);

    // 去除换行符
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0'; // 将换行符替换为字符串结束符
    }

    // 记录日志，显示从文件中读取的内容
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    // 返回成功状态
    return ESP_OK;
}

// ----------------------------------  SD测试----------------------------------

void bsp_test_read(void) {
    const char *file_nihao = SD_MOUNT_POINT "/nihao.txt";
    // 读取第二个测试文件
    esp_err_t ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read file %s", file_nihao);
        return;
    }
}
void bsp_sd_card_test(device_ctx_t *sd) {
    // 创建第一个测试文件
    const char *file_hello = SD_MOUNT_POINT "/hello.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];

    // 格式化数据，包含问候语和SD卡名称
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", sd->card->cid.name);

    // 写入文件
    esp_err_t ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return;
    }

    // 定义目标文件名
    const char *file_foo = SD_MOUNT_POINT "/foo.txt";

    // 检查目标文件是否存在
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // 如果存在则删除
        unlink(file_foo);
    }

    // 重命名文件
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    // 读取重命名后的文件
    ret = s_example_read_file(file_foo);
    if (ret != ESP_OK) {
        return;
    }

    // 格式化SD卡（如果配置了相关选项）
#ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return;
    }

    // 检查文件是否仍然存在
    if (stat(file_foo, &st) == 0) {
        ESP_LOGI(TAG, "file still exists");
        return;
    } else {
        ESP_LOGI(TAG, "file doesn't exist, formatting done");
    }
#endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

    // 创建第二个测试文件
    const char *file_nihao = SD_MOUNT_POINT "/nihao.txt";
    memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", sd->card->cid.name);
    ret = s_example_write_file(file_nihao, data);
    if (ret != ESP_OK) {
        return;
    }

    // 读取第二个测试文件
    ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK) {
        return;
    }
}
