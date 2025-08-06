
## 项目概述

这是一个 ESP32-P4 网络摄像头项目 (MS500_Network_P4)，实现了一个具备视频流功能的 AI 驱动的车牌检测系统 

## 构建命令

```bash
# 构建项目
idf.py build

# 烧录到设备
idf.py flash

# 监控串口输出
idf.py monitor

# 完全清理构建
idf.py fullclean

# 配置项目
idf.py menuconfig
```
## 架构概述
### 核心组件

*   **主应用程序** (`main/main.c`): 入口点，包含初始化序列、睡眠/唤醒管理以及任务协调
*   **AI 处理** (`main/ai_*`): IMX501 AI 传感器集成、SPI 通信和推理流水线
*   **视频系统** (`main/bsp_video/`): 摄像头控制、UVC 设备、JPEG 编码和 V4L2 接口
*   **网络栈** (`main/bsp_network/`): 以太网、WiFi AP/STA 和 MQTT 连接
*   **存储** (`main/bsp/`): SD 卡、SPIFFS、RTC、OTA 更新和 DNN 模型管理

### 关键子系统

1.  **AI 推理流水线**: IMX501 传感器 → SPI 数据传输 → 车牌检测 → 结果处理
2.  **视频流水线**: 摄像头传感器 → ISP → H.264/JPEG 编码 → USB UVC 或文件存储
3.  **网络管理**: 可配置的以太网/WiFi → MQTT 消息传递 → 远程控制
4.  **电源管理**: 睡眠/唤醒周期、GPIO 唤醒触发、资源清理

### 硬件接口

*   **SPI**: IMX501 AI 传感器通信（主/从模式切换）
*   **I2C/SCCB**: 摄像头传感器配置和 MIPI 控制
*   **USB**: 用于视频流的 UVC 设备实现
*   **以太网**: 主要网络接口（在 `bsp_network.h` 中可配置）
*   **SD 卡**: 模型存储、OTA 更新、图像捕获
*   **GPIO**: 唤醒触发、传感器控制、状态指示灯

### 配置系统

网络连接通过 `main/bsp_network/bsp_network.h` 中的宏进行配置：
*   `BSP_NETWORK_USE_WIFI_AP_STA`: WiFi 接入点/站点模式
*   `BSP_NETWORK_USE_ETHERNET`: 以太网接口（默认）

### 运行模式

*   **UVC_MODEL_VIDEO**: 仅实时视频流
*   **UVC_MODEL_JPG**: 带 AI 检测的 JPEG 捕获
*   **UVC_MODEL_ALL**: 视频和 JPEG 组合模式

### 启动模式

*   **BOOT_LAUNCH_FLASH**: 从闪存固件正常启动运行
*   **BOOT_UPDATE_FLASH**: 固件/模型更新模式
*   **BOOT_LAUNCH_SPI**: 基于 SPI 的固件加载

## 开发说明

*   项目使用 ESP-IDF 框架，并包含用于摄像头、AI 和 USB 功能的定制组件
*   主任务协调处理图像捕获、AI 处理和电源管理周期
*   大量使用 FreeRTOS 任务来实现并发操作
*   模型更新和 OTA 通过检测 SD 卡文件来处理
*   睡眠/唤醒周期在保持对检测事件响应能力的同时节省功耗