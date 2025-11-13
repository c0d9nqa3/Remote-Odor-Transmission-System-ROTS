// ROTS Sender Main Application - ESP32
#include "rots_sender.h"
#include "rots_sensor_manager.h"
#include "rots_ai_engine.h"
#include "rots_communication.h"
#include "rots_system_monitor.h"
#include "rots_debug.h"

// Global variables
ROTS_SenderStatus_t sender_status;
bool system_initialized = false;

// Function declarations
void setup();
void loop();
ROTS_StatusTypeDef ROTS_Sender_Init(void);
void ROTS_Sender_MainLoop(void);
void ROTS_Sender_ErrorHandler(ROTS_StatusTypeDef error_code);

void setup() {
    // Initialize serial port for debugging
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_INFO("ROTS Sender Starting...\r\n");
    
    // Initialize system
    ROTS_StatusTypeDef status = ROTS_Sender_Init();
    if (status != ROTS_OK) {
        ROTS_Sender_ErrorHandler(status);
        return;
    }
    
    system_initialized = true;
    DEBUG_INFO("System initialization completed\r\n");
}

void loop() {
    if (!system_initialized) {
        delay(1000);
        return;
    }
    
    ROTS_Sender_MainLoop();
    delay(10);
}

ROTS_StatusTypeDef ROTS_Sender_Init(void) {
    ROTS_StatusTypeDef status = ROTS_OK;
    
    // Initialize debug system
    status = ROTS_Debug_Init();
    if (status != ROTS_OK) return status;
    
    // Initialize sensor manager
    status = ROTS_SensorManager_Init();
    if (status != ROTS_OK) {
        DEBUG_ERROR("Sensor manager init failed\r\n");
        return status;
    }
    
    // Initialize AI engine
    status = ROTS_AIEngine_Init();
    if (status != ROTS_OK) {
        DEBUG_ERROR("AI engine init failed\r\n");
        return status;
    }
    
    // Initialize communication module
    status = ROTS_Communication_Init();
    if (status != ROTS_OK) {
        DEBUG_ERROR("Communication init failed\r\n");
        return status;
    }
    
    // Initialize system monitor
    status = ROTS_SystemMonitor_Init();
    if (status != ROTS_OK) {
        DEBUG_ERROR("System monitor init failed\r\n");
        return status;
    }
    
    // Initialize sender status
    sender_status.state = ROTS_SENDER_IDLE;
    sender_status.last_detection_time = 0;
    sender_status.detection_count = 0;
    sender_status.error_count = 0;
    sender_status.wifi_connected = false;
    sender_status.mqtt_connected = false;
    sender_status.battery_voltage = 0.0f;
    
    return ROTS_OK;
}

void ROTS_Sender_MainLoop(void) {
    static uint32_t last_sensor_read = 0;
    static uint32_t last_ai_inference = 0;
    static uint32_t last_status_update = 0;
    static uint32_t last_debug_output = 0;
    static uint32_t last_status_send = 0;
    
    uint32_t current_time = millis();
    
    // Update communication module
    ROTS_Communication_Update();
    
    // Read sensor data (every 100ms)
    if (current_time - last_sensor_read >= 100) {
        ROTS_SensorData_t sensor_data;
        ROTS_StatusTypeDef status = ROTS_SensorManager_ReadSensors(&sensor_data);
        
        if (status == ROTS_OK) {
            // Update sensor data
            ROTS_SensorManager_UpdateData(&sensor_data);
            last_sensor_read = current_time;
        } else {
            DEBUG_ERROR("Sensor read failed: %d\r\n", status);
        }
    }
    
    // AI inference (every 500ms)
    if (current_time - last_ai_inference >= 500) {
        ROTS_OdorResult_t ai_result;
        ROTS_StatusTypeDef status = ROTS_AIEngine_ProcessOdor(&ai_result);
        
        if (status == ROTS_OK && ai_result.confidence > ROTS_AI_CONFIDENCE_THRESHOLD) {
            DEBUG_INFO("Odor detected: %s (confidence: %.2f)\r\n", 
                      ai_result.odor_name, ai_result.confidence);
            
            // Send detection result to cloud server
            ROTS_Communication_SendOdorDetection(&ai_result);
            
            // Update sender status
            sender_status.state = ROTS_SENDER_DETECTING;
            sender_status.last_detection_time = current_time;
            sender_status.detection_count++;
        } else if (current_time - sender_status.last_detection_time > 5000) {
            // No detection for 5 seconds, return to idle state
            sender_status.state = ROTS_SENDER_IDLE;
        }
        
        last_ai_inference = current_time;
    }
    
    // Update system status (every 1 second)
    if (current_time - last_status_update >= 1000) {
        ROTS_SystemMonitor_Update();
        last_status_update = current_time;
    }
    
    // Send status information (every 5 seconds)
    if (current_time - last_status_send >= 5000) {
        // Update sender status
        ROTS_SystemStatus_t system_status;
        ROTS_SystemMonitor_GetStatus(&system_status);
        sender_status.wifi_connected = system_status.wifi_connected;
        sender_status.battery_voltage = system_status.battery_voltage;
        
        // Send status to cloud server
        ROTS_Communication_SendStatus(&sender_status);
        last_status_send = current_time;
    }
    
    // Debug output (every 10 seconds)
    if (current_time - last_debug_output >= 10000) {
        ROTS_Debug_PrintSystemStatus();
        ROTS_Debug_PrintSensorStatus();
        ROTS_Debug_PrintAIStatus();
        ROTS_Debug_PrintMemoryUsage();
        last_debug_output = current_time;
    }
}

void ROTS_Sender_ErrorHandler(ROTS_StatusTypeDef error_code) {
    DEBUG_ERROR("System error: %d\r\n", error_code);
    
    // Display error LED
    digitalWrite(ROTS_ERROR_LED_PIN, HIGH);
    
    // Log error
    sender_status.error_count++;
    ROTS_SystemMonitor_LogError(error_code);
    
    // Send error to cloud server
    ROTS_Communication_SendError(error_code);
    
    // Attempt recovery after delay
    delay(1000);
    digitalWrite(ROTS_ERROR_LED_PIN, LOW);
}
