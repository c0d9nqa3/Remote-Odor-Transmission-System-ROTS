/**
 * @file stm32f4xx_it.c
 * @brief STM32F4 Interrupt Service Routines
 * @author ROTS Team
 * @date 2024
 * 
 * Interrupt handlers for STM32F407VET6
 */

#include "stm32f4xx_hal.h"
#include "rots_receiver.h"
#include "rots_actuator_control.h"

extern TIM_HandleTypeDef htim_timer;

/**
 * @brief This function handles TIM4 global interrupt
 */
void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim_timer);
}

/**
 * @brief This function handles USART1 global interrupt
 */
void USART1_IRQHandler(void)
{
    extern UART_HandleTypeDef huart1;
    HAL_UART_IRQHandler(&huart1);
}

/**
 * @brief This function handles USART2 global interrupt
 */
void USART2_IRQHandler(void)
{
    extern UART_HandleTypeDef huart_esp8266;
    if (&huart_esp8266 != NULL) {
        HAL_UART_IRQHandler(&huart_esp8266);
    }
}

/**
 * @brief This function handles SysTick interrupt
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

