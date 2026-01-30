// ROTS Sender Main Header File
#ifndef ROTS_SENDER_H
#define ROTS_SENDER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>

#ifdef __cplusplus
extern "C" {
#endif

// System status codes
typedef enum {
    ROTS_OK = 0x00,
    ROTS_ERROR = 0x01,
    ROTS_BUSY = 0x02,
    ROTS_TIMEOUT = 0x03,
    ROTS_INVALID_PARAM = 0x04,
    ROTS_COMM_ERROR = 0x05,
    ROTS_SENSOR_ERROR = 0x06,
    ROTS_AI_ERROR = 0x07,
    ROTS_MEMORY_ERROR = 0x08
} ROTS_StatusTypeDef;

// Sender state
typedef enum {
    ROTS_SENDER_IDLE = 0x00,
    ROTS_SENDER_DETECTING = 0x01,
    ROTS_SENDER_SENDING = 0x02,
    ROTS_SENDER_ERROR = 0x03
} ROTS_SenderState_t;

// Debug level
typedef enum {
    ROTS_DEBUG_ERROR = 0,
    ROTS_DEBUG_WARNING = 1,
    ROTS_DEBUG_INFO = 2,
    ROTS_DEBUG_DEBUG = 3
} ROTS_DebugLevel_t;

// Odor type
typedef enum {
    ROTS_ODOR_COFFEE = 0x01,
    ROTS_ODOR_ALCOHOL = 0x02,
    ROTS_ODOR_LEMON = 0x03,
    ROTS_ODOR_MINT = 0x04,
    ROTS_ODOR_LAVENDER = 0x05,
    ROTS_ODOR_UNKNOWN = 0x00
} ROTS_OdorType_t;

// Sensor data structure
typedef struct {
    float mq2_value;      // MQ-2 Combustible gas
    float mq3_value;      // MQ-3 Alcohol
    float mq4_value;      // MQ-4 Methane
    float mq5_value;      // MQ-5 LPG
    float mq6_value;      // MQ-6 LPG
    float mq7_value;      // MQ-7 Carbon monoxide
    float mq8_value;      // MQ-8 Hydrogen
    float mq9_value;      // MQ-9 Carbon monoxide
    float temperature;    // Temperature
    float humidity;       // Humidity
    float pressure;       // Pressure
    uint32_t timestamp;   // Timestamp
} ROTS_SensorData_t;

// AI inference result
typedef struct {
    ROTS_OdorType_t odor_type;
    char odor_name[16];
    float confidence;
    float intensity;
    uint32_t timestamp;
} ROTS_OdorResult_t;

// Sender status
typedef struct {
    ROTS_SenderState_t state;
    uint32_t last_detection_time;
    uint32_t detection_count;
    uint32_t error_count;
    bool wifi_connected;
    bool mqtt_connected;
    float battery_voltage;
} ROTS_SenderStatus_t;

// Hardware pin definitions
#define ROTS_ERROR_LED_PIN        2
#define ROTS_STATUS_LED_PIN       4
#define ROTS_SENSOR_POWER_PIN     5

// MQ sensor pins (ESP32 ADC channels on GPIO 32-39)
#define ROTS_MQ2_PIN              32  // ADC1_CH4
#define ROTS_MQ3_PIN              33  // ADC1_CH5
#define ROTS_MQ4_PIN              34  // ADC1_CH6
#define ROTS_MQ5_PIN              35  // ADC1_CH7
#define ROTS_MQ6_PIN              36  // ADC1_CH0 (VP)
#define ROTS_MQ7_PIN              39  // ADC1_CH3 (VN)
#define ROTS_MQ8_PIN              25  // ADC2_CH8 (fallback)
#define ROTS_MQ9_PIN              26  // ADC2_CH9 (fallback)

// I2C pins (DHT22, BMP280)
#define ROTS_SDA_PIN              21
#define ROTS_SCL_PIN              22

// System configuration
#define ROTS_MAX_SENSORS          8
#define ROTS_AI_CONFIDENCE_THRESHOLD  0.7f
#define ROTS_SENSOR_READ_INTERVAL     100    // ms
#define ROTS_AI_INFERENCE_INTERVAL    500    // ms
#define ROTS_STATUS_UPDATE_INTERVAL   1000   // ms
#define ROTS_DEBUG_OUTPUT_INTERVAL    10000  // ms

// WiFi configuration
#define ROTS_WIFI_SSID            "ROTS_Network"
#define ROTS_WIFI_PASSWORD        "rots_password_2024"
#define ROTS_WIFI_TIMEOUT_MS      10000

// MQTT configuration
#define ROTS_MQTT_BROKER_HOST     "mqtt.rots-system.com"
#define ROTS_MQTT_BROKER_PORT     1883
#define ROTS_MQTT_CLIENT_ID       "ROTS_SENDER_001"
#define ROTS_MQTT_TOPIC_DETECTION "rots/detection/001"
#define ROTS_MQTT_TOPIC_STATUS    "rots/status/001"
#define ROTS_MQTT_TOPIC_ERROR     "rots/error/001"

// Function declarations
ROTS_StatusTypeDef ROTS_Sender_Init(void);
void ROTS_Sender_MainLoop(void);
void ROTS_Sender_ErrorHandler(ROTS_StatusTypeDef error_code);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_SENDER_H */
