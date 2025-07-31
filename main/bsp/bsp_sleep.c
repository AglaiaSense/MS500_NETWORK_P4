#include "bsp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

#include "bsp_rtc.h"

static const char *TAG = "BSP_SLEEP";

#define TRIG_IO_ONE GPIO_NUM_32
#define TRIG_IO_TWO GPIO_NUM_33

extern capture_mode_t g_capture_mode; // 拍照模式

// --------------------------   睡眠 ------------------------------------------

#if 1
// 检查重启原因
void check_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Timer wakeup");
            break;
            
        case ESP_SLEEP_WAKEUP_GPIO: {
            uint64_t gpio_mask = esp_sleep_get_gpio_wakeup_status();
            printf("GPIO wakeup, gpio_mask: %lld\n", gpio_mask);
            
            uint32_t pin_one = gpio_get_level(TRIG_IO_ONE);
            uint32_t pin_two = gpio_get_level(TRIG_IO_TWO);

            // 检查具体唤醒引脚
            if (pin_one == 1) {
                ESP_LOGI(TAG, "---------   SINGLE_SHOT");
                g_capture_mode = CAPTURE_MODE_SINGLE_SHOT; // 设置单次拍照标志
            }
            if (pin_two == 1) {
                ESP_LOGI(TAG, "---------   CONTINUOUS");
                g_capture_mode = CAPTURE_MODE_CONTINUOUS    ; // 设置单次拍照标志

            }
            break;
        }
            
        default:
            ESP_LOGI(TAG, "Wakeup cause: %d", wakeup_reason);
            break;
    }
}

void configure_wakeup_gpio(void) {
    // 使用宏定义配置GPIO
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << TRIG_IO_ONE) | (1ULL << TRIG_IO_TWO),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config);

    // 使能宏定义GPIO的唤醒功能
    gpio_wakeup_enable(TRIG_IO_ONE, GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable(TRIG_IO_TWO, GPIO_INTR_HIGH_LEVEL);
    
    esp_sleep_enable_gpio_wakeup();
}
#endif


#if 0
//  io33 和Io32 不是retc io
// 检查唤醒原因（适配EXT1唤醒）
void check_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "Timer wakeup");
        break;

    case ESP_SLEEP_WAKEUP_EXT1: {
        uint64_t gpio_mask = esp_sleep_get_ext1_wakeup_status();
        ESP_LOGI(TAG, "EXT1 wakeup, gpio_mask: 0x%llx", gpio_mask);

        // 通过位掩码直接判断唤醒引脚
        if (gpio_mask & (1ULL << TRIG_IO_ONE)) {
            ESP_LOGI(TAG, "---------   SINGLE_SHOT");
            g_capture_mode = CAPTURE_MODE_SINGLE_SHOT;
        }
        if (gpio_mask & (1ULL << TRIG_IO_TWO)) {
            ESP_LOGI(TAG, "---------   CONTINUOUS");
            g_capture_mode = CAPTURE_MODE_CONTINUOUS;
        }
        break;
    }

    default:
        ESP_LOGI(TAG, "Wakeup cause: %d", wakeup_reason);
        break;
    }
}
// 配置唤醒GPIO为RTC GPIO并设置ext1唤醒
void configure_wakeup_gpio(void) {
    // 配置为输入 + 下拉
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << TRIG_IO_ONE) | (1ULL << TRIG_IO_TWO),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config);

    // 初始化为 RTC GPIO
    rtc_gpio_init(TRIG_IO_ONE);
    rtc_gpio_set_direction(TRIG_IO_ONE, RTC_GPIO_MODE_INPUT_ONLY);

    rtc_gpio_init(TRIG_IO_TWO);
    rtc_gpio_set_direction(TRIG_IO_TWO, RTC_GPIO_MODE_INPUT_ONLY);

    // 配置EXT1唤醒（任意引脚高电平触发）
    uint64_t pin_mask = (1ULL << TRIG_IO_ONE) | (1ULL << TRIG_IO_TWO);
    esp_sleep_enable_ext1_wakeup(pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
}


#endif

// 配置唤醒时间
void configure_wakeup_timer(uint64_t sleep_time_us) {

    // 增加0.5秒，不然在1s内会重复睡眠
    sleep_time_us = sleep_time_us + SEC_TO_USEC(0.5);

    // 配置计时器唤醒
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_us));
}

// --------------------------   深度睡眠 ------------------------------------------

void enter_deep_sleep_time(uint64_t sleep_time_us) {

    ESP_LOGI(TAG, "deep sleep time wakeup");

    // 配置唤醒源
    configure_wakeup_timer(sleep_time_us);

    // 进入深度睡眠模式
    esp_deep_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// 进入深度睡眠模式
void enter_deep_sleep_gpio() {

    ESP_LOGI(TAG, "deep sleep gpio wakeup");

    configure_wakeup_gpio();

    // 开始进入轻睡眠模式
    esp_deep_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// --------------------------   浅睡眠 ------------------------------------------

void enter_light_sleep_time(uint64_t sleep_time_us) {

    ESP_LOGI(TAG, "light sleep time wakeup");

    // 配置唤醒源
    configure_wakeup_timer(sleep_time_us);

    esp_light_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// 进入深度睡眠模式
void enter_light_sleep_gpio() {

    ESP_LOGI(TAG, "light sleep gpio wakeup");

    configure_wakeup_gpio();

    // 开始进入轻睡眠模式
    esp_light_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}
