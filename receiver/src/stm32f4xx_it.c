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
#include "rots_communication.h"

/* External variables */
extern TIM_HandleTypeDef htim_timer;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart_esp8266;

/**
 * @brief This function handles Non maskable interrupt
 */
void NMI_Handler(void)
{
    /* User can add error handling code here */
}

/**
 * @brief This function handles Hard fault interrupt
 */
void HardFault_Handler(void)
{
    while (1)
    {
        /* User can add error handling code here */
    }
}

/**
 * @brief This function handles Memory management fault
 */
void MemManage_Handler(void)
{
    while (1)
    {
        /* User can add error handling code here */
    }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault
 */
void BusFault_Handler(void)
{
    while (1)
    {
        /* User can add error handling code here */
    }
}

/**
 * @brief This function handles Undefined instruction or illegal state
 */
void UsageFault_Handler(void)
{
    while (1)
    {
        /* User can add error handling code here */
    }
}

/**
 * @brief This function handles System service call via SWI instruction
 */
void SVC_Handler(void)
{
    /* User can add code here */
}

/**
 * @brief This function handles Debug monitor
 */
void DebugMon_Handler(void)
{
    /* User can add code here */
}

/**
 * @brief This function handles Pendable request for system service
 */
void PendSV_Handler(void)
{
    /* User can add code here */
}

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
    HAL_UART_IRQHandler(&huart1);
}

/**
 * @brief This function handles USART2 global interrupt
 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart_esp8266);
}

/**
 * @brief This function handles SysTick interrupt
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

