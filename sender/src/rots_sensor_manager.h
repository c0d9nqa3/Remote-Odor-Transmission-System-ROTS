// ROTS Sensor Manager Header
#ifndef ROTS_SENSOR_MANAGER_H
#define ROTS_SENSOR_MANAGER_H

#include "rots_sender.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sensor status structure
typedef struct {
    bool initialized;
    uint32_t last_read_time;
    float temperature;
    float humidity;
    float pressure;
    uint8_t sensor_health; // 0-100%
} ROTS_SensorStatus_t;

// Function declarations
ROTS_StatusTypeDef ROTS_SensorManager_Init(void);
ROTS_StatusTypeDef ROTS_SensorManager_ReadSensors(ROTS_SensorData_t* data);
void ROTS_SensorManager_UpdateData(const ROTS_SensorData_t* data);
ROTS_StatusTypeDef ROTS_SensorManager_GetCurrentData(ROTS_SensorData_t* data);
ROTS_StatusTypeDef ROTS_SensorManager_GetHistoryData(ROTS_SensorData_t* data, uint8_t count);
ROTS_StatusTypeDef ROTS_SensorManager_CalibrateSensors(void);
ROTS_StatusTypeDef ROTS_SensorManager_GetStatus(ROTS_SensorStatus_t* status);

// Sensor reading functions
float ROTS_SensorManager_ReadTemperature(void);
float ROTS_SensorManager_ReadHumidity(void);
float ROTS_SensorManager_ReadPressure(void);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_SENSOR_MANAGER_H */
