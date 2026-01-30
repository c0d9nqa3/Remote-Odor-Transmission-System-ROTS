// ROTS Debug Module - Debug Module
#include "rots_sender.h"
#include "rots_debug.h"
#include "rots_sensor_manager.h"
#include "rots_ai_engine.h"
#include "rots_communication.h"
#include "rots_system_monitor.h"

// Debug level
static ROTS_DebugLevel_t debug_level = ROTS_DEBUG_INFO;

// Initialize debug system
ROTS_StatusTypeDef ROTS_Debug_Init(void) {
    // Configure debug LEDs
    pinMode(ROTS_ERROR_LED_PIN, OUTPUT);
    pinMode(ROTS_STATUS_LED_PIN, OUTPUT);
    
    digitalWrite(ROTS_ERROR_LED_PIN, LOW);
    digitalWrite(ROTS_STATUS_LED_PIN, LOW);
    
    DEBUG_INFO("Debug system initialized\r\n");
    return ROTS_OK;
}

// Set debug level
void ROTS_Debug_SetLevel(ROTS_DebugLevel_t level) {
    debug_level = level;
}

// Print debug information
void ROTS_Debug_Print(ROTS_DebugLevel_t level, const char* format, ...) {
    if (level > debug_level) return;
    
    char buffer[256];
    char timestamp[20];
    uint32_t tick = millis();
    
    // Add timestamp
    sprintf(timestamp, "[%lu] ", tick);
    strcpy(buffer, timestamp);
    
    // Add level identifier
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
    
    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, args);
    va_end(args);
    
    // Send to serial port
    Serial.print(buffer);
}

// Print hexadecimal data
void ROTS_Debug_PrintHex(ROTS_DebugLevel_t level, const char* label, const uint8_t* data, uint16_t length) {
    if (level > debug_level) return;
    
    char buffer[512];
    char hex_str[4];
    
    sprintf(buffer, "[%lu] %s: ", millis(), label);
    
    for (uint16_t i = 0; i < length; i++) {
        sprintf(hex_str, "%02X ", data[i]);
        strcat(buffer, hex_str);
        
        if ((i + 1) % 16 == 0) {
            strcat(buffer, "\r\n");
            Serial.print(buffer);
            strcpy(buffer, "        ");
        }
    }
    
    if (length % 16 != 0) {
        strcat(buffer, "\r\n");
    }
    
    Serial.print(buffer);
}

// Print system status
void ROTS_Debug_PrintSystemStatus(void) {
    DEBUG_INFO("=== System Status ===\r\n");
    DEBUG_INFO("Free Heap: %lu bytes\r\n", ESP.getFreeHeap());
    DEBUG_INFO("Free PSRAM: %lu bytes\r\n", ESP.getFreePsram());
    DEBUG_INFO("CPU Frequency: %lu MHz\r\n", ESP.getCpuFreqMHz());
    DEBUG_INFO("Uptime: %lu seconds\r\n", millis() / 1000);
}

// Print sensor status
void ROTS_Debug_PrintSensorStatus(void) {
    ROTS_SensorStatus_t status;
    if (ROTS_SensorManager_GetStatus(&status) == ROTS_OK) {
        DEBUG_INFO("=== Sensor Status ===\r\n");
        DEBUG_INFO("Initialized: %s\r\n", status.initialized ? "Yes" : "No");
        DEBUG_INFO("Temperature: %.1f°C\r\n", status.temperature);
        DEBUG_INFO("Humidity: %.1f%%\r\n", status.humidity);
        DEBUG_INFO("Pressure: %.1f hPa\r\n", status.pressure);
        DEBUG_INFO("Health: %d%%\r\n", status.sensor_health);
    }
}

// Print AI status
void ROTS_Debug_PrintAIStatus(void) {
    ROTS_AIStatus_t status;
    if (ROTS_AIEngine_GetStatus(&status) == ROTS_OK) {
        DEBUG_INFO("=== AI Status ===\r\n");
        DEBUG_INFO("Initialized: %s\r\n", status.initialized ? "Yes" : "No");
        DEBUG_INFO("Last Inference: %lu\r\n", status.last_inference_time);
        DEBUG_INFO("Last Odor: %d\r\n", status.last_odor_type);
        DEBUG_INFO("Last Confidence: %.2f\r\n", status.last_confidence);
        DEBUG_INFO("Inference Count: %lu\r\n", status.inference_count);
    }
}

// Print communication status
void ROTS_Debug_PrintCommStatus(void) {
    ROTS_CommStatus_t status;
    if (ROTS_Communication_GetStatus(&status) == ROTS_OK) {
        DEBUG_INFO("=== Communication Status ===\r\n");
        DEBUG_INFO("WiFi Connected: %s\r\n", status.wifi_connected ? "Yes" : "No");
        DEBUG_INFO("MQTT Connected: %s\r\n", status.mqtt_connected ? "Yes" : "No");
        DEBUG_INFO("WiFi RSSI: %ld dBm\r\n", status.wifi_rssi);
        DEBUG_INFO("Last Heartbeat: %lu\r\n", status.last_heartbeat);
    }
}

// Print memory usage
void ROTS_Debug_PrintMemoryUsage(void) {
    DEBUG_INFO("=== Memory Usage ===\r\n");
    DEBUG_INFO("Free Heap: %lu bytes\r\n", ESP.getFreeHeap());
    DEBUG_INFO("Free PSRAM: %lu bytes\r\n", ESP.getFreePsram());
    DEBUG_INFO("Heap Size: %lu bytes\r\n", ESP.getHeapSize());
    DEBUG_INFO("PSRAM Size: %lu bytes\r\n", ESP.getPsramSize());
}

// Print error information
void ROTS_Debug_PrintError(ROTS_StatusTypeDef error_code) {
    const char* error_messages[] = {
        "OK",
        "ERROR",
        "BUSY",
        "TIMEOUT",
        "INVALID_PARAM",
        "COMM_ERROR",
        "SENSOR_ERROR",
        "AI_ERROR",
        "MEMORY_ERROR"
    };
    
    if (error_code < sizeof(error_messages) / sizeof(char*)) {
        DEBUG_ERROR("Error: %s (%d)\r\n", error_messages[error_code], error_code);
    } else {
        DEBUG_ERROR("Unknown Error: %d\r\n", error_code);
    }
}

// Blink LED indicator
void ROTS_Debug_BlinkLED(uint8_t pin, uint8_t times, uint16_t delay_ms) {
    for (uint8_t i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delay_ms);
        digitalWrite(pin, LOW);
        delay(delay_ms);
    }
}

// Error LED indicator
void ROTS_Debug_ErrorLED(bool state) {
    digitalWrite(ROTS_ERROR_LED_PIN, state ? HIGH : LOW);
}

// Status LED indicator
void ROTS_Debug_StatusLED(bool state) {
    digitalWrite(ROTS_STATUS_LED_PIN, state ? HIGH : LOW);
}
