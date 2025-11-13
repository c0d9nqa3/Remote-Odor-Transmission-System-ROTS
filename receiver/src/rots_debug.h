/**
 * @file rots_debug.h
 * @brief ROTS Debug Module Header
 * @author ROTS Team
 * @date 2024
 * 
 * Header file for debug and logging functionality
 */

#ifndef ROTS_DEBUG_H
#define ROTS_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rots_receiver.h"

/* Debug level enumeration */
typedef enum {
    ROTS_DEBUG_ERROR = 0,
    ROTS_DEBUG_WARNING = 1,
    ROTS_DEBUG_INFO = 2,
    ROTS_DEBUG_DEBUG = 3
} ROTS_DebugLevel_t;

/* Function prototypes */
ROTS_StatusTypeDef ROTS_Debug_Init(void);
void ROTS_Debug_SetLevel(ROTS_DebugLevel_t level);
void ROTS_Debug_Print(ROTS_DebugLevel_t level, const char* format, ...);
void ROTS_Debug_PrintHex(ROTS_DebugLevel_t level, const char* label, const uint8_t* data, uint16_t length);
void ROTS_Debug_PrintSystemStatus(void);
void ROTS_Debug_PrintMessage(const ROTS_MessageTypeDef* message);
void ROTS_Debug_PrintError(ROTS_StatusTypeDef error_code);
void ROTS_Debug_PrintWiFiStatus(void);
void ROTS_Debug_PrintMQTTStatus(void);
void ROTS_Debug_PrintMemoryUsage(void);

/* Debug macros */
#define DEBUG_ERROR(fmt, ...)   ROTS_Debug_Print(ROTS_DEBUG_ERROR, fmt, ##__VA_ARGS__)
#define DEBUG_WARNING(fmt, ...) ROTS_Debug_Print(ROTS_DEBUG_WARNING, fmt, ##__VA_ARGS__)
#define DEBUG_INFO(fmt, ...)    ROTS_Debug_Print(ROTS_DEBUG_INFO, fmt, ##__VA_ARGS__)
#define DEBUG_DEBUG(fmt, ...)   ROTS_Debug_Print(ROTS_DEBUG_DEBUG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* ROTS_DEBUG_H */
