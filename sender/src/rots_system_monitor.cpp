// ROTS System Monitor - System Monitoring Module
#include "rots_sender.h"
#include "rots_system_monitor.h"
#include "rots_debug.h"
#include <WiFi.h>
#include <cstring>

// Private variables
static bool monitor_initialized = false;
static uint32_t error_log[32];
static uint8_t error_count = 0;
static uint32_t system_start_time = 0;

// Battery voltage monitoring pin
#define BATTERY_VOLTAGE_PIN 35  // ADC1_CH7 on ESP32

// Initialize system monitor
ROTS_StatusTypeDef ROTS_SystemMonitor_Init(void) {
    // Initialize error log
    memset(error_log, 0, sizeof(error_log));
    error_count = 0;
    
    // Record system start time
    system_start_time = millis();
    
    // Configure battery voltage monitoring pin
    pinMode(BATTERY_VOLTAGE_PIN, INPUT);
    
    monitor_initialized = true;
    DEBUG_INFO("System monitor initialized\r\n");
    return ROTS_OK;
}

// Update system monitor
ROTS_StatusTypeDef ROTS_SystemMonitor_Update(void) {
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    // Check memory usage
    if (ESP.getFreeHeap() < 10000) { // Less than 10KB
        ROTS_SystemMonitor_LogError(ROTS_MEMORY_ERROR);
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        ROTS_SystemMonitor_LogError(ROTS_COMM_ERROR);
    }
    
    // Update status LED
    ROTS_Debug_StatusLED(WiFi.status() == WL_CONNECTED);
    
    return ROTS_OK;
}

// Log system error
ROTS_StatusTypeDef ROTS_SystemMonitor_LogError(ROTS_StatusTypeDef error_code) {
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    // Add error to log
    if (error_count < 32) {
        error_log[error_count] = (uint32_t)error_code;
        error_count++;
    } else {
        // Shift log entries (circular buffer)
        for (int i = 0; i < 31; i++) {
            error_log[i] = error_log[i + 1];
        }
        error_log[31] = (uint32_t)error_code;
    }
    
    // Error LED indication
    ROTS_Debug_ErrorLED(true);
    delay(100);
    ROTS_Debug_ErrorLED(false);
    
    DEBUG_ERROR("Error logged: %d\r\n", error_code);
    return ROTS_OK;
}

// Get system status
ROTS_StatusTypeDef ROTS_SystemMonitor_GetStatus(ROTS_SystemStatus_t* status) {
    if (!monitor_initialized || !status) {
        return ROTS_INVALID_PARAM;
    }
    
    status->uptime = (millis() - system_start_time) / 1000;
    status->free_heap = ESP.getFreeHeap();
    status->free_psram = ESP.getFreePsram();
    status->error_count = error_count;
    status->wifi_connected = (WiFi.status() == WL_CONNECTED);
    status->wifi_rssi = WiFi.RSSI();
    
    // Read battery voltage from ADC
    // ESP32 ADC: 0-4095 for 0-3.3V
    // Typical battery voltage divider: Vbat / 2 (using two equal resistors)
    // So actual battery voltage = ADC_voltage * 2
    int adc_value = analogRead(BATTERY_VOLTAGE_PIN);
    float adc_voltage = (adc_value * 3.3f) / 4095.0f;
    status->battery_voltage = adc_voltage * 2.0f; // Voltage divider ratio
    
    // Clamp battery voltage to reasonable range
    if (status->battery_voltage < 0.0f) status->battery_voltage = 0.0f;
    if (status->battery_voltage > 4.5f) status->battery_voltage = 4.5f;
    
    return ROTS_OK;
}

// Get error log
ROTS_StatusTypeDef ROTS_SystemMonitor_GetErrorLog(uint32_t* log, uint8_t max_count, uint8_t* actual_count) {
    if (!monitor_initialized || !log || !actual_count) {
        return ROTS_INVALID_PARAM;
    }
    
    uint8_t count = (error_count < max_count) ? error_count : max_count;
    
    // Copy error log
    for (int i = 0; i < count; i++) {
        log[i] = error_log[i];
    }
    
    *actual_count = count;
    return ROTS_OK;
}

// Clear error log
ROTS_StatusTypeDef ROTS_SystemMonitor_ClearErrorLog(void) {
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    memset(error_log, 0, sizeof(error_log));
    error_count = 0;
    
    DEBUG_INFO("Error log cleared\r\n");
    return ROTS_OK;
}

// Get system information
ROTS_StatusTypeDef ROTS_SystemMonitor_GetSystemInfo(ROTS_SystemInfo_t* info) {
    if (!monitor_initialized || !info) {
        return ROTS_INVALID_PARAM;
    }
    
    {
        const char* p = ESP.getChipModel();
        if (p) {
            strncpy(info->chip_model, p, sizeof(info->chip_model) - 1);
            info->chip_model[sizeof(info->chip_model) - 1] = '\0';
        } else {
            info->chip_model[0] = '\0';
        }
    }
    info->chip_revision = ESP.getChipRevision();
    info->cpu_freq = ESP.getCpuFreqMHz();
    info->flash_size = ESP.getFlashChipSize();
    info->free_heap = ESP.getFreeHeap();
    info->free_psram = ESP.getFreePsram();
    info->uptime = (millis() - system_start_time) / 1000;
    
    return ROTS_OK;
}
