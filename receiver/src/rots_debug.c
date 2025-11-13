/**
 * @file rots_debug.c
 * @brief ROTS Debug Module
 * @author ROTS Team
 * @date 2024
 * 
 * Debug and logging functionality for STM32
 */

#include "rots_receiver.h"
#include "rots_debug.h"
#include "rots_system_monitor.h"
#include "rots_communication.h"
#include <stdio.h>
#include <stdarg.h>

/* Debug UART handle */
static UART_HandleTypeDef huart_debug;

/* Debug level */
static ROTS_DebugLevel_t debug_level = ROTS_DEBUG_INFO;

/* External variables */
extern bool wifi_connected;
extern bool mqtt_connected;

/**
 * @brief Initialize debug UART
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Debug_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable clocks */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Configure USART1 pins (PA9=TX, PA10=RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Configure USART1 for debug output */
    huart_debug.Instance = USART1;
    huart_debug.Init.BaudRate = 115200;
    huart_debug.Init.WordLength = UART_WORDLENGTH_8B;
    huart_debug.Init.StopBits = UART_STOPBITS_1;
    huart_debug.Init.Parity = UART_PARITY_NONE;
    huart_debug.Init.Mode = UART_MODE_TX_RX;
    huart_debug.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_debug.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart_debug) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    /* Send startup message */
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "ROTS Debug System Started\r\n");
    
    return ROTS_OK;
}

/**
 * @brief Set debug level
 * @param level Debug level
 */
void ROTS_Debug_SetLevel(ROTS_DebugLevel_t level)
{
    debug_level = level;
}

/**
 * @brief Print debug information
 * @param level Debug level
 * @param format Format string
 * @param ... Variable arguments
 */
void ROTS_Debug_Print(ROTS_DebugLevel_t level, const char* format, ...)
{
    if (level > debug_level) return;
    
    char buffer[256];
    char timestamp[20];
    uint32_t tick = HAL_GetTick();
    
    /* Add timestamp */
    sprintf(timestamp, "[%lu] ", tick);
    strcpy(buffer, timestamp);
    
    /* Add level identifier */
    switch (level) {
        case ROTS_DEBUG_ERROR:
            strcat(buffer, "ERROR: ");
            break;
        case ROTS_DEBUG_WARNING:
            strcat(buffer, "WARN:  ");
            break;
        case ROTS_DEBUG_INFO:
            strcat(buffer, "INFO:  ");
            break;
        case ROTS_DEBUG_DEBUG:
            strcat(buffer, "DEBUG: ");
            break;
    }
    
    /* Format message */
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, args);
    va_end(args);
    
    /* Send to UART */
    HAL_UART_Transmit(&huart_debug, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
 * @brief Print hexadecimal data
 * @param level Debug level
 * @param label Label for data
 * @param data Data buffer
 * @param length Data length
 */
void ROTS_Debug_PrintHex(ROTS_DebugLevel_t level, const char* label, const uint8_t* data, uint16_t length)
{
    if (level > debug_level || !data || length == 0) return;
    
    char buffer[512];
    char hex_str[4];
    
    sprintf(buffer, "[%lu] %s: ", HAL_GetTick(), label);
    
    for (uint16_t i = 0; i < length && i < 256; i++) {
        sprintf(hex_str, "%02X ", data[i]);
        strcat(buffer, hex_str);
        
        if ((i + 1) % 16 == 0) {
            strcat(buffer, "\r\n");
            HAL_UART_Transmit(&huart_debug, (uint8_t*)buffer, strlen(buffer), 1000);
            strcpy(buffer, "        ");
        }
    }
    
    if (length % 16 != 0) {
        strcat(buffer, "\r\n");
    }
    
    HAL_UART_Transmit(&huart_debug, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
 * @brief Print system status
 */
void ROTS_Debug_PrintSystemStatus(void)
{
    ROTS_SystemStatus_t status;
    if (ROTS_SystemMonitor_GetStatus(&status) != ROTS_OK) {
        return;
    }
    
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "=== System Status ===\r\n");
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "State: %d\r\n", status.state);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Uptime: %lu seconds\r\n", status.uptime);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Error Count: %d\r\n", status.error_count);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Temperature: %.1f°C\r\n", status.temperature);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Humidity: %.1f%%\r\n", status.humidity);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Communication: %s\r\n", status.communication_active ? "Active" : "Inactive");
    
    /* Print pump status */
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Pump Status: ");
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        ROTS_Debug_Print(ROTS_DEBUG_INFO, "%d ", status.pump_status[i]);
    }
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "\r\n");
    
    /* Print valve status */
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Valve Status: ");
    for (int i = 0; i < ROTS_MAX_VALVES; i++) {
        ROTS_Debug_Print(ROTS_DEBUG_INFO, "%d ", status.valve_status[i]);
    }
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "\r\n");
}

/**
 * @brief Print message content
 * @param message Message structure
 */
void ROTS_Debug_PrintMessage(const ROTS_MessageTypeDef* message)
{
    if (!message) return;
    
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "=== Received Message ===\r\n");
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Type: %d\r\n", message->message_type);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Odor Type: %d\r\n", message->odor_type);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Intensity: %d%%\r\n", message->intensity);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Duration: %d seconds\r\n", message->duration);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Timestamp: %lu\r\n", message->timestamp);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Checksum: 0x%04X\r\n", message->checksum);
    
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Pump Config: ");
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        ROTS_Debug_Print(ROTS_DEBUG_INFO, "%d ", message->pump_config[i]);
    }
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "\r\n");
}

/**
 * @brief Print error information
 * @param error_code Error code
 */
void ROTS_Debug_PrintError(ROTS_StatusTypeDef error_code)
{
    const char* error_messages[] = {
        "OK",
        "ERROR",
        "BUSY",
        "TIMEOUT",
        "INVALID_PARAM",
        "COMM_ERROR",
        "ACTUATOR_ERROR",
        "RECIPE_ERROR",
        "DISPLAY_ERROR",
        "MEMORY_ERROR"
    };
    
    if (error_code < sizeof(error_messages) / sizeof(char*)) {
        ROTS_Debug_Print(ROTS_DEBUG_ERROR, "Error: %s (%d)\r\n", error_messages[error_code], error_code);
    } else {
        ROTS_Debug_Print(ROTS_DEBUG_ERROR, "Unknown Error: %d\r\n", error_code);
    }
}

/**
 * @brief Print WiFi status
 */
void ROTS_Debug_PrintWiFiStatus(void)
{
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "=== WiFi Status ===\r\n");
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "SSID: %s\r\n", ROTS_WIFI_SSID);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Connected: %s\r\n", wifi_connected ? "Yes" : "No");
}

/**
 * @brief Print MQTT status
 */
void ROTS_Debug_PrintMQTTStatus(void)
{
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "=== MQTT Status ===\r\n");
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Broker: %s:%d\r\n", ROTS_MQTT_BROKER_HOST, ROTS_MQTT_BROKER_PORT);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Client ID: %s\r\n", ROTS_MQTT_CLIENT_ID);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Connected: %s\r\n", mqtt_connected ? "Yes" : "No");
}

/**
 * @brief Print memory usage
 */
void ROTS_Debug_PrintMemoryUsage(void)
{
    extern uint32_t _end;
    extern uint32_t _sdata;
    extern uint32_t _estack;
    
    uint32_t stack_used = (uint32_t)&_estack - (uint32_t)__get_MSP();
    uint32_t heap_used = (uint32_t)&_end - (uint32_t)&_sdata;
    
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "=== Memory Usage ===\r\n");
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Stack Used: %lu bytes\r\n", stack_used);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Heap Used: %lu bytes\r\n", heap_used);
    ROTS_Debug_Print(ROTS_DEBUG_INFO, "Free Stack: %lu bytes\r\n", (uint32_t)&_estack - (uint32_t)__get_MSP());
}
