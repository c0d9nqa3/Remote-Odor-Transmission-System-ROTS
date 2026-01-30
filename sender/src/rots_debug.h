// ROTS Debug Module Header
#ifndef ROTS_DEBUG_H
#define ROTS_DEBUG_H

#include "rots_sender.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
ROTS_StatusTypeDef ROTS_Debug_Init(void);
void ROTS_Debug_SetLevel(ROTS_DebugLevel_t level);
void ROTS_Debug_Print(ROTS_DebugLevel_t level, const char* format, ...);
void ROTS_Debug_PrintHex(ROTS_DebugLevel_t level, const char* label, const uint8_t* data, uint16_t length);
void ROTS_Debug_PrintSystemStatus(void);
void ROTS_Debug_PrintSensorStatus(void);
void ROTS_Debug_PrintAIStatus(void);
void ROTS_Debug_PrintCommStatus(void);
void ROTS_Debug_PrintMemoryUsage(void);
void ROTS_Debug_PrintError(ROTS_StatusTypeDef error_code);
void ROTS_Debug_BlinkLED(uint8_t pin, uint8_t times, uint16_t delay_ms);
void ROTS_Debug_ErrorLED(bool state);
void ROTS_Debug_StatusLED(bool state);

// Debug macro definitions
#define DEBUG_ERROR(fmt, ...)   ROTS_Debug_Print(ROTS_DEBUG_ERROR, fmt, ##__VA_ARGS__)
#define DEBUG_WARNING(fmt, ...) ROTS_Debug_Print(ROTS_DEBUG_WARNING, fmt, ##__VA_ARGS__)
#define DEBUG_INFO(fmt, ...)    ROTS_Debug_Print(ROTS_DEBUG_INFO, fmt, ##__VA_ARGS__)
#define DEBUG_DEBUG(fmt, ...)   ROTS_Debug_Print(ROTS_DEBUG_DEBUG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* ROTS_DEBUG_H */
