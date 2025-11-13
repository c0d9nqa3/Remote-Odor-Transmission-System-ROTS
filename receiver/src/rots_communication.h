/**
 * @file rots_communication.h
 * @brief ROTS Communication Module Header
 * @author ROTS Team
 * @date 2024
 * 
 * Header file for communication between ESP32 and STM32
 */

#ifndef ROTS_COMMUNICATION_H
#define ROTS_COMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "rots_receiver.h"

/* MQTT Configuration */
#define ROTS_MQTT_BROKER_HOST     "mqtt.rots-system.com"
#define ROTS_MQTT_BROKER_PORT     1883
#define ROTS_MQTT_CLIENT_ID       "ROTS_RECEIVER_001"
#define ROTS_MQTT_TOPIC_COMMAND   "rots/command/001"
#define ROTS_MQTT_TOPIC_STATUS    "rots/status/001"
#define ROTS_MQTT_TOPIC_ERROR     "rots/error/001"

/* WiFi Configuration */
#define ROTS_WIFI_SSID            "ROTS_Network"
#define ROTS_WIFI_PASSWORD        "rots_password_2024"
#define ROTS_WIFI_TIMEOUT_MS      10000

/* Function Prototypes */
ROTS_StatusTypeDef ROTS_Communication_Init(void);
ROTS_StatusTypeDef ROTS_Communication_ConnectWiFi(void);
ROTS_StatusTypeDef ROTS_Communication_ConnectMQTT(void);
ROTS_StatusTypeDef ROTS_Communication_ReceiveMessage(ROTS_MessageTypeDef* message);
ROTS_StatusTypeDef ROTS_Communication_SendStatus(ROTS_SystemStatus_t* status);
ROTS_StatusTypeDef ROTS_Communication_SendError(ROTS_StatusTypeDef error_code);
ROTS_StatusTypeDef ROTS_Communication_KeepAlive(void);
ROTS_StatusTypeDef ROTS_Communication_Update(void);

/* MQTT Callbacks */
void ROTS_MQTT_MessageCallback(char* topic, char* payload, int length);
void ROTS_MQTT_ConnectCallback(void);
void ROTS_MQTT_DisconnectCallback(void);

/* UART Callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_COMMUNICATION_H */
