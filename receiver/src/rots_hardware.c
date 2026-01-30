/**
 * @file rots_hardware.c
 * @brief ROTS Hardware Driver
 * @author ROTS Team
 * @date 2024
 * 
 * Hardware driver for STM32F407VET6
 */

#include "rots_receiver.h"
#include "rots_hardware.h"
#include "rots_debug.h"

/* ADC handle for sensor readings */
static ADC_HandleTypeDef hadc1;

/**
 * @brief Initialize system clock
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SystemClock_Init(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* Configure HSE oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    /* Configure system clock */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    DEBUG_INFO("System clock initialized: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
    return ROTS_OK;
}

/**
 * @brief Initialize GPIO
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    /* Configure status LED pins */
    GPIO_InitStruct.Pin = ERROR_LED_PIN | STATUS_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ERROR_LED_PORT, &GPIO_InitStruct);
    
    /* Configure pump enable pins */
    GPIO_InitStruct.Pin = PUMP1_EN_PIN | PUMP2_EN_PIN;
    HAL_GPIO_Init(PUMP1_EN_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = PUMP3_EN_PIN | PUMP4_EN_PIN | PUMP5_EN_PIN;
    HAL_GPIO_Init(PUMP3_EN_PORT, &GPIO_InitStruct);
    
    /* Configure valve control pins */
    GPIO_InitStruct.Pin = VALVE1_PIN | VALVE2_PIN | VALVE3_PIN | VALVE4_PIN | VALVE5_PIN;
    HAL_GPIO_Init(VALVE1_PORT, &GPIO_InitStruct);
    
    /* Configure fan control pins */
    GPIO_InitStruct.Pin = FAN1_PIN | FAN2_PIN;
    HAL_GPIO_Init(FAN1_PORT, &GPIO_InitStruct);
    
    /* Configure ESP8266 control pin */
    GPIO_InitStruct.Pin = GPIO_PIN_4;  /* ESP8266 Reset pin */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    
    DEBUG_INFO("GPIO initialized\r\n");
    return ROTS_OK;
}

/* Global PWM timer handles */
static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim3;

/**
 * @brief Initialize PWM
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_PWM_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* TIM2 PWM: PA0/PA1 (CH1/2), PB10/PB11 (CH3/4). PA2/PA3 reserved for USART2/ESP8266. */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* Configure TIM2 for pumps 1-4 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 84 - 1;  /* 1MHz timer clock (84MHz / 84 = 1MHz) */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000 - 1;   /* 1kHz PWM frequency (1MHz / 1000 = 1kHz) */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    /* Configure GPIO pins for TIM3 PWM (PB0, PB1) */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* Configure TIM3 for pump 5 */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 84 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    /* Configure PWM channels */
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    /* Configure TIM2 channels for pumps 1-4 */
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    /* Configure TIM3 channel for pump 5 */
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    /* Start PWM outputs */
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4) != HAL_OK) {
        return ROTS_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    DEBUG_INFO("PWM initialized\r\n");
    return ROTS_OK;
}

/* Global UART handles */
UART_HandleTypeDef huart1;  /* Debug UART (USART1, exported for interrupt handler) */
/* USART2 / ESP8266 UART is owned and initialized by rots_communication.c (huart_esp8266) */

/**
 * @brief Initialize UART
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Configure USART1 for debug (PA9=TX, PA10=RX). USART2/ESP8266 is owned by rots_communication. */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    DEBUG_INFO("UART initialized\r\n");
    return ROTS_OK;
}

/* Global I2C handle */
static I2C_HandleTypeDef hi2c1;

/**
 * @brief Initialize I2C
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable I2C1 clock */
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* Configure I2C1 pins (PB6=SCL, PB7=SDA) */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* Configure I2C1 for OLED display */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    DEBUG_INFO("I2C initialized\r\n");
    return ROTS_OK;
}

// Hardware self-test
ROTS_StatusTypeDef ROTS_Hardware_SelfTest(void)
{
    DEBUG_INFO("Starting hardware self-test...\r\n");
    
    // Test status LEDs
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_RESET);
    
    // Test pump enable pins
    HAL_GPIO_WritePin(PUMP1_EN_PORT, PUMP1_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(PUMP1_EN_PORT, PUMP1_EN_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(PUMP1_EN_PORT, PUMP2_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(PUMP1_EN_PORT, PUMP2_EN_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP3_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP3_EN_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP4_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP4_EN_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP5_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(PUMP3_EN_PORT, PUMP5_EN_PIN, GPIO_PIN_RESET);
    
    // Test valve control pins
    HAL_GPIO_WritePin(VALVE1_PORT, VALVE1_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(VALVE1_PORT, VALVE1_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(VALVE2_PORT, VALVE2_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(VALVE2_PORT, VALVE2_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(VALVE3_PORT, VALVE3_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(VALVE3_PORT, VALVE3_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(VALVE4_PORT, VALVE4_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(VALVE4_PORT, VALVE4_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(VALVE5_PORT, VALVE5_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(VALVE5_PORT, VALVE5_PIN, GPIO_PIN_RESET);
    
    // Test fan control pins
    HAL_GPIO_WritePin(FAN1_PORT, FAN1_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(FAN1_PORT, FAN1_PIN, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(FAN2_PORT, FAN2_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(FAN2_PORT, FAN2_PIN, GPIO_PIN_RESET);
    
    DEBUG_INFO("Hardware self-test completed\r\n");
    return ROTS_OK;
}

// Set pump speed via PWM
void ROTS_Hardware_SetPumpSpeed(uint8_t pump_id, uint8_t speed)
{
    if (pump_id >= ROTS_MAX_PUMPS) return;
    
    // Limit speed range (0-100%)
    if (speed > 100) speed = 100;
    
    // Calculate PWM value (0-1000 for 0-100%)
    uint32_t pwm_value = (speed * 1000) / 100;
    
    // Set PWM duty cycle based on pump ID
    switch (pump_id) {
        case 0:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value);
            break;
        case 1:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwm_value);
            break;
        case 2:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pwm_value);
            break;
        case 3:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, pwm_value);
            break;
        case 4:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_value);
            break;
    }
    
    DEBUG_DEBUG("Pump %d speed set to %d%%\r\n", pump_id, speed);
}

// Set valve state
void ROTS_Hardware_SetValveState(uint8_t valve_id, ROTS_ActuatorState_t state)
{
    if (valve_id >= ROTS_MAX_VALVES) return;
    
    GPIO_PinState pin_state = (state == ROTS_ACTUATOR_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    // Set valve pin based on valve ID
    switch (valve_id) {
        case 0:
            HAL_GPIO_WritePin(VALVE1_PORT, VALVE1_PIN, pin_state);
            break;
        case 1:
            HAL_GPIO_WritePin(VALVE2_PORT, VALVE2_PIN, pin_state);
            break;
        case 2:
            HAL_GPIO_WritePin(VALVE3_PORT, VALVE3_PIN, pin_state);
            break;
        case 3:
            HAL_GPIO_WritePin(VALVE4_PORT, VALVE4_PIN, pin_state);
            break;
        case 4:
            HAL_GPIO_WritePin(VALVE5_PORT, VALVE5_PIN, pin_state);
            break;
    }
    
    DEBUG_DEBUG("Valve %d set to %s\r\n", valve_id, (state == ROTS_ACTUATOR_ON) ? "ON" : "OFF");
}

// Set fan speed
void ROTS_Hardware_SetFanSpeed(uint8_t fan_id, uint8_t speed)
{
    if (fan_id >= ROTS_MAX_FANS) return;
    
    GPIO_PinState pin_state = (speed > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    // Set fan pin based on fan ID
    if (fan_id == 0) {
        HAL_GPIO_WritePin(FAN1_PORT, FAN1_PIN, pin_state);
    } else {
        HAL_GPIO_WritePin(FAN2_PORT, FAN2_PIN, pin_state);
    }
    
    DEBUG_DEBUG("Fan %d speed set to %d%%\r\n", fan_id, speed);
}

/**
 * @brief Initialize ADC for sensor readings
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_ADC_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Enable ADC1 clock
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure ADC channel pin (PA0 for ADC1_IN0)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure ADC1
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    // Configure ADC channel
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return ROTS_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Read ADC value
 * @param channel ADC channel
 * @return ADC value (0-4095)
 */
uint16_t ROTS_Hardware_ReadADC(uint8_t channel)
{
    uint16_t adc_value = 0;
    ADC_ChannelConfTypeDef sConfig = {0};
    
    /* Configure ADC channel */
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0;
    }
    
    /* Start ADC conversion */
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        /* Wait for conversion to complete (timeout 100ms) */
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            /* Read converted value */
            adc_value = HAL_ADC_GetValue(&hadc1);
        }
        /* Stop ADC */
        HAL_ADC_Stop(&hadc1);
    }
    
    return adc_value;
}

/**
 * @brief Read temperature from internal temperature sensor
 * @return Temperature in Celsius
 */
float ROTS_Hardware_ReadTemperature(void)
{
    /* Use internal temperature sensor on STM32F4 */
    /* Temperature sensor is connected to ADC1_IN16 (channel 16) */
    /* Temperature = (VSENSE - V25) / Avg_Slope + 25 */
    /* V25 = 0.76V typical at 25°C, Avg_Slope = 2.5 mV/°C typical */
    
    /* Enable temperature sensor and Vrefint */
    ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};
    ADC_CommonInitStruct.ADC_Mode = ADC_MODE_INDEPENDENT;
    ADC_CommonInitStruct.ADC_Prescaler = ADC_PRESCALER_DIV2;
    ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    HAL_ADCEx_Calibration_Start(&hadc1);
    
    /* Configure ADC channel for temperature sensor (channel 16) */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_16;  /* Internal temperature sensor channel */
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 25.0f;  /* Return default on error */
    }
    
    /* Enable temperature sensor */
    ADC->CCR |= ADC_CCR_TSVREFE;  /* Enable temperature sensor and VREFINT */
    HAL_Delay(10);  /* Wait for sensor to stabilize */
    
    /* Read ADC value */
    uint16_t adc_value = ROTS_Hardware_ReadADC(16);
    
    /* Disable temperature sensor to save power */
    ADC->CCR &= ~ADC_CCR_TSVREFE;
    
    /* Convert to voltage (12-bit ADC, reference = VDDA = 3.3V typical) */
    float voltage = (adc_value * 3.3f) / 4095.0f;
    
    /* Convert to temperature using STM32F4 calibration values */
    /* For STM32F407: V25 = 0.76V, Avg_Slope = 2.5mV/°C */
    float temperature = ((voltage - 0.76f) / 0.0025f) + 25.0f;
    
    /* Clamp temperature to reasonable range */
    if (temperature < -40.0f) temperature = -40.0f;
    if (temperature > 125.0f) temperature = 125.0f;
    
    return temperature;
}

/**
 * @brief Read humidity (placeholder - requires external sensor)
 * @return Humidity in percentage
 * @note STM32F4 does not have built-in humidity sensor
 */
float ROTS_Hardware_ReadHumidity(void)
{
    /* STM32F4 does not have built-in humidity sensor */
    /* This would require an external sensor like DHT22 connected via I2C/GPIO */
    /* For now, return a default value */
    /* TODO: Implement DHT22 or similar sensor reading via I2C */
    return 50.0f;  /* Default humidity value */
}
