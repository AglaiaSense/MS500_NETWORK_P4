

## 开发环境和参考

### 开发环境
- **开发板**：ESP32-P4
- **开发环境**：VS Code
- **ESP-IDF 版本**：v5.4.0+


###  官方参考网站
- **ESP-IDF 编程指南** ： https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/index.html
- **ESP-IoT-Solution 编程指南**： https://docs.espressif.com/projects/esp-iot-solution/zh_CN/latest/index.htm
- **适用于 VS Code 的 ESP-IDF 扩展**：https://docs.espressif.com/projects/vscode-esp-idf-extension/zh_CN/latest/index.html
- **ESP-FAQ**：https://docs.espressif.com/projects/esp-faq/zh_CN/latest/index.html
- **esp-dev-kits开发板说明 ：P4**：https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32p4/index.html
- **ESP 硬件设计指南 ：P4**：https://docs.espressif.com/projects/esp-hardware-design-guidelines/zh_CN/latest/esp32p4/index.html

### 官方代码库
- **官方组织**： https://github.com/espressif
- **ESP-IDF**：https://github.com/espressif/esp-idf
- **ESP-IoT-Solution**：https://github.com/espressif/esp-iot-solution
- **esp-video-components**：https://github.com/espressif/esp-video-components
- **esp-hosted-mcu**：https://github.com/espressif/esp-hosted-mcu
- **esp-iot-bridge**：https://github.com/espressif/esp-iot-bridge
- **esp32-p4-eye**：https://github.com/espressif/esp-dev-kits/blob/master/examples/esp32-p4-eye/examples/factory_demo/README_CN.md

### 组件参考网站
- **ESP组件库**：https://components.espressif.com/
- **如果官方库没有对应代码，可在组件库中搜索**



## 开发规范
### 🔄 项目认知与上下文
- **在开始新的对话时，务必阅读 `README.md`** 以了解项目架构概述
- **在开始新任务前检查 `TASK.md`**。如果任务未列出，请添加一个简短的描述和今天的日期
- **使用一致的命名约定、文件结构和架构模式**，如 `PLANNING.md` 所述


### 🧱 代码结构与模块化
- **切勿创建超过500行代码的文件**。如果文件接近此限制，应通过将其拆分为模块或辅助文件进行重构
- **将代码组织成清晰分离的模块**，按功能或职责分组
- **使用清晰、一致的导入方式**, 函数和变量尽量使用extern声明。像main.c主函数等，关系到创建的地方可以使用#include 
- **vc_config.h**  来处理多次使用的变量和枚举
- **同一文件内涉及多个不同的功能函数**，可使用#------------------  功能描述  ------------------  来进行注释
<!-- 
### 🧪 测试与可靠性
- **一次完整的会话结束，始终通过idf.py build来构建项目** 检测项目是否报错，报错及时
- **通过监测终端查看运行结果**，  -->


### ✅ 任务完成
- **在完成任务后立即在 `TASK.md` 中标记**
- 在开发过程中发现的新子任务或待办事项，将其添加到 `TASK.md` 的"开发过程中发现"部分下

### 📎 风格与约定
- **使用 c** 作为主要语言
- **使用 `pydantic` 进行数据验证**
- **为每个函数编写中文文档字符串**，只需要写函数的作用，不需要写参数和返回值说明
  ```c
    // 主函数
     void app_main(void) {
   
    }
  ```

### 📚 文档与可解释性
- **添加新功能、依赖变更或修改设置步骤时更新 `README.md`** 
- **对非显而易见的代码进行注释**，确保所有内容对中级开发人员都是可理解的
- 编写复杂逻辑时，**添加内联注释 `# 原因：`** 解释为什么这样做，而不仅仅是做什么
- **在代码中使用有意义的变量名和函数名**，避免使用单个字母或无意义的名称



### 🧠 AI 行为规则
- **切勿假设缺失的上下文。如果不确定，请提问**
- **切勿虚构库或函数** 先从我给出的参加里面去查找库和函数
- **在代码或测试中引用文件路径和模块名之前，始终确认它们存在**
- **除非明确指示或作为 `TASK.md` 中的任务的一部分，否则切勿删除或覆盖现有代码**
- **注释使用中文**，
- **打印使用英文**
 