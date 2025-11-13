// ROTS Sensor Manager - Sensor Management Module
#include "rots_sender.h"
#include "rots_sensor_manager.h"
#include "rots_debug.h"
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

// Private variables
static ROTS_SensorData_t current_sensor_data;
static ROTS_SensorData_t sensor_history[10];
static uint8_t history_index = 0;
static bool sensor_initialized = false;

// Sensor calibration parameters
static float sensor_calibration[ROTS_MAX_SENSORS] = {
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};

// Temperature compensation parameter
static float temperature_compensation = 0.02f; // Compensation coefficient per degree

// DHT22 sensor instance
#define DHT22_PIN 21
#define DHT22_TYPE DHT22
static DHT dht(DHT22_PIN, DHT22_TYPE);

// BMP280 sensor instance
static Adafruit_BMP280 bmp;
#define BMP280_I2C_ADDRESS 0x76

// Private function declarations
static float ROTS_SensorManager_ReadMQSensor(uint8_t pin, uint8_t sensor_id);
static void ROTS_SensorManager_ApplyCalibration(ROTS_SensorData_t* data);
static void ROTS_SensorManager_ApplyTemperatureCompensation(ROTS_SensorData_t* data);
static void ROTS_SensorManager_UpdateHistory(const ROTS_SensorData_t* data);

// Initialize sensor manager
ROTS_StatusTypeDef ROTS_SensorManager_Init(void) {
    // Configure sensor power pin
    pinMode(ROTS_SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(ROTS_SENSOR_POWER_PIN, HIGH);
    
    // Initialize I2C bus
    Wire.begin(ROTS_SDA_PIN, ROTS_SCL_PIN);
    Wire.setClock(400000); // 400kHz I2C speed
    
    // Initialize DHT22 sensor
    dht.begin();
    
    // Initialize BMP280 sensor
    if (!bmp.begin(BMP280_I2C_ADDRESS)) {
        DEBUG_ERROR("BMP280 sensor not found\r\n");
        return ROTS_SENSOR_ERROR;
    }
    
    // Configure BMP280
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // Operating mode
                    Adafruit_BMP280::SAMPLING_X2,     // Temperature oversampling
                    Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling
                    Adafruit_BMP280::FILTER_X16,      // Filtering
                    Adafruit_BMP280::STANDBY_MS_500); // Standby time
    
    // Wait for sensors to warm up
    DEBUG_INFO("Warming up sensors...\r\n");
    delay(3000);
    
    // Initialize sensor data structures
    memset(&current_sensor_data, 0, sizeof(ROTS_SensorData_t));
    memset(sensor_history, 0, sizeof(sensor_history));
    
    // Perform sensor calibration
    ROTS_StatusTypeDef status = ROTS_SensorManager_CalibrateSensors();
    if (status != ROTS_OK) {
        DEBUG_ERROR("Sensor calibration failed\r\n");
        return status;
    }
    
    sensor_initialized = true;
    DEBUG_INFO("Sensor manager initialized\r\n");
    return ROTS_OK;
}

// Read all sensor data
ROTS_StatusTypeDef ROTS_SensorManager_ReadSensors(ROTS_SensorData_t* data) {
    if (!sensor_initialized || !data) {
        return ROTS_INVALID_PARAM;
    }
    
    // Read MQ gas sensors
    data->mq2_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ2_PIN, 0);
    data->mq3_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ3_PIN, 1);
    data->mq4_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ4_PIN, 2);
    data->mq5_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ5_PIN, 3);
    data->mq6_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ6_PIN, 4);
    data->mq7_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ7_PIN, 5);
    data->mq8_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ8_PIN, 6);
    data->mq9_value = ROTS_SensorManager_ReadMQSensor(ROTS_MQ9_PIN, 7);
    
    // Read environmental sensors
    data->temperature = ROTS_SensorManager_ReadTemperature();
    data->humidity = ROTS_SensorManager_ReadHumidity();
    data->pressure = ROTS_SensorManager_ReadPressure();
    
    // Set timestamp
    data->timestamp = millis();
    
    // Apply calibration and compensation
    ROTS_SensorManager_ApplyCalibration(data);
    ROTS_SensorManager_ApplyTemperatureCompensation(data);
    
    // Update history buffer
    ROTS_SensorManager_UpdateHistory(data);
    
    return ROTS_OK;
}

// Update sensor data
void ROTS_SensorManager_UpdateData(const ROTS_SensorData_t* data) {
    if (!data) return;
    
    memcpy(&current_sensor_data, data, sizeof(ROTS_SensorData_t));
}

// Get current sensor data
ROTS_StatusTypeDef ROTS_SensorManager_GetCurrentData(ROTS_SensorData_t* data) {
    if (!sensor_initialized || !data) {
        return ROTS_INVALID_PARAM;
    }
    
    memcpy(data, &current_sensor_data, sizeof(ROTS_SensorData_t));
    return ROTS_OK;
}

// Get sensor history data
ROTS_StatusTypeDef ROTS_SensorManager_GetHistoryData(ROTS_SensorData_t* data, uint8_t count) {
    if (!sensor_initialized || !data || count > 10) {
        return ROTS_INVALID_PARAM;
    }
    
    uint8_t start_index = (history_index - count + 10) % 10;
    for (uint8_t i = 0; i < count; i++) {
        memcpy(&data[i], &sensor_history[(start_index + i) % 10], sizeof(ROTS_SensorData_t));
    }
    
    return ROTS_OK;
}

// Calibrate sensors in clean air
ROTS_StatusTypeDef ROTS_SensorManager_CalibrateSensors(void) {
    DEBUG_INFO("Starting sensor calibration...\r\n");
    
    // Calibrate in clean air (baseline reading)
    float calibration_values[ROTS_MAX_SENSORS];
    const int calibration_samples = 100;
    
    uint8_t mq_pins[ROTS_MAX_SENSORS] = {
        ROTS_MQ2_PIN, ROTS_MQ3_PIN, ROTS_MQ4_PIN, ROTS_MQ5_PIN,
        ROTS_MQ6_PIN, ROTS_MQ7_PIN, ROTS_MQ8_PIN, ROTS_MQ9_PIN
    };
    
    for (int sensor = 0; sensor < ROTS_MAX_SENSORS; sensor++) {
        float sum = 0;
        for (int i = 0; i < calibration_samples; i++) {
            sum += analogRead(mq_pins[sensor]);
            delay(10);
        }
        calibration_values[sensor] = sum / calibration_samples;
        // Calculate calibration factor to normalize to full scale
        sensor_calibration[sensor] = 4095.0f / calibration_values[sensor];
    }
    
    DEBUG_INFO("Sensor calibration completed\r\n");
    return ROTS_OK;
}

// Read MQ gas sensor
static float ROTS_SensorManager_ReadMQSensor(uint8_t pin, uint8_t sensor_id) {
    // Read analog value (12-bit ADC, 0-4095)
    int raw_value = analogRead(pin);
    
    // Convert to voltage (0-3.3V)
    float voltage = (raw_value * 3.3f) / 4095.0f;
    
    // Apply calibration
    float calibrated_value = voltage * sensor_calibration[sensor_id];
    
    // Convert to resistance value (assuming load resistor RL=10kΩ)
    // Vout = Vcc * RL / (RL + Rsensor)
    // Rsensor = RL * (Vcc - Vout) / Vout
    float resistance = (3.3f - calibrated_value) * 10000.0f / calibrated_value;
    
    // Prevent division by zero
    if (resistance < 1.0f) resistance = 1.0f;
    
    // Convert resistance to gas concentration using sensor-specific formula
    // This is a simplified calculation - actual formulas vary by sensor type
    // For MQ sensors: Rs/R0 = a * (concentration)^b
    // concentration = 10^((log10(Rs/R0) - log10(a)) / b)
    float ratio = resistance / 10000.0f; // Assume R0 = 10kΩ in clean air
    float concentration = pow(10, (log10(ratio) - 0.477f) / -0.8f);
    
    // Limit concentration range (ppm)
    if (concentration > 1000.0f) concentration = 1000.0f;
    if (concentration < 0.1f) concentration = 0.1f;
    
    return concentration;
}

// Read temperature from DHT22 sensor
float ROTS_SensorManager_ReadTemperature(void) {
    float temperature = dht.readTemperature();
    
    // Check if reading is valid
    if (isnan(temperature)) {
        DEBUG_ERROR("Failed to read temperature from DHT22\r\n");
        // Return last known value if available
        return current_sensor_data.temperature > 0 ? current_sensor_data.temperature : 25.0f;
    }
    
    return temperature;
}

// Read humidity from DHT22 sensor
float ROTS_SensorManager_ReadHumidity(void) {
    float humidity = dht.readHumidity();
    
    // Check if reading is valid
    if (isnan(humidity)) {
        DEBUG_ERROR("Failed to read humidity from DHT22\r\n");
        // Return last known value if available
        return current_sensor_data.humidity > 0 ? current_sensor_data.humidity : 50.0f;
    }
    
    return humidity;
}

// Read pressure from BMP280 sensor
float ROTS_SensorManager_ReadPressure(void) {
    float pressure = bmp.readPressure() / 100.0f; // Convert Pa to hPa
    
    // Check if reading is valid
    if (isnan(pressure) || pressure <= 0) {
        DEBUG_ERROR("Failed to read pressure from BMP280\r\n");
        // Return last known value if available
        return current_sensor_data.pressure > 0 ? current_sensor_data.pressure : 1013.25f;
    }
    
    return pressure;
}

// Apply calibration factors to sensor data
static void ROTS_SensorManager_ApplyCalibration(ROTS_SensorData_t* data) {
    data->mq2_value *= sensor_calibration[0];
    data->mq3_value *= sensor_calibration[1];
    data->mq4_value *= sensor_calibration[2];
    data->mq5_value *= sensor_calibration[3];
    data->mq6_value *= sensor_calibration[4];
    data->mq7_value *= sensor_calibration[5];
    data->mq8_value *= sensor_calibration[6];
    data->mq9_value *= sensor_calibration[7];
}

// Apply temperature compensation to gas sensor readings
static void ROTS_SensorManager_ApplyTemperatureCompensation(ROTS_SensorData_t* data) {
    // MQ sensors are sensitive to temperature changes
    // Compensation factor based on deviation from 25°C reference
    float temp_factor = 1.0f + (data->temperature - 25.0f) * temperature_compensation;
    
    data->mq2_value *= temp_factor;
    data->mq3_value *= temp_factor;
    data->mq4_value *= temp_factor;
    data->mq5_value *= temp_factor;
    data->mq6_value *= temp_factor;
    data->mq7_value *= temp_factor;
    data->mq8_value *= temp_factor;
    data->mq9_value *= temp_factor;
}

// Update sensor history buffer
static void ROTS_SensorManager_UpdateHistory(const ROTS_SensorData_t* data) {
    memcpy(&sensor_history[history_index], data, sizeof(ROTS_SensorData_t));
    history_index = (history_index + 1) % 10;
}

// Get sensor status information
ROTS_StatusTypeDef ROTS_SensorManager_GetStatus(ROTS_SensorStatus_t* status) {
    if (!sensor_initialized || !status) {
        return ROTS_INVALID_PARAM;
    }
    
    status->initialized = sensor_initialized;
    status->last_read_time = current_sensor_data.timestamp;
    status->temperature = current_sensor_data.temperature;
    status->humidity = current_sensor_data.humidity;
    status->pressure = current_sensor_data.pressure;
    
    // Calculate sensor health status
    // Check if sensors are returning valid readings
    uint32_t time_since_last_read = millis() - current_sensor_data.timestamp;
    if (time_since_last_read > 5000) {
        status->sensor_health = 0; // No recent readings
    } else if (status->temperature < -40.0f || status->temperature > 80.0f ||
               status->humidity < 0.0f || status->humidity > 100.0f ||
               status->pressure < 300.0f || status->pressure > 1100.0f) {
        status->sensor_health = 50; // Out of range readings
    } else {
        status->sensor_health = 100; // All sensors healthy
    }
    
    return ROTS_OK;
}
