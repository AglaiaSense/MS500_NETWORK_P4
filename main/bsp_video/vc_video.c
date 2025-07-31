
#include "vc_video.h"

static const char *TAG = "BSP_VIDEO";

const esp_video_init_csi_config_t csi_config[] = {
    {
        .sccb_config = {
            .init_sccb = true, // 初始化SCCB
            .i2c_config = {
                .port = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_PORT,       // I2C端口
                .scl_pin = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN, // I2C SCL引脚
                .sda_pin = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN, // I2C SDA引脚
            },
            .freq = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_FREQ, // I2C频率
        },
        .reset_pin = CONFIG_EXAMPLE_MIPI_CSI_CAM_SENSOR_RESET_PIN, // 摄像头传感器复位引脚
        .pwdn_pin = CONFIG_EXAMPLE_MIPI_CSI_CAM_SENSOR_PWDN_PIN,   // 摄像头传感器电源引脚
    },
};
 const esp_video_init_config_t cam_config = {
    .csi = csi_config, // CSI配置
};
/**
 * @brief 打印视频设备信息
 *
 * 该函数用于打印视频设备的详细信息，包括版本、驱动、卡片、总线信息以及设备的各种能力。
 *
 * @param capability 指向v4l2_capability结构体的指针，包含设备的能力信息。
 */
static void print_video_device_info(const struct v4l2_capability *capability) {
    // 打印设备版本信息
    ESP_LOGI(TAG, "version: %d.%d.%d", (uint16_t)(capability->version >> 16),
             (uint8_t)(capability->version >> 8),
             (uint8_t)capability->version);

    // 打印设备驱动信息
    ESP_LOGI(TAG, "driver:  %s", capability->driver);

    // 打印设备卡片信息
    ESP_LOGI(TAG, "card:    %s", capability->card);

    // 打印设备总线信息
    ESP_LOGI(TAG, "bus:     %s", capability->bus_info);

    // 打印设备能力信息
    ESP_LOGI(TAG, "capabilities:");
    if (capability->capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        // 打印设备是否支持视频捕获
        ESP_LOGI(TAG, "\tVIDEO_CAPTURE");
    }
    if (capability->capabilities & V4L2_CAP_READWRITE) {
        // 打印设备是否支持读写操作
        ESP_LOGI(TAG, "\tREADWRITE");
    }
    if (capability->capabilities & V4L2_CAP_ASYNCIO) {
        // 打印设备是否支持异步I/O操作
        ESP_LOGI(TAG, "\tASYNCIO");
    }
    if (capability->capabilities & V4L2_CAP_STREAMING) {
        // 打印设备是否支持流媒体操作
        ESP_LOGI(TAG, "\tSTREAMING");
    }
    if (capability->capabilities & V4L2_CAP_META_OUTPUT) {
        // 打印设备是否支持元数据输出
        ESP_LOGI(TAG, "\tMETA_OUTPUT");
    }
    if (capability->capabilities & V4L2_CAP_DEVICE_CAPS) {
        ESP_LOGI(TAG, "device capabilities:");
        if (capability->device_caps & V4L2_CAP_VIDEO_CAPTURE) {
            // 打印设备是否支持视频捕获
            ESP_LOGI(TAG, "\tVIDEO_CAPTURE");
        }
        if (capability->device_caps & V4L2_CAP_READWRITE) {
            // 打印设备是否支持读写操作
            ESP_LOGI(TAG, "\tREADWRITE");
        }
        if (capability->device_caps & V4L2_CAP_ASYNCIO) {
            // 打印设备是否支持异步I/O操作
            ESP_LOGI(TAG, "\tASYNCIO");
        }
        if (capability->device_caps & V4L2_CAP_STREAMING) {
            // 打印设备是否支持流媒体操作
            ESP_LOGI(TAG, "\tSTREAMING");
        }
        if (capability->device_caps & V4L2_CAP_META_OUTPUT) {
            // 打印设备是否支持元数据输出
            ESP_LOGI(TAG, "\tMETA_OUTPUT");
        }
    }
}

/**
 * @brief 初始化视频捕获功能
 *
 * @param uvc 指向uvc_t结构体的指针
 * @return esp_err_t 返回错误代码
 */
static esp_err_t init_capture_video(device_ctx_t *evice_ctx) {
    int fd;
    struct v4l2_capability capability;

    // 打开摄像头设备
    fd = open(CAM_DEV_PATH, O_RDONLY);
    assert(fd >= 0);

    // 查询摄像头设备能力
    ESP_ERROR_CHECK(ioctl(fd, VIDIOC_QUERYCAP, &capability));
    // print_video_device_info(&capability);

    // 保存文件描述符到uvc结构体
    evice_ctx->cap_fd = fd;

    return 0;
}

/**
 * @brief 设置编解码器控制参数
 *
 * @param fd 设备文件描述符
 * @param ctrl_class 控制类（如V4L2_CID_JPEG_CLASS等）
 * @param id 控制项ID（如V4L2_CID_JPEG_COMPRESSION_QUALITY等）
 * @param value 要设置的值
 * @return esp_err_t 返回操作结果，成功返回ESP_OK，失败返回ESP_FAIL
 */
static esp_err_t set_codec_control(int fd, uint32_t ctrl_class, uint32_t id, int32_t value) {
    // 定义扩展控制结构体
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control control[1];

    // 设置控制类
    controls.ctrl_class = ctrl_class;
    // 设置控制项数量
    controls.count = 1;
    // 指向控制项数组
    controls.controls = control;
    // 设置控制项ID
    control[0].id = id;
    // 设置控制项值
    control[0].value = value;

    // 调用ioctl设置控制参数
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        // 设置失败时记录警告日志
        ESP_LOGW(TAG, "failed to set control: %" PRIu32, id);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief 初始化H.264视频编码功能
 *
 * 该函数用于初始化H.264视频编码器，设置相关编码参数，并将配置保存到SD卡相关结构体中。
 *
 * @param sd 指向image_sd_card_t结构体的指针，用于保存编码配置信息
 * @return esp_err_t 返回操作结果，成功返回0
 */
esp_err_t init_codec_h264_video(device_ctx_t *device_ctx) {
    int fd;
    // 定义H.264编码设备路径
    const char *devpath = ESP_VIDEO_H264_DEVICE_NAME;
    struct v4l2_capability capability;

    // 打开H.264编码设备
    fd = open(devpath, O_RDONLY);
    assert(fd >= 0);

    // 查询编码设备能力
    ESP_ERROR_CHECK(ioctl(fd, VIDIOC_QUERYCAP, &capability));
    // 打印设备信息
    // print_video_device_info(&capability);

    // 设置H.264编码I帧间隔
    set_codec_control(fd, V4L2_CID_CODEC_CLASS, V4L2_CID_MPEG_VIDEO_H264_I_PERIOD, CONFIG_EXAMPLE_H264_I_PERIOD);
    // 设置H.264编码比特率
    set_codec_control(fd, V4L2_CID_CODEC_CLASS, V4L2_CID_MPEG_VIDEO_BITRATE, CONFIG_EXAMPLE_H264_BITRATE);
    // 设置H.264编码最小QP值（质量）
    set_codec_control(fd, V4L2_CID_CODEC_CLASS, V4L2_CID_MPEG_VIDEO_H264_MIN_QP, CONFIG_EXAMPLE_H264_MIN_QP);
    // 设置H.264编码最大QP值（质量）
    set_codec_control(fd, V4L2_CID_CODEC_CLASS, V4L2_CID_MPEG_VIDEO_H264_MAX_QP, CONFIG_EXAMPLE_H264_MAX_QP);

    // 保存编码格式为H264
    device_ctx->format = V4L2_PIX_FMT_H264;
    // 保存设备文件描述符
    device_ctx->m2m_fd = fd;

    return 0;
}

/**
 * @brief 初始化MJPEG视频编码功能
 *
 * 该函数用于初始化MJPEG视频编码器，设置JPEG压缩质量，并将配置保存到SD卡相关结构体中。
 *
 * @param sd 指向image_sd_card_t结构体的指针，用于保存编码配置信息
 * @return esp_err_t 返回操作结果，成功返回0
 */
esp_err_t init_codec_mjpeg_video(device_ctx_t *device_ctx) {
    int fd;
    // 定义JPEG编码设备路径
    const char *devpath = ESP_VIDEO_JPEG_DEVICE_NAME;
    struct v4l2_capability capability;

    // 打开JPEG编码设备
    fd = open(devpath, O_RDONLY);
    assert(fd >= 0);

    // 查询编码设备能力
    ESP_ERROR_CHECK(ioctl(fd, VIDIOC_QUERYCAP, &capability));
    // 打印设备信息
    // print_video_device_info(&capability);

    // 设置JPEG压缩质量，默认值为80
    set_codec_control(fd, V4L2_CID_JPEG_CLASS, V4L2_CID_JPEG_COMPRESSION_QUALITY, CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY);

    // 保存编码格式为JPEG
    device_ctx->format = V4L2_PIX_FMT_JPEG;
    // 保存设备文件描述符
    device_ctx->m2m_fd = fd;

    return 0;
}

void vc_video_init(device_ctx_t *video) {

     // 绑定配置到设备上下文
    video->cam_config = &cam_config;  // 使用指针引用

    // 初始化摄像头配置
    ESP_ERROR_CHECK(esp_video_init(video->cam_config));

    // 初始化视频捕获功能
    ESP_ERROR_CHECK(init_capture_video(video));

    // 初始化视频编码功能
#if CONFIG_FORMAT_MJPEG_CAM1
    ESP_ERROR_CHECK(init_codec_mjpeg_video(video));

#elif CONFIG_FORMAT_H264_CAM1
    ESP_ERROR_CHECK(init_codec_h264_video(video));

#endif

    // ESP_ERROR_CHECK(init_codec_h264_video(video));
}

