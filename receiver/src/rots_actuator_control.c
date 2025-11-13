/**
 * @file rots_actuator_control.c
 * @brief ROTS Actuator Control Module
 * @author ROTS Team
 * @date 2024
 * 
 * Controls pumps, valves, and fans for odor generation
 */

#include "rots_receiver.h"
#include "rots_actuator_control.h"
#include "rots_hardware.h"
#include "rots_recipe_manager.h"
#include "rots_system_monitor.h"
#include <math.h>
#include <string.h>

/* Private variables */
TIM_HandleTypeDef htim_timer;  /* Timer for odor generation duration (exported for interrupt handler) */
static ROTS_ActuatorState_t pump_states[ROTS_MAX_PUMPS];
static ROTS_ActuatorState_t valve_states[ROTS_MAX_VALVES];
static ROTS_ActuatorState_t fan_states[ROTS_MAX_FANS];
static uint8_t pump_speeds[ROTS_MAX_PUMPS];
static bool system_initialized = false;
static uint32_t generation_start_time = 0;
static uint16_t generation_duration = 0;  /* Duration in seconds */
static bool generation_active = false;

/* Private function prototypes */
static ROTS_StatusTypeDef ROTS_ActuatorControl_InitPWM(void);
static ROTS_StatusTypeDef ROTS_ActuatorControl_InitGPIO(void);
static void ROTS_ActuatorControl_SetPumpSpeed(uint8_t pump_id, uint8_t speed);
static void ROTS_ActuatorControl_SetValveState(uint8_t valve_id, ROTS_ActuatorState_t state);
static void ROTS_ActuatorControl_SetFanSpeed(uint8_t fan_id, uint8_t speed);

/**
 * @brief Initialize actuator control system
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_Init(void)
{
    // Note: PWM and GPIO initialization is handled by ROTS_Hardware module
    // This function only initializes internal state
    
    // Initialize all actuators to OFF state using hardware interface
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        pump_states[i] = ROTS_ACTUATOR_OFF;
        pump_speeds[i] = 0;
        ROTS_Hardware_SetPumpSpeed(i, 0);
    }
    
    for (int i = 0; i < ROTS_MAX_VALVES; i++) {
        valve_states[i] = ROTS_ACTUATOR_OFF;
        ROTS_Hardware_SetValveState(i, ROTS_ACTUATOR_OFF);
    }
    
    for (int i = 0; i < ROTS_MAX_FANS; i++) {
        fan_states[i] = ROTS_ACTUATOR_OFF;
        ROTS_Hardware_SetFanSpeed(i, 0);
    }
    
    generation_active = false;
    generation_duration = 0;
    generation_start_time = 0;
    
    system_initialized = true;
    return ROTS_OK;
}

/**
 * @brief Process odor command and control actuators
 * @param message Odor command message
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_ProcessOdorCommand(ROTS_MessageTypeDef* message)
{
    if (!system_initialized) {
        return ROTS_ERROR;
    }
    
    ROTS_StatusTypeDef status = ROTS_OK;
    
    // Emergency stop check
    if (message->message_type == ROTS_MSG_EMERGENCY_STOP) {
        ROTS_ActuatorControl_EmergencyStop();
        return ROTS_OK;
    }
    
    // Process odor command
    if (message->message_type == ROTS_MSG_ODOR_COMMAND) {
        // Configure pumps based on odor type and intensity
        status = ROTS_ActuatorControl_ConfigurePumps(message);
        if (status != ROTS_OK) return status;
        
        // Configure valves
        status = ROTS_ActuatorControl_ConfigureValves(message);
        if (status != ROTS_OK) return status;
        
        // Configure fans
        status = ROTS_ActuatorControl_ConfigureFans(message);
        if (status != ROTS_OK) return status;
        
        // Start odor generation
        status = ROTS_ActuatorControl_StartOdorGeneration(message->duration);
    }
    
    return status;
}

/**
 * @brief Configure pumps for odor generation
 * @param message Odor command message
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_ConfigurePumps(ROTS_MessageTypeDef* message)
{
    // Get recipe for the specified odor type
    ROTS_Recipe_t recipe;
    ROTS_StatusTypeDef status = ROTS_RecipeManager_GetRecipe(message->odor_type, &recipe);
    if (status != ROTS_OK) return status;
    
    // Configure each pump based on recipe and intensity
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        uint8_t pump_speed = (recipe.pump_ratios[i] * message->intensity) / 100;
        ROTS_ActuatorControl_SetPumpSpeed(i, pump_speed);
    }
    
    return ROTS_OK;
}

/**
 * @brief Configure valves for odor generation
 * @param message Odor command message
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_ConfigureValves(ROTS_MessageTypeDef* message)
{
    // Get recipe for the specified odor type
    ROTS_Recipe_t recipe;
    ROTS_StatusTypeDef status = ROTS_RecipeManager_GetRecipe(message->odor_type, &recipe);
    if (status != ROTS_OK) return status;
    
    // Configure valves based on recipe
    for (int i = 0; i < ROTS_MAX_VALVES; i++) {
        ROTS_ActuatorState_t valve_state = recipe.valve_states[i] ? ROTS_ACTUATOR_ON : ROTS_ACTUATOR_OFF;
        ROTS_ActuatorControl_SetValveState(i, valve_state);
    }
    
    return ROTS_OK;
}

/**
 * @brief Configure fans for odor generation
 * @param message Odor command message
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_ConfigureFans(ROTS_MessageTypeDef* message)
{
    // Configure fans based on intensity
    uint8_t fan_speed = (message->intensity * 255) / 100;
    
    for (int i = 0; i < ROTS_MAX_FANS; i++) {
        ROTS_ActuatorControl_SetFanSpeed(i, fan_speed);
    }
    
    return ROTS_OK;
}

/**
 * @brief Start odor generation process with timer control
 * @param duration Duration in seconds
 * @return ROTS_OK if successful, error code otherwise
 */
/**
 * @brief Start odor generation process with timer control
 * @param duration Duration in seconds
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_StartOdorGeneration(uint16_t duration)
{
    if (duration == 0 || duration > ROTS_MAX_DURATION) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Start all configured pumps */
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        if (pump_speeds[i] > 0) {
            pump_states[i] = ROTS_ACTUATOR_ON;
            ROTS_Hardware_SetPumpSpeed(i, pump_speeds[i]);
            
            /* Enable pump enable pins */
            switch (i) {
                case 0:
                    HAL_GPIO_WritePin(PUMP1_EN_PORT, PUMP1_EN_PIN, GPIO_PIN_SET);
                    break;
                case 1:
                    HAL_GPIO_WritePin(PUMP2_EN_PORT, PUMP2_EN_PIN, GPIO_PIN_SET);
                    break;
                case 2:
                    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP3_EN_PIN, GPIO_PIN_SET);
                    break;
                case 3:
                    HAL_GPIO_WritePin(PUMP4_EN_PORT, PUMP4_EN_PIN, GPIO_PIN_SET);
                    break;
                case 4:
                    HAL_GPIO_WritePin(PUMP5_EN_PORT, PUMP5_EN_PIN, GPIO_PIN_SET);
                    break;
            }
        }
    }
    
    /* Store generation parameters */
    generation_start_time = HAL_GetTick();
    generation_duration = duration * 1000;  /* Convert to milliseconds */
    generation_active = true;
    
    /* Configure timer for duration control */
    /* TIM4 is used for odor generation timer */
    __HAL_RCC_TIM4_CLK_ENABLE();
    
    htim_timer.Instance = TIM4;
    htim_timer.Init.Prescaler = 8400 - 1;  /* 10kHz timer (84MHz / 8400 = 10kHz) */
    htim_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_timer.Init.Period = (duration * 10000) - 1;  /* Duration in 0.1ms units (10kHz / 10000 = 1Hz for duration seconds) */
    htim_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim_timer) != HAL_OK) {
        generation_active = false;
        return ROTS_ERROR;
    }
    
    /* Enable TIM4 interrupt */
    HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    
    /* Start timer in interrupt mode */
    if (HAL_TIM_Base_Start_IT(&htim_timer) != HAL_OK) {
        generation_active = false;
        HAL_NVIC_DisableIRQ(TIM4_IRQn);
        return ROTS_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Timer period elapsed callback
 * @param htim Timer handle
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4 && generation_active) {
        /* Timer expired, stop odor generation */
        ROTS_ActuatorControl_StopOdorGeneration();
        generation_active = false;
    }
}

/**
 * @brief Update actuator control (called from main loop)
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_Update(void)
{
    if (!system_initialized) {
        return ROTS_ERROR;
    }
    
    /* Timer expiration is handled by interrupt callback */
    /* This function is kept for compatibility and future enhancements */
    
    return ROTS_OK;
}

/**
 * @brief Stop odor generation
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_StopOdorGeneration(void)
{
    /* Stop timer if active */
    if (generation_active) {
        HAL_TIM_Base_Stop_IT(&htim_timer);
        HAL_NVIC_DisableIRQ(TIM4_IRQn);
        generation_active = false;
    }
    
    /* Stop all pumps using hardware interface */
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        pump_states[i] = ROTS_ACTUATOR_OFF;
        pump_speeds[i] = 0;
        ROTS_Hardware_SetPumpSpeed(i, 0);
        
        /* Disable pump enable pins */
        switch (i) {
            case 0:
                HAL_GPIO_WritePin(PUMP1_EN_PORT, PUMP1_EN_PIN, GPIO_PIN_RESET);
                break;
            case 1:
                HAL_GPIO_WritePin(PUMP2_EN_PORT, PUMP2_EN_PIN, GPIO_PIN_RESET);
                break;
            case 2:
                HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP3_EN_PIN, GPIO_PIN_RESET);
                break;
            case 3:
                HAL_GPIO_WritePin(PUMP4_EN_PORT, PUMP4_EN_PIN, GPIO_PIN_RESET);
                break;
            case 4:
                HAL_GPIO_WritePin(PUMP5_EN_PORT, PUMP5_EN_PIN, GPIO_PIN_RESET);
                break;
        }
    }
    
    /* Close all valves using hardware interface */
    for (int i = 0; i < ROTS_MAX_VALVES; i++) {
        valve_states[i] = ROTS_ACTUATOR_OFF;
        ROTS_Hardware_SetValveState(i, ROTS_ACTUATOR_OFF);
    }
    
    /* Stop fans using hardware interface */
    for (int i = 0; i < ROTS_MAX_FANS; i++) {
        fan_states[i] = ROTS_ACTUATOR_OFF;
        ROTS_Hardware_SetFanSpeed(i, 0);
    }
    
    return ROTS_OK;
}

/**
 * @brief Emergency stop all actuators
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_EmergencyStop(void)
{
    // Immediately stop all actuators
    ROTS_ActuatorControl_StopOdorGeneration();
    
    // Log emergency stop event
    ROTS_SystemMonitor_LogError(ROTS_ERROR);
    
    return ROTS_OK;
}

/**
 * @brief Get actuator status
 * @param pump_status Array to store pump status
 * @param valve_status Array to store valve status
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ActuatorControl_GetStatus(uint8_t* pump_status, uint8_t* valve_status)
{
    if (!pump_status || !valve_status) {
        return ROTS_INVALID_PARAM;
    }
    
    // Copy pump status
    for (int i = 0; i < ROTS_MAX_PUMPS; i++) {
        pump_status[i] = pump_states[i];
    }
    
    // Copy valve status
    for (int i = 0; i < ROTS_MAX_VALVES; i++) {
        valve_status[i] = valve_states[i];
    }
    
    return ROTS_OK;
}

/**
 * @brief Initialize PWM for pump control
 * @return ROTS_OK if successful, error code otherwise
 * @note PWM initialization is handled by ROTS_PWM_Init() in hardware module
 */
static ROTS_StatusTypeDef ROTS_ActuatorControl_InitPWM(void)
{
    // PWM initialization is handled by ROTS_PWM_Init() in hardware module
    // This function is kept for compatibility but does nothing
    return ROTS_OK;
}

/**
 * @brief Initialize GPIO for valves and fans
 * @return ROTS_OK if successful, error code otherwise
 * @note GPIO initialization is handled by ROTS_GPIO_Init() in hardware module
 */
static ROTS_StatusTypeDef ROTS_ActuatorControl_InitGPIO(void)
{
    // GPIO initialization is handled by ROTS_GPIO_Init() in hardware module
    // This function is kept for compatibility but does nothing
    return ROTS_OK;
}

/**
 * @brief Set pump speed
 * @param pump_id Pump ID (0-4)
 * @param speed Speed (0-100%)
 */
static void ROTS_ActuatorControl_SetPumpSpeed(uint8_t pump_id, uint8_t speed)
{
    if (pump_id >= ROTS_MAX_PUMPS) return;
    
    pump_speeds[pump_id] = speed;
    
    // Use hardware interface to set pump speed
    ROTS_Hardware_SetPumpSpeed(pump_id, speed);
}

/**
 * @brief Set valve state
 * @param valve_id Valve ID (0-4)
 * @param state Valve state
 */
static void ROTS_ActuatorControl_SetValveState(uint8_t valve_id, ROTS_ActuatorState_t state)
{
    if (valve_id >= ROTS_MAX_VALVES) return;
    
    valve_states[valve_id] = state;
    
    // Use hardware interface to set valve state
    ROTS_Hardware_SetValveState(valve_id, state);
}

/**
 * @brief Set fan speed
 * @param fan_id Fan ID (0-1)
 * @param speed Speed (0-255)
 */
static void ROTS_ActuatorControl_SetFanSpeed(uint8_t fan_id, uint8_t speed)
{
    if (fan_id >= ROTS_MAX_FANS) return;
    
    fan_states[fan_id] = (speed > 0) ? ROTS_ACTUATOR_ON : ROTS_ACTUATOR_OFF;
    
    // Use hardware interface to set fan speed
    // Convert 0-255 range to 0-100% for hardware interface
    uint8_t fan_speed_percent = (speed * 100) / 255;
    ROTS_Hardware_SetFanSpeed(fan_id, fan_speed_percent);
}
