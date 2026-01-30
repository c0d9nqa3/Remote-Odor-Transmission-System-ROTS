/**
 * @file rots_communication.c
 * @brief ROTS Communication Module - MQTT + WiFi
 * @author ROTS Team
 * @date 2024
 * 
 * Complete MQTT and WiFi communication implementation for receiver
 */

#include "rots_receiver.h"
#include "rots_communication.h"
#include "rots_debug.h"
#include <string.h>
#include <stdlib.h>

/* MQTT and WiFi variables */
static bool wifi_connected = false;
static bool mqtt_connected = false;
static ROTS_MessageTypeDef rx_buffer;
static bool message_received = false;
static uint32_t last_communication_time = 0;
static uint8_t uart_rx_buffer[512];
static uint16_t uart_rx_index = 0;
static bool uart_rx_complete = false;

/* Global UART handle for ESP8266 (exported for interrupt handler) */
UART_HandleTypeDef huart_esp8266;

/* WiFi and MQTT configuration */
#ifndef ROTS_WIFI_SSID
#define ROTS_WIFI_SSID "ROTS_Network"
#endif

#ifndef ROTS_WIFI_PASSWORD
#define ROTS_WIFI_PASSWORD "rots_password_2024"
#endif

#ifndef ROTS_MQTT_BROKER_HOST
#define ROTS_MQTT_BROKER_HOST "mqtt.rots-system.com"
#endif

#ifndef ROTS_MQTT_BROKER_PORT
#define ROTS_MQTT_BROKER_PORT 1883
#endif

/* Function prototypes */
static ROTS_StatusTypeDef ROTS_Communication_ValidateMessage(ROTS_MessageTypeDef* msg);
static uint16_t ROTS_Communication_CalculateChecksum(ROTS_MessageTypeDef* msg);
static ROTS_StatusTypeDef ROTS_Communication_ParseMQTTMessage(const uint8_t* data, uint16_t length, ROTS_MessageTypeDef* msg);
static ROTS_StatusTypeDef ROTS_Communication_SendATCommand(const char* cmd, char* response, uint16_t response_size, uint32_t timeout);
static ROTS_StatusTypeDef ROTS_Communication_WaitForResponse(const char* expected, uint32_t timeout);
static ROTS_StatusTypeDef ROTS_Communication_SendMQTTPacket(const uint8_t* packet, uint16_t length);

/**
 * @brief Initialize communication module
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_Init(void)
{
    ROTS_StatusTypeDef status = ROTS_OK;
    
    /* Initialize WiFi connection */
    status = ROTS_Communication_ConnectWiFi();
    if (status != ROTS_OK) {
        DEBUG_ERROR("WiFi connection failed\r\n");
        return status;
    }
    
    /* Initialize MQTT connection */
    status = ROTS_Communication_ConnectMQTT();
    if (status != ROTS_OK) {
        DEBUG_ERROR("MQTT connection failed\r\n");
        return status;
    }
    
    /* Start UART reception */
    HAL_UART_Receive_IT(&huart_esp8266, uart_rx_buffer, sizeof(uart_rx_buffer));
    
    DEBUG_INFO("Communication initialized\r\n");
    return ROTS_OK;
}

/**
 * @brief Connect to WiFi network
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_ConnectWiFi(void)
{
    char at_cmd[100];
    char response[200];
    uint32_t timeout;
    
    /* Initialize UART for ESP8266 */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart_esp8266.Instance = USART2;
    huart_esp8266.Init.BaudRate = 115200;
    huart_esp8266.Init.WordLength = UART_WORDLENGTH_8B;
    huart_esp8266.Init.StopBits = UART_STOPBITS_1;
    huart_esp8266.Init.Parity = UART_PARITY_NONE;
    huart_esp8266.Init.Mode = UART_MODE_TX_RX;
    huart_esp8266.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_esp8266.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart_esp8266) != HAL_OK) {
        return ROTS_COMM_ERROR;
    }
    
    /* Reset ESP8266 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(2000);
    
    /* Test AT command */
    if (ROTS_Communication_SendATCommand("AT\r\n", response, sizeof(response), 1000) != ROTS_OK) {
        DEBUG_ERROR("ESP8266 not responding\r\n");
        return ROTS_COMM_ERROR;
    }
    
    /* Set WiFi mode to station */
    if (ROTS_Communication_SendATCommand("AT+CWMODE=1\r\n", response, sizeof(response), 1000) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    HAL_Delay(500);
    
    /* Connect to WiFi */
    sprintf(at_cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ROTS_WIFI_SSID, ROTS_WIFI_PASSWORD);
    if (ROTS_Communication_SendATCommand(at_cmd, response, sizeof(response), 15000) != ROTS_OK) {
        DEBUG_ERROR("WiFi connection failed\r\n");
        return ROTS_COMM_ERROR;
    }
    
    /* Wait for connection */
    if (ROTS_Communication_WaitForResponse("WIFI GOT IP", 10000) != ROTS_OK) {
        DEBUG_ERROR("WiFi connection timeout\r\n");
        return ROTS_TIMEOUT;
    }
    
    /* Check connection status */
    if (ROTS_Communication_SendATCommand("AT+CIPSTATUS\r\n", response, sizeof(response), 1000) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    /* Enable multiple connections */
    if (ROTS_Communication_SendATCommand("AT+CIPMUX=1\r\n", response, sizeof(response), 1000) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    wifi_connected = true;
    DEBUG_INFO("WiFi connected\r\n");
    return ROTS_OK;
}

/**
 * @brief Connect to MQTT broker
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_ConnectMQTT(void)
{
    char mqtt_cmd[200];
    char response[200];
    uint8_t mqtt_connect_packet[128];
    uint16_t packet_length = 0;
    
    /* Build MQTT CONNECT packet */
    /* Fixed header */
    mqtt_connect_packet[packet_length++] = 0x10;  /* CONNECT packet type */
    packet_length++;  /* Remaining length (will be filled later) */
    
    /* Variable header */
    mqtt_connect_packet[packet_length++] = 0x00;  /* Protocol name length MSB */
    mqtt_connect_packet[packet_length++] = 0x04;  /* Protocol name length LSB */
    mqtt_connect_packet[packet_length++] = 'M';
    mqtt_connect_packet[packet_length++] = 'Q';
    mqtt_connect_packet[packet_length++] = 'T';
    mqtt_connect_packet[packet_length++] = 'T';
    mqtt_connect_packet[packet_length++] = 0x04;  /* Protocol version */
    mqtt_connect_packet[packet_length++] = 0x02;  /* Connect flags (clean session) */
    mqtt_connect_packet[packet_length++] = 0x00;  /* Keep alive MSB */
    mqtt_connect_packet[packet_length++] = 0x3C;  /* Keep alive LSB (60 seconds) */
    
    /* Client ID */
    const char* client_id = ROTS_MQTT_CLIENT_ID;
    uint8_t client_id_len = strlen(client_id);
    mqtt_connect_packet[packet_length++] = 0x00;  /* Client ID length MSB */
    mqtt_connect_packet[packet_length++] = client_id_len;  /* Client ID length LSB */
    memcpy(&mqtt_connect_packet[packet_length], client_id, client_id_len);
    packet_length += client_id_len;
    
    /* Fill remaining length */
    uint8_t remaining_length = packet_length - 2;
    mqtt_connect_packet[1] = remaining_length;
    
    /* Connect to MQTT broker via TCP */
    sprintf(mqtt_cmd, "AT+CIPSTART=0,\"TCP\",\"%s\",%d\r\n", ROTS_MQTT_BROKER_HOST, ROTS_MQTT_BROKER_PORT);
    if (ROTS_Communication_SendATCommand(mqtt_cmd, response, sizeof(response), 5000) != ROTS_OK) {
        DEBUG_ERROR("TCP connection failed\r\n");
        return ROTS_COMM_ERROR;
    }
    
    /* Wait for connection */
    if (ROTS_Communication_WaitForResponse("CONNECT", 5000) != ROTS_OK) {
        DEBUG_ERROR("TCP connection timeout\r\n");
        return ROTS_TIMEOUT;
    }
    
    HAL_Delay(500);
    
    /* Send MQTT CONNECT packet */
    if (ROTS_Communication_SendMQTTPacket(mqtt_connect_packet, packet_length) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    HAL_Delay(1000);
    
    /* Build MQTT SUBSCRIBE packet */
    uint8_t mqtt_subscribe_packet[64];
    packet_length = 0;
    
    /* Fixed header */
    mqtt_subscribe_packet[packet_length++] = 0x82;  /* SUBSCRIBE packet type */
    packet_length++;  /* Remaining length */
    
    /* Variable header */
    mqtt_subscribe_packet[packet_length++] = 0x00;  /* Packet identifier MSB */
    mqtt_subscribe_packet[packet_length++] = 0x01;  /* Packet identifier LSB */
    
    /* Topic filter */
    const char* topic = ROTS_MQTT_TOPIC_COMMAND;
    uint8_t topic_len = strlen(topic);
    mqtt_subscribe_packet[packet_length++] = 0x00;  /* Topic length MSB */
    mqtt_subscribe_packet[packet_length++] = topic_len;  /* Topic length LSB */
    memcpy(&mqtt_subscribe_packet[packet_length], topic, topic_len);
    packet_length += topic_len;
    
    /* QoS */
    mqtt_subscribe_packet[packet_length++] = 0x01;  /* QoS level 1 */
    
    /* Fill remaining length */
    remaining_length = packet_length - 2;
    mqtt_subscribe_packet[1] = remaining_length;
    
    /* Send MQTT SUBSCRIBE packet */
    if (ROTS_Communication_SendMQTTPacket(mqtt_subscribe_packet, packet_length) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    HAL_Delay(500);
    
    mqtt_connected = true;
    DEBUG_INFO("MQTT connected\r\n");
    return ROTS_OK;
}

/**
 * @brief Send AT command to ESP8266
 * @param cmd AT command
 * @param response Response buffer
 * @param response_size Response buffer size
 * @param timeout Timeout in milliseconds
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Communication_SendATCommand(const char* cmd, char* response, uint16_t response_size, uint32_t timeout)
{
    if (!cmd) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Clear response buffer */
    if (response) {
        memset(response, 0, response_size);
    }
    
    /* Send command */
    HAL_UART_Transmit(&huart_esp8266, (uint8_t*)cmd, strlen(cmd), timeout);
    
    /* Wait for response */
    uint32_t start_time = HAL_GetTick();
    uint16_t response_index = 0;
    
    while ((HAL_GetTick() - start_time) < timeout) {
        uint8_t byte = 0;
        if (HAL_UART_Receive(&huart_esp8266, &byte, 1, 100) == HAL_OK) {
            if (response && response_index < (response_size - 1)) {
                response[response_index++] = byte;
            }
            
            /* Check for OK response */
            if (response_index >= 2) {
                if (strstr(response, "OK") != NULL) {
                    return ROTS_OK;
                }
                if (strstr(response, "ERROR") != NULL) {
                    return ROTS_ERROR;
                }
            }
        }
    }
    
    return ROTS_TIMEOUT;
}

/**
 * @brief Wait for specific response
 * @param expected Expected response string
 * @param timeout Timeout in milliseconds
 * @return ROTS_OK if expected response received, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Communication_WaitForResponse(const char* expected, uint32_t timeout)
{
    if (!expected) {
        return ROTS_INVALID_PARAM;
    }
    
    char response[256] = {0};
    uint32_t start_time = HAL_GetTick();
    uint16_t response_index = 0;
    
    while ((HAL_GetTick() - start_time) < timeout) {
        uint8_t byte = 0;
        if (HAL_UART_Receive(&huart_esp8266, &byte, 1, 100) == HAL_OK) {
            if (response_index < (sizeof(response) - 1)) {
                response[response_index++] = byte;
                response[response_index] = '\0';
            }
            
            /* Check for expected response */
            if (strstr(response, expected) != NULL) {
                return ROTS_OK;
            }
        }
    }
    
    return ROTS_TIMEOUT;
}

/**
 * @brief Send MQTT packet via ESP8266
 * @param packet MQTT packet data
 * @param length Packet length
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Communication_SendMQTTPacket(const uint8_t* packet, uint16_t length)
{
    if (!packet || length == 0) {
        return ROTS_INVALID_PARAM;
    }
    
    char at_cmd[50];
    
    /* Send AT+CIPSEND command */
    sprintf(at_cmd, "AT+CIPSEND=0,%d\r\n", length);
    if (ROTS_Communication_SendATCommand(at_cmd, NULL, 0, 1000) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    /* Wait for '>' prompt */
    HAL_Delay(100);
    
    /* Send packet data */
    HAL_UART_Transmit(&huart_esp8266, packet, length, 1000);
    
    return ROTS_OK;
}

/**
 * @brief Parse MQTT message and extract JSON data
 * @param data MQTT message data
 * @param length Data length
 * @param msg Output message structure
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Communication_ParseMQTTMessage(const uint8_t* data, uint16_t length, ROTS_MessageTypeDef* msg)
{
    if (!data || !msg || length == 0) {
        return ROTS_INVALID_PARAM;
    }
    
    /* MQTT PUBLISH packet format:
     * Byte 0: Fixed header (0x30 for PUBLISH, QoS 0)
     * Byte 1: Remaining length
     * Bytes 2-3: Topic length
     * Bytes 4-N: Topic name
     * Bytes N+1-M: Payload (JSON)
     */
    
    /* Skip MQTT header and topic */
    uint16_t offset = 0;
    if (data[0] != 0x30) {
        return ROTS_INVALID_PARAM;  /* Not a PUBLISH packet */
    }
    
    offset = 1;  /* Skip fixed header */
    
    /* Skip remaining length */
    offset++;
    
    /* Skip topic length (2 bytes) */
    uint16_t topic_len = (data[offset] << 8) | data[offset + 1];
    offset += 2;
    
    /* Skip topic name */
    offset += topic_len;
    
    /* Extract JSON payload */
    uint16_t payload_length = length - offset;
    if (payload_length == 0) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Parse JSON payload */
    /* JSON format: {"message_type":1,"odor_type":1,"intensity":50,"duration":30,"pump_config":[0,0,0,0,0],"timestamp":1234567890,"checksum":1234} */
    char json_buffer[256];
    if (payload_length > sizeof(json_buffer) - 1) {
        payload_length = sizeof(json_buffer) - 1;
    }
    memcpy(json_buffer, &data[offset], payload_length);
    json_buffer[payload_length] = '\0';
    
    /* Simple JSON parsing (extract values) */
    /* In real implementation, use a JSON parser library */
    /* For now, parse manually */
    char* token = strtok(json_buffer, "{}\",:[] ");
    uint8_t field_index = 0;
    uint32_t temp_value = 0;
    
    memset(msg, 0, sizeof(ROTS_MessageTypeDef));
    
    while (token != NULL) {
        if (strcmp(token, "message_type") == 0) {
            token = strtok(NULL, "{}\",:[] ");
            if (token) {
                msg->message_type = atoi(token);
            }
        } else if (strcmp(token, "odor_type") == 0) {
            token = strtok(NULL, "{}\",:[] ");
            if (token) {
                msg->odor_type = atoi(token);
            }
        } else if (strcmp(token, "intensity") == 0) {
            token = strtok(NULL, "{}\",:[] ");
            if (token) {
                msg->intensity = atoi(token);
            }
        } else if (strcmp(token, "duration") == 0) {
            token = strtok(NULL, "{}\",:[] ");
            if (token) {
                msg->duration = atoi(token);
            }
        } else if (strcmp(token, "timestamp") == 0) {
            token = strtok(NULL, "{}\",:[] ");
            if (token) {
                msg->timestamp = strtoul(token, NULL, 10);
            }
        } else if (strcmp(token, "checksum") == 0) {
            token = strtok(NULL, "{}\",:[] ");
            if (token) {
                msg->checksum = atoi(token);
            }
        } else if (strcmp(token, "pump_config") == 0) {
            /* Parse pump_config array */
            for (int i = 0; i < 5; i++) {
                token = strtok(NULL, "{}\",:[] ");
                if (token) {
                    msg->pump_config[i] = atoi(token);
                }
            }
        }
        
        token = strtok(NULL, "{}\",:[] ");
    }
    
    return ROTS_OK;
}

/**
 * @brief Receive message from MQTT
 * @param message Output message structure
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_ReceiveMessage(ROTS_MessageTypeDef* message)
{
    if (!message) {
        return ROTS_INVALID_PARAM;
    }
    
    if (message_received && uart_rx_complete) {
        /* Parse received MQTT message */
        ROTS_StatusTypeDef status = ROTS_Communication_ParseMQTTMessage(uart_rx_buffer, uart_rx_index, &rx_buffer);
        
        if (status == ROTS_OK) {
            /* Validate message */
            if (ROTS_Communication_ValidateMessage(&rx_buffer) == ROTS_OK) {
                memcpy(message, &rx_buffer, sizeof(ROTS_MessageTypeDef));
                message_received = false;
                uart_rx_complete = false;
                uart_rx_index = 0;
                last_communication_time = HAL_GetTick();
                
                /* Restart reception */
                HAL_UART_Receive_IT(&huart_esp8266, uart_rx_buffer, sizeof(uart_rx_buffer));
                
                return ROTS_OK;
            } else {
                message_received = false;
                uart_rx_complete = false;
                uart_rx_index = 0;
                return ROTS_COMM_ERROR;
            }
        } else {
            message_received = false;
            uart_rx_complete = false;
            uart_rx_index = 0;
            return status;
        }
    }
    
    /* Check for communication timeout */
    if ((HAL_GetTick() - last_communication_time) > ROTS_COMM_TIMEOUT && last_communication_time != 0) {
        return ROTS_TIMEOUT;
    }
    
    return ROTS_BUSY;
}

/**
 * @brief Send status message to cloud server via MQTT
 * @param status System status structure
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_SendStatus(ROTS_SystemStatus_t* status)
{
    if (!status || !mqtt_connected) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Build JSON status message */
    char json_buffer[256];
    snprintf(json_buffer, sizeof(json_buffer),
        "{\"device_id\":\"%s\",\"state\":%d,\"error_count\":%d,\"uptime\":%lu,\"temperature\":%.1f,\"humidity\":%.1f}",
        ROTS_MQTT_CLIENT_ID,
        status->state,
        status->error_count,
        status->uptime,
        status->temperature,
        status->humidity);
    
    /* Build MQTT PUBLISH packet */
    uint8_t mqtt_publish_packet[512];
    uint16_t packet_length = 0;
    uint16_t json_length = strlen(json_buffer);
    
    /* Fixed header */
    mqtt_publish_packet[packet_length++] = 0x30;  /* PUBLISH packet, QoS 0 */
    packet_length++;  /* Remaining length (fill later) */
    
    /* Topic */
    const char* topic = ROTS_MQTT_TOPIC_STATUS;
    uint8_t topic_len = strlen(topic);
    mqtt_publish_packet[packet_length++] = 0x00;  /* Topic length MSB */
    mqtt_publish_packet[packet_length++] = topic_len;  /* Topic length LSB */
    memcpy(&mqtt_publish_packet[packet_length], topic, topic_len);
    packet_length += topic_len;
    
    /* Payload */
    memcpy(&mqtt_publish_packet[packet_length], json_buffer, json_length);
    packet_length += json_length;
    
    /* Fill remaining length */
    uint8_t remaining_length = packet_length - 2;
    mqtt_publish_packet[1] = remaining_length;
    
    /* Send MQTT packet */
    if (ROTS_Communication_SendMQTTPacket(mqtt_publish_packet, packet_length) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Send error message to cloud server via MQTT
 * @param error_code Error code
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_SendError(ROTS_StatusTypeDef error_code)
{
    if (!mqtt_connected) {
        return ROTS_COMM_ERROR;
    }
    
    /* Build JSON error message */
    char json_buffer[128];
    snprintf(json_buffer, sizeof(json_buffer),
        "{\"device_id\":\"%s\",\"error_code\":%d,\"timestamp\":%lu}",
        ROTS_MQTT_CLIENT_ID,
        error_code,
        HAL_GetTick());
    
    /* Build MQTT PUBLISH packet */
    uint8_t mqtt_publish_packet[256];
    uint16_t packet_length = 0;
    uint16_t json_length = strlen(json_buffer);
    
    /* Fixed header */
    mqtt_publish_packet[packet_length++] = 0x30;  /* PUBLISH packet, QoS 0 */
    packet_length++;  /* Remaining length */
    
    /* Topic */
    const char* topic = ROTS_MQTT_TOPIC_ERROR;
    uint8_t topic_len = strlen(topic);
    mqtt_publish_packet[packet_length++] = 0x00;  /* Topic length MSB */
    mqtt_publish_packet[packet_length++] = topic_len;  /* Topic length LSB */
    memcpy(&mqtt_publish_packet[packet_length], topic, topic_len);
    packet_length += topic_len;
    
    /* Payload */
    memcpy(&mqtt_publish_packet[packet_length], json_buffer, json_length);
    packet_length += json_length;
    
    /* Fill remaining length */
    uint8_t remaining_length = packet_length - 2;
    mqtt_publish_packet[1] = remaining_length;
    
    /* Send MQTT packet */
    if (ROTS_Communication_SendMQTTPacket(mqtt_publish_packet, packet_length) != ROTS_OK) {
        return ROTS_COMM_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Validate received message
 * @param msg Pointer to message structure
 * @return ROTS_OK if valid, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Communication_ValidateMessage(ROTS_MessageTypeDef* msg)
{
    if (!msg) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Check message type */
    if (msg->message_type < ROTS_MSG_ODOR_COMMAND || msg->message_type > ROTS_MSG_EMERGENCY_STOP) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Check odor type */
    if (msg->odor_type < ROTS_ODOR_COFFEE || msg->odor_type > ROTS_ODOR_MIXED) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Check intensity */
    if (msg->intensity > ROTS_MAX_INTENSITY) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Check duration */
    if (msg->duration > ROTS_MAX_DURATION) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Verify checksum */
    uint16_t calculated_checksum = ROTS_Communication_CalculateChecksum(msg);
    if (calculated_checksum != msg->checksum) {
        DEBUG_ERROR("Checksum mismatch: calculated=0x%04X, received=0x%04X\r\n", calculated_checksum, msg->checksum);
        return ROTS_COMM_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Calculate message checksum
 * @param msg Pointer to message structure
 * @return Calculated checksum
 */
static uint16_t ROTS_Communication_CalculateChecksum(ROTS_MessageTypeDef* msg)
{
    if (!msg) {
        return 0;
    }
    
    uint16_t checksum = 0;
    uint8_t* data = (uint8_t*)msg;
    
    /* Calculate checksum for all fields except checksum itself */
    for (uint16_t i = 0; i < (sizeof(ROTS_MessageTypeDef) - sizeof(uint16_t)); i++) {
        checksum += data[i];
    }
    
    return checksum;
}

/**
 * @brief UART receive callback
 * @param huart UART handle
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        /* Check for MQTT message indicators */
        /* ESP8266 sends: +IPD,0,<length>:<data> */
        if (uart_rx_index < sizeof(uart_rx_buffer) - 1) {
            uart_rx_buffer[uart_rx_index] = '\0';
            
            /* Check for MQTT message indicator */
            if (strstr((char*)uart_rx_buffer, "+IPD") != NULL) {
                /* Extract message length and data */
                char* ipd_ptr = strstr((char*)uart_rx_buffer, "+IPD");
                if (ipd_ptr) {
                    /* Parse: +IPD,0,<length>:<data> */
                    char* comma1 = strchr(ipd_ptr, ',');
                    if (comma1) {
                        char* comma2 = strchr(comma1 + 1, ',');
                        if (comma2) {
                            char* colon = strchr(comma2 + 1, ':');
                            if (colon) {
                                /* Extract length */
                                uint16_t msg_length = atoi(comma2 + 1);
                                
                                /* Extract message data */
                                uint8_t* msg_data = (uint8_t*)(colon + 1);
                                
                                /* Parse MQTT message */
                                ROTS_MessageTypeDef parsed_msg;
                                if (ROTS_Communication_ParseMQTTMessage(msg_data, msg_length, &parsed_msg) == ROTS_OK) {
                                    memcpy(&rx_buffer, &parsed_msg, sizeof(ROTS_MessageTypeDef));
                                    message_received = true;
                                    uart_rx_complete = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        /* Restart reception */
        uart_rx_index = 0;
        HAL_UART_Receive_IT(&huart_esp8266, uart_rx_buffer, sizeof(uart_rx_buffer));
    }
}

/**
 * @brief UART receive half complete callback
 * @param huart UART handle
 */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        uart_rx_index = sizeof(uart_rx_buffer) / 2;
    }
}

/**
 * @brief Update communication module (check for incoming messages)
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_Update(void)
{
    /* Check for incoming UART data */
    if (HAL_UART_GetState(&huart_esp8266) == HAL_UART_STATE_READY) {
        if (!uart_rx_complete) {
            HAL_UART_Receive_IT(&huart_esp8266, uart_rx_buffer, sizeof(uart_rx_buffer));
        }
    }
    
    /* Check WiFi and MQTT connection status */
    if (wifi_connected && mqtt_connected) {
        /* Send keep-alive if needed */
        static uint32_t last_keepalive = 0;
        if ((HAL_GetTick() - last_keepalive) >= 30000) {  /* Every 30 seconds */
            ROTS_Communication_KeepAlive();
            last_keepalive = HAL_GetTick();
        }
    }
    
    return ROTS_OK;
}

/**
 * @brief Keep alive communication (send heartbeat)
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Communication_KeepAlive(void)
{
    if (!mqtt_connected) {
        return ROTS_COMM_ERROR;
    }
    
    /* Send MQTT PINGREQ packet */
    uint8_t pingreq_packet[2] = {0xC0, 0x00};
    return ROTS_Communication_SendMQTTPacket(pingreq_packet, 2);
}

/**
 * @brief MQTT message callback (not used in AT command mode, kept for compatibility)
 * @param topic Topic name
 * @param payload Message payload
 * @param length Payload length
 */
void ROTS_MQTT_MessageCallback(char* topic, char* payload, int length)
{
    /* In AT command mode, messages are received via UART interrupt */
    /* This callback is kept for compatibility but not actively used */
    (void)topic;
    (void)payload;
    (void)length;
}

/**
 * @brief MQTT connect callback (not used in AT command mode, kept for compatibility)
 */
void ROTS_MQTT_ConnectCallback(void)
{
    /* Connection status is checked via AT commands */
    mqtt_connected = true;
    DEBUG_INFO("MQTT connected callback\r\n");
}

/**
 * @brief MQTT disconnect callback (not used in AT command mode, kept for compatibility)
 */
void ROTS_MQTT_DisconnectCallback(void)
{
    /* Disconnection is detected via AT commands */
    mqtt_connected = false;
    DEBUG_WARNING("MQTT disconnected callback\r\n");
}
