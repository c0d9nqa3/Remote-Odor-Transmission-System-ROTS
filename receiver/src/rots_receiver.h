// ROTS Receiver Main Header File

#ifndef ROTS_RECEIVER_H
#define ROTS_RECEIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ROTS System Status Codes */
typedef enum {
    ROTS_OK = 0x00,
    ROTS_ERROR = 0x01,
    ROTS_BUSY = 0x02,
    ROTS_TIMEOUT = 0x03,
    ROTS_INVALID_PARAM = 0x04,
    ROTS_COMM_ERROR = 0x05,
    ROTS_ACTUATOR_ERROR = 0x06,
    ROTS_RECIPE_ERROR = 0x07,
    ROTS_DISPLAY_ERROR = 0x08,
    ROTS_MEMORY_ERROR = 0x09
} ROTS_StatusTypeDef;

/* ROTS Message Types */
typedef enum {
    ROTS_MSG_ODOR_COMMAND = 0x01,
    ROTS_MSG_STATUS_REQUEST = 0x02,
    ROTS_MSG_RECIPE_UPDATE = 0x03,
    ROTS_MSG_SYSTEM_CONFIG = 0x04,
    ROTS_MSG_EMERGENCY_STOP = 0x05
} ROTS_MessageType_t;

/* ROTS Odor Types */
typedef enum {
    ROTS_ODOR_COFFEE = 0x01,
    ROTS_ODOR_ALCOHOL = 0x02,
    ROTS_ODOR_LEMON = 0x03,
    ROTS_ODOR_MINT = 0x04,
    ROTS_ODOR_LAVENDER = 0x05,
    ROTS_ODOR_MIXED = 0x06
} ROTS_OdorType_t;

/* ROTS Actuator States */
typedef enum {
    ROTS_ACTUATOR_OFF = 0x00,
    ROTS_ACTUATOR_ON = 0x01,
    ROTS_ACTUATOR_ERROR = 0x02
} ROTS_ActuatorState_t;

/* ROTS System States */
typedef enum {
    ROTS_SYSTEM_IDLE = 0x00,
    ROTS_SYSTEM_RUNNING = 0x01,
    ROTS_SYSTEM_ERROR = 0x02,
    ROTS_SYSTEM_MAINTENANCE = 0x03
} ROTS_SystemState_t;

/* ROTS Message Structure */
typedef struct {
    uint8_t message_type;
    uint8_t odor_type;
    uint8_t intensity;        /* 0-100% */
    uint16_t duration;        /* Duration in seconds */
    uint8_t pump_config[5];   /* Pump configuration (0-100%) */
    uint32_t timestamp;
    uint16_t checksum;
} ROTS_MessageTypeDef;

/* ROTS System Configuration */
typedef struct {
    uint8_t system_id;
    uint8_t firmware_version[3];
    uint16_t max_intensity;
    uint16_t max_duration;
    bool emergency_stop_enabled;
    uint32_t watchdog_timeout;
} ROTS_SystemConfig_t;

/* ROTS System Status */
typedef struct {
    ROTS_SystemState_t state;
    uint8_t error_count;
    uint32_t uptime;
    float temperature;
    float humidity;
    uint8_t pump_status[5];
    uint8_t valve_status[5];
    bool communication_active;
} ROTS_SystemStatus_t;

/* Hardware Pin Definitions */
#define ERROR_LED_PORT    GPIOA
#define ERROR_LED_PIN     GPIO_PIN_5

#define STATUS_LED_PORT   GPIOA
#define STATUS_LED_PIN    GPIO_PIN_6

#define PUMP1_PWM_PORT    GPIOA
#define PUMP1_PWM_PIN     GPIO_PIN_8
#define PUMP1_EN_PORT     GPIOA
#define PUMP1_EN_PIN      GPIO_PIN_9

#define PUMP2_PWM_PORT    GPIOA
#define PUMP2_PWM_PIN     GPIO_PIN_10
#define PUMP2_EN_PORT     GPIOA
#define PUMP2_EN_PIN      GPIO_PIN_11

#define PUMP3_PWM_PORT    GPIOB
#define PUMP3_PWM_PIN     GPIO_PIN_0
#define PUMP3_EN_PORT     GPIOB
#define PUMP3_EN_PIN      GPIO_PIN_1

#define PUMP4_PWM_PORT    GPIOB
#define PUMP4_PWM_PIN     GPIO_PIN_6
#define PUMP4_EN_PORT     GPIOB
#define PUMP4_EN_PIN      GPIO_PIN_7

#define PUMP5_PWM_PORT    GPIOB
#define PUMP5_PWM_PIN     GPIO_PIN_8
#define PUMP5_EN_PORT     GPIOB
#define PUMP5_EN_PIN      GPIO_PIN_9

#define VALVE1_PORT       GPIOC
#define VALVE1_PIN        GPIO_PIN_0
#define VALVE2_PORT       GPIOC
#define VALVE2_PIN        GPIO_PIN_1
#define VALVE3_PORT       GPIOC
#define VALVE3_PIN        GPIO_PIN_2
#define VALVE4_PORT       GPIOC
#define VALVE4_PIN        GPIO_PIN_3
#define VALVE5_PORT       GPIOC
#define VALVE5_PIN        GPIO_PIN_4

#define FAN1_PORT         GPIOC
#define FAN1_PIN          GPIO_PIN_5
#define FAN2_PORT         GPIOC
#define FAN2_PIN          GPIO_PIN_6

/* System Constants */
#define ROTS_MAX_PUMPS            5
#define ROTS_MAX_VALVES           5
#define ROTS_MAX_FANS             2
#define ROTS_MAX_INTENSITY        100
#define ROTS_MAX_DURATION         300
#define ROTS_COMM_TIMEOUT         5000
#define ROTS_WATCHDOG_TIMEOUT     10000

/* Function Prototypes */
ROTS_StatusTypeDef ROTS_ProcessOdorCommand(ROTS_MessageTypeDef* message);
ROTS_StatusTypeDef ROTS_SystemClock_Init(void);
void ROTS_SystemMonitor_LogError(ROTS_StatusTypeDef error_code);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_RECEIVER_H */
