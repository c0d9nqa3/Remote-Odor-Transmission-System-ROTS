/**
 * @file rots_actuator_control.h
 * @brief ROTS Actuator Control Module Header
 * @author ROTS Team
 * @date 2024
 * 
 * Header file for actuator control (pumps, valves, fans)
 */

#ifndef ROTS_ACTUATOR_CONTROL_H
#define ROTS_ACTUATOR_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "rots_receiver.h"

/* Function Prototypes */
ROTS_StatusTypeDef ROTS_ActuatorControl_Init(void);
ROTS_StatusTypeDef ROTS_ActuatorControl_ProcessOdorCommand(ROTS_MessageTypeDef* message);
ROTS_StatusTypeDef ROTS_ActuatorControl_ConfigurePumps(ROTS_MessageTypeDef* message);
ROTS_StatusTypeDef ROTS_ActuatorControl_ConfigureValves(ROTS_MessageTypeDef* message);
ROTS_StatusTypeDef ROTS_ActuatorControl_ConfigureFans(ROTS_MessageTypeDef* message);
ROTS_StatusTypeDef ROTS_ActuatorControl_StartOdorGeneration(uint16_t duration);
ROTS_StatusTypeDef ROTS_ActuatorControl_Update(void);
ROTS_StatusTypeDef ROTS_ActuatorControl_StopOdorGeneration(void);
ROTS_StatusTypeDef ROTS_ActuatorControl_EmergencyStop(void);
ROTS_StatusTypeDef ROTS_ActuatorControl_GetStatus(uint8_t* pump_status, uint8_t* valve_status);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_ACTUATOR_CONTROL_H */
