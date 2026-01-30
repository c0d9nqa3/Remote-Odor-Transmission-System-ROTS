/**
 * @file rots_system_monitor.c
 * @brief ROTS System Monitor Module
 * @author ROTS Team
 * @date 2024
 * 
 * Monitors system health and logs events
 */

#include "rots_receiver.h"
#include "rots_system_monitor.h"
#include "rots_actuator_control.h"
#include "rots_hardware.h"
#include <stdio.h>
#include <string.h>

/* Private variables */
static ROTS_SystemStatus_t system_status;
static uint32_t error_log[32];
static uint8_t error_count = 0;
static uint32_t system_start_time = 0;
static bool monitor_initialized = false;

/* Private function prototypes */
static void ROTS_SystemMonitor_UpdateTemperature(void);
static void ROTS_SystemMonitor_UpdateHumidity(void);
static void ROTS_SystemMonitor_CheckActuators(void);

/**
 * @brief Initialize system monitor
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_Init(void)
{
    // Initialize system status
    memset(&system_status, 0, sizeof(ROTS_SystemStatus_t));
    system_status.state = ROTS_SYSTEM_IDLE;
    system_status.communication_active = false;
    
    // Initialize error log
    memset(error_log, 0, sizeof(error_log));
    error_count = 0;
    
    // Record system start time
    system_start_time = HAL_GetTick();
    
    monitor_initialized = true;
    return ROTS_OK;
}

/**
 * @brief Update system monitor
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_Update(void)
{
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    // Update system uptime
    system_status.uptime = (HAL_GetTick() - system_start_time) / 1000;
    
    // Update environmental sensors
    ROTS_SystemMonitor_UpdateTemperature();
    ROTS_SystemMonitor_UpdateHumidity();
    
    // Check actuator status
    ROTS_SystemMonitor_CheckActuators();
    
    // Update communication status
    // This would typically check if we received data recently
    system_status.communication_active = true;
    
    return ROTS_OK;
}

/**
 * @brief Log system error
 * @param error_code Error code
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_LogError(ROTS_StatusTypeDef error_code)
{
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    // Add error to log
    if (error_count < 32) {
        error_log[error_count] = (uint32_t)error_code;
        error_count++;
    } else {
        // Shift log entries
        for (int i = 0; i < 31; i++) {
            error_log[i] = error_log[i + 1];
        }
        error_log[31] = (uint32_t)error_code;
    }
    
    // Update error count
    system_status.error_count = error_count;
    
    // Set system state to error if critical
    if (error_code == ROTS_ERROR || error_code == ROTS_ACTUATOR_ERROR) {
        system_status.state = ROTS_SYSTEM_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Get system status
 * @param status Pointer to status structure
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_GetStatus(ROTS_SystemStatus_t* status)
{
    if (!monitor_initialized || !status) {
        return ROTS_INVALID_PARAM;
    }
    
    // Copy current status
    memcpy(status, &system_status, sizeof(ROTS_SystemStatus_t));
    
    return ROTS_OK;
}

/**
 * @brief Get error log
 * @param log Array to store error log
 * @param max_count Maximum number of errors to return
 * @param actual_count Pointer to actual count returned
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_GetErrorLog(uint32_t* log, uint8_t max_count, uint8_t* actual_count)
{
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

/**
 * @brief Clear error log
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_ClearErrorLog(void)
{
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    memset(error_log, 0, sizeof(error_log));
    error_count = 0;
    system_status.error_count = 0;
    
    return ROTS_OK;
}

/**
 * @brief Set system state
 * @param state New system state
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemMonitor_SetState(ROTS_SystemState_t state)
{
    if (!monitor_initialized) {
        return ROTS_ERROR;
    }
    
    system_status.state = state;
    return ROTS_OK;
}

/**
 * @brief Update temperature reading from hardware sensor
 */
static void ROTS_SystemMonitor_UpdateTemperature(void)
{
    // Read temperature from internal temperature sensor
    system_status.temperature = ROTS_Hardware_ReadTemperature();
}

/**
 * @brief Update humidity reading from hardware sensor
 */
static void ROTS_SystemMonitor_UpdateHumidity(void)
{
    // Read humidity from hardware sensor (if available)
    // Note: STM32F4 does not have built-in humidity sensor
    // This requires external sensor or returns default value
    system_status.humidity = ROTS_Hardware_ReadHumidity();
}

/**
 * @brief Check actuator status
 */
static void ROTS_SystemMonitor_CheckActuators(void)
{
    // Get actuator status from actuator control module
    ROTS_ActuatorControl_GetStatus(system_status.pump_status, system_status.valve_status);
    
    // Check for actuator errors
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        if (system_status.pump_status[i] == ROTS_ACTUATOR_ERROR) {
            ROTS_SystemMonitor_LogError(ROTS_ACTUATOR_ERROR);
        }
    }
    
    for (int i = 0; i < ROTS_MAX_VALVES; i++) {
        if (system_status.valve_status[i] == ROTS_ACTUATOR_ERROR) {
            ROTS_SystemMonitor_LogError(ROTS_ACTUATOR_ERROR);
        }
    }
}
