/**
 * @file rots_hardware.h
 * @brief ROTS Hardware Driver Header
 * @author ROTS Team
 * @date 2024
 * 
 * Header file for hardware driver functionality
 */

#ifndef ROTS_HARDWARE_H
#define ROTS_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rots_receiver.h"

/* Function declarations */
ROTS_StatusTypeDef ROTS_SystemClock_Init(void);
ROTS_StatusTypeDef ROTS_GPIO_Init(void);
ROTS_StatusTypeDef ROTS_PWM_Init(void);
ROTS_StatusTypeDef ROTS_UART_Init(void);
ROTS_StatusTypeDef ROTS_I2C_Init(void);
ROTS_StatusTypeDef ROTS_ADC_Init(void);
ROTS_StatusTypeDef ROTS_Hardware_SelfTest(void);

/* Hardware control functions */
void ROTS_Hardware_SetPumpSpeed(uint8_t pump_id, uint8_t speed);
void ROTS_Hardware_SetValveState(uint8_t valve_id, ROTS_ActuatorState_t state);
void ROTS_Hardware_SetFanSpeed(uint8_t fan_id, uint8_t speed);

/* Sensor reading functions */
uint16_t ROTS_Hardware_ReadADC(uint8_t channel);
float ROTS_Hardware_ReadTemperature(void);
float ROTS_Hardware_ReadHumidity(void);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_HARDWARE_H */
