#ifndef AI_MIPI_I2C_H
#define AI_MIPI_I2C_H

 #include "stdint.h"
 #include "esp_err.h"

#define IMX501_SENSOR_ADDR          (0x1A)           /*!< IMX501传感器的从设备地址 */

#define I2C_MASTER_SCL_IO           (6)             /*!< I2C主时钟使用的GPIO引脚号 */
#define I2C_MASTER_SDA_IO           (5)             /*!< I2C主数据线使用的GPIO引脚号 */
#define I2C_MASTER_NUM              (0)              /*!< I2C主端口号，可用的I2C外设接口数量取决于芯片 */
#define I2C_MASTER_FREQ_HZ          (400000)         /*!< I2C主时钟频率，单位Hz */
#define I2C_MASTER_TX_BUF_DISABLE   (0)              /*!< I2C主设备不需要发送缓冲区 */
#define I2C_MASTER_RX_BUF_DISABLE   (0)              /*!< I2C主设备不需要接收缓冲区 */
#define I2C_MASTER_TIMEOUT_MS       (1000)           /*!< I2C操作超时时间，单位毫秒 */

void ai_mipi_i2c_copy_sccb_handle(void) ;

esp_err_t ai_mipi_i2c_register_read(uint16_t reg_addr, size_t *data, size_t size);
esp_err_t ai_mipi_i2c_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size);
esp_err_t ai_mipi_i2c_register_write(uint16_t reg_addr, size_t data, size_t size);
 

void ai_mipi_again_init(void) ;

#endif
