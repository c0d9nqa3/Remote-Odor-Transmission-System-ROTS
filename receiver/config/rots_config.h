/**
 * @file rots_config.h
 * @brief ROTS System Configuration
 * @author ROTS Team
 * @date 2024
 * 
 * System configuration parameters for STM32F407VET6
 */

#ifndef ROTS_CONFIG_H
#define ROTS_CONFIG_H

/* System Configuration */
#define ROTS_SYSTEM_ID                 0x01
#define ROTS_FIRMWARE_VERSION_MAJOR   1
#define ROTS_FIRMWARE_VERSION_MINOR   0
#define ROTS_FIRMWARE_VERSION_PATCH   0

/* Hardware Configuration */
#define ROTS_USE_ESP8266_COMMUNICATION    1
#define ROTS_USE_SSD1306_DISPLAY          1
#define ROTS_USE_SPI_FLASH_STORAGE        1
#define ROTS_USE_ENVIRONMENTAL_SENSORS    0

/* HAL Configuration - Disable Legacy CAN to avoid conflicts */
#define HAL_CAN_LEGACY_MODULE_ENABLED     0
#define HAL_CAN_MODULE_ENABLED            0  /* We don't use CAN in this project */

/* Communication Configuration */
#define ROTS_COMM_BAUDRATE            115200
#define ROTS_COMM_TIMEOUT_MS          5000
#define ROTS_COMM_RETRY_COUNT         3

/* Actuator Configuration */
#define ROTS_PUMP_PWM_FREQUENCY       1000    // Hz
#define ROTS_PUMP_PWM_RESOLUTION      1000    // 0-1000
#define ROTS_VALVE_DEBOUNCE_MS        50
#define ROTS_FAN_PWM_FREQUENCY        1000    // Hz

/* Recipe Configuration */
#define ROTS_MAX_CUSTOM_RECIPES       10
#define ROTS_RECIPE_STORAGE_SIZE      1024    // bytes

/* Display Configuration */
#define ROTS_DISPLAY_I2C_ADDRESS      0x3C
#define ROTS_DISPLAY_WIDTH            128
#define ROTS_DISPLAY_HEIGHT           64
#define ROTS_DISPLAY_UPDATE_MS        500

/* System Monitor Configuration */
#define ROTS_MONITOR_UPDATE_MS        100
#define ROTS_ERROR_LOG_SIZE           32
#define ROTS_WATCHDOG_TIMEOUT_MS      10000

/* Safety Configuration */
#define ROTS_EMERGENCY_STOP_ENABLED   1
#define ROTS_MAX_ODOR_DURATION_MS     300000  // 5 minutes
#define ROTS_MAX_PUMP_SPEED           100     // 100%
#define ROTS_MIN_PUMP_SPEED           0       // 0%

/* Debug Configuration */
#define ROTS_DEBUG_ENABLED            1
#define ROTS_DEBUG_UART               USART1
#define ROTS_DEBUG_BAUDRATE           115200

/* Pin Configuration */
#define ROTS_ESP8266_UART             USART2
#define ROTS_ESP8266_TX_PIN           GPIO_PIN_2
#define ROTS_ESP8266_RX_PIN           GPIO_PIN_3
#define ROTS_ESP8266_EN_PIN           GPIO_PIN_4

#define ROTS_DISPLAY_I2C              I2C1
#define ROTS_DISPLAY_SDA_PIN          GPIO_PIN_6
#define ROTS_DISPLAY_SCL_PIN          GPIO_PIN_7

#define ROTS_SPI_FLASH_SPI            SPI1
#define ROTS_SPI_FLASH_CS_PIN         GPIO_PIN_4
#define ROTS_SPI_FLASH_MOSI_PIN       GPIO_PIN_5
#define ROTS_SPI_FLASH_MISO_PIN       GPIO_PIN_6
#define ROTS_SPI_FLASH_SCK_PIN        GPIO_PIN_7

/* Memory Configuration */
#define ROTS_HEAP_SIZE                8192    // 8KB
#define ROTS_STACK_SIZE               4096    // 4KB

/* Timing Configuration */
#define ROTS_SYSTEM_TICK_MS           1
#define ROTS_MAIN_LOOP_DELAY_MS       10
#define ROTS_ACTUATOR_UPDATE_MS       50

#endif /* ROTS_CONFIG_H */
