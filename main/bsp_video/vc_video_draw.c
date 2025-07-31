#include "vc_video_draw.h"
#include "esp_log.h"
#include <stdlib.h>

extern uvc_model_t current_model;

static const char *TAG = "VC_VIDEO_DRAW";

#if USE_RGB_24

// RGB888颜色定义 (R:8位, G:8位, B:8位)
#define RGB888(r, g, b) ((r << 16) | (g << 8) | b)

// 常用颜色 (使用24位真彩色)
#define COLOR_WHITE RGB888(255, 255, 255)
#define COLOR_BLACK RGB888(0, 0, 0)
#define COLOR_RED RGB888(255, 0, 0)
#define COLOR_GREEN RGB888(0, 255, 0)
#define COLOR_BLUE RGB888(0, 0, 255)

// 显示缓冲区结构
typedef struct {
    int width;
    int height;
    uint8_t *pixels; // RGB888像素数组 (每个像素3字节)
} Framebuffer;

#else

// RGB16颜色定义 (R:5位, G:6位, B:5位)
#define RGB16(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

// 常用颜色
#define COLOR_WHITE RGB16(31, 63, 31)
#define COLOR_BLACK RGB16(0, 0, 0)
#define COLOR_RED RGB16(31, 0, 0)
#define COLOR_GREEN RGB16(0, 63, 0)
#define COLOR_BLUE RGB16(0, 0, 31)

// 显示缓冲区结构
typedef struct {
    int width;
    int height;
    unsigned short *pixels; // RGB16像素数组
} Framebuffer;

#endif

// 字体数据
static const uint8_t font8x8[36][8] = {
    {0x3E, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00}, // '0'
    {0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x3E, 0x00}, // '1'
    {0x3E, 0x01, 0x01, 0x3E, 0x20, 0x20, 0x3F, 0x00}, // '2'
    {0x3E, 0x01, 0x01, 0x1E, 0x01, 0x01, 0x3E, 0x00}, // '3'
    {0x20, 0x24, 0x24, 0x3F, 0x04, 0x04, 0x04, 0x00}, // '4'
    {0x3F, 0x20, 0x20, 0x3E, 0x01, 0x01, 0x3E, 0x00}, // '5'
    {0x3E, 0x20, 0x20, 0x3E, 0x21, 0x21, 0x3E, 0x00}, // '6'
    {0x3F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0x00}, // '7'
    {0x3E, 0x21, 0x21, 0x3E, 0x21, 0x21, 0x3E, 0x00}, // '8'
    {0x3E, 0x21, 0x21, 0x3E, 0x01, 0x01, 0x3E, 0x00}, // '9'

    {0x08, 0x14, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x00}, // 'A'
    {0x3C, 0x22, 0x22, 0x3C, 0x22, 0x22, 0x3C, 0x00}, // 'B'
    {0x1E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x1E, 0x00}, // 'C'
    {0x3C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x3C, 0x00}, // 'D'
    {0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x3E, 0x00}, // 'E'
    {0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x20, 0x00}, // 'F'
    {0x1E, 0x20, 0x20, 0x27, 0x21, 0x21, 0x1E, 0x00}, // 'G'
    {0x22, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x22, 0x00}, // 'H'
    {0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // 'I'
    {0x07, 0x02, 0x02, 0x02, 0x22, 0x22, 0x1C, 0x00}, // 'J'
    {0x22, 0x24, 0x28, 0x30, 0x28, 0x24, 0x22, 0x00}, // 'K'
    {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E, 0x00}, // 'L'
    {0x41, 0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x00}, // 'M'
    {0x22, 0x32, 0x2A, 0x26, 0x22, 0x22, 0x22, 0x00}, // 'N'
    {0x1C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00}, // 'O'
    {0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x20, 0x00}, // 'P'
    {0x1C, 0x22, 0x22, 0x22, 0x2A, 0x24, 0x1A, 0x00}, // 'Q'
    {0x3C, 0x22, 0x22, 0x3C, 0x28, 0x24, 0x22, 0x00}, // 'R'
    {0x1E, 0x20, 0x20, 0x1C, 0x02, 0x02, 0x3C, 0x00}, // 'S'
    {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // 'T'
    {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00}, // 'U'
    {0x22, 0x22, 0x22, 0x22, 0x14, 0x14, 0x08, 0x00}, // 'V'
    {0x41, 0x41, 0x41, 0x49, 0x55, 0x63, 0x41, 0x00}, // 'W'
    {0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22, 0x00}, // 'X'
    {0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00}, // 'Y'
    {0x3E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x3E, 0x00}  // 'Z'
};

#define ORIG_SIZE 8
#define SCALE_FACTOR 8

#if USE_RGB_24

// 初始化帧缓冲区
static Framebuffer *create_framebuffer(int width, int height, uint8_t *pixels) {
    Framebuffer *fb = malloc(sizeof(Framebuffer));
    if (!fb) return NULL;
    fb->width = width;
    fb->height = height;
    fb->pixels = pixels;
    return fb;
}

// 释放帧缓冲区
static void free_framebuffer(Framebuffer *fb) {
    free(fb);
}

// 设置像素 (RGB888格式)
static void set_pixel(Framebuffer *fb, int x, int y, uint32_t color) {
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) {
        int index = (y * fb->width + x) * 3;
        fb->pixels[index] = (color >> 16) & 0xFF;
        fb->pixels[index + 1] = (color >> 8) & 0xFF;
        fb->pixels[index + 2] = color & 0xFF;
    }
}

// Bresenham画线算法 (适配RGB888)
static void draw_line(Framebuffer *fb, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        set_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// 绘制一个字符 (适配RGB888)
static void draw_char(Framebuffer *fb, char c, int x, int y, uint32_t fg, uint32_t bg) {
    int idx = 0;
    if (c >= '0' && c <= '9')
        idx = c - '0';
    else if (c >= 'A' && c <= 'Z')
        idx = c - 'A' + 10;
    else
        return;

    for (int orig_row = 0; orig_row < ORIG_SIZE; orig_row++) {
        uint8_t row_data = font8x8[idx][orig_row];

        for (int orig_col = 0; orig_col < ORIG_SIZE; orig_col++) {
            uint8_t pixel = (row_data >> (7 - orig_col)) & 1;
            int start_row = orig_row * SCALE_FACTOR;
            int start_col = orig_col * SCALE_FACTOR;

            uint32_t color = pixel ? fg : bg;

            for (int yy = 0; yy < SCALE_FACTOR; yy++) {
                for (int xx = 0; xx < SCALE_FACTOR; xx++) {
                    set_pixel(fb, x + start_col + xx, y + start_row + yy, color);
                }
            }
        }
    }
}

// 绘制字符串 (适配RGB888)
static void draw_string(Framebuffer *fb, const char *str, int x, int y, uint32_t fg, uint32_t bg) {
    int start_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 80;
            x = start_x;
        } else if (*str == '\0') {
            return;
        } else {
            draw_char(fb, *str, x, y, fg, bg);
            x += 64;
        }
        str++;
    }
}

// 绘制矩形 (适配RGB888)
static void draw_rectangle(Framebuffer *fb, int x, int y, int width, int height, uint32_t color) {
    for (int i = 0; i < 3; i++) {
        draw_line(fb, x + i, y + i, x + width - 1 - i, y + i, color);
        draw_line(fb, x + width - 1 - i, y + i, x + width - 1 - i, y + height - 1 - i, color);
        draw_line(fb, x + width - 1 - i, y + height - 1 - i, x + i, y + height - 1 - i, color);
        draw_line(fb, x + i, y + height - 1 - i, x + i, y + i, color);
    }
}

#else

// 初始化帧缓冲区
static Framebuffer *create_framebuffer(int width, int height, uint8_t *pixels) {
    Framebuffer *fb = malloc(sizeof(Framebuffer));
    if (!fb) return NULL;
    fb->width = width;
    fb->height = height;
    fb->pixels = (unsigned short *)pixels;
    return fb;
}

// 释放帧缓冲区
static void free_framebuffer(Framebuffer *fb) {
    free(fb);
}

// 设置像素
static void set_pixel(Framebuffer *fb, int x, int y, unsigned short color) {
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) {
        fb->pixels[y * fb->width + x] = color;
    }
}

// Bresenham画线算法
static void draw_line(Framebuffer *fb, int x0, int y0, int x1, int y1, unsigned short color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        set_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// 绘制一个字符
static void draw_char(Framebuffer *fb, char c, int x, int y, unsigned short fg, unsigned short bg) {
    int idx = 0;
    if (c >= '0' && c <= '9')
        idx = c - '0';
    else if (c >= 'A' && c <= 'Z')
        idx = c - 'A' + 10;
    else
        return;

    for (int orig_row = 0; orig_row < ORIG_SIZE; orig_row++) {
        uint8_t row_data = font8x8[idx][orig_row];

        for (int orig_col = 0; orig_col < ORIG_SIZE; orig_col++) {
            uint8_t pixel = (row_data >> (7 - orig_col)) & 1;
            int start_row = orig_row * SCALE_FACTOR;
            int start_col = orig_col * SCALE_FACTOR;

            unsigned short color = (pixel) ? fg : bg;

            for (int yy = 0; yy < SCALE_FACTOR; yy++) {
                for (int xx = 0; xx < SCALE_FACTOR; xx++) {
                    set_pixel(fb, x + start_col + xx, y + start_row + yy, color);
                }
            }
        }
    }
}

// 绘制字符串
static void draw_string(Framebuffer *fb, const char *str, int x, int y, unsigned short fg, unsigned short bg) {
    int start_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 80;
            x = start_x;
        } else if (*str == '\0') {
            return;
        } else {
            draw_char(fb, *str, x, y, fg, bg);
            x += 64;
        }
        str++;
    }
}

static void draw_rectangle(Framebuffer *fb, int x, int y, int width, int height, unsigned short color) {
    for (int i = 0; i < 3; i++) {
        draw_line(fb, x + i, y + i, x + width - 1 - i, y + i, color);
        draw_line(fb, x + width - 1 - i, y + i, x + width - 1 - i, y + height - 1 - i, color);
        draw_line(fb, x + width - 1 - i, y + height - 1 - i, x + i, y + height - 1 - i, color);
        draw_line(fb, x + i, y + height - 1 - i, x + i, y + i, color);
    }
}

#endif

// 根据current_model条件性绘制图像
void video_draw_image_conditionally(uint8_t *pixels) {
    // 只有在特定模式下才进行绘制
    if (current_model == UVC_MODEL_ALL || current_model == UVC_MODEL_VIDEO) {
        Framebuffer *fb = create_framebuffer(1920, 1080, pixels);
        if (!fb) {
            ESP_LOGE(TAG, "Failed to create framebuffer");
            return;
        }

        // 假设plate_result已定义，这里可以添加实际的绘制逻辑
        draw_rectangle(fb, plate_result.x, plate_result.y, plate_result.w, plate_result.h, COLOR_BLUE);
        draw_string(fb, plate_result.plate, 120, 120, COLOR_WHITE, COLOR_BLACK);

        free_framebuffer(fb);
        ESP_LOGD(TAG, "Image drawing completed for model: %d", current_model);
    } else {
        ESP_LOGD(TAG, "Image drawing skipped for model: %d", current_model);
    }
}