# ROTS Sender - ESP32 Sender End

ESP32-based remote odor recognition sender, using edge AI for real-time odor detection.

## Hardware Requirements

- ESP32 development board (ESP32-WROOM-32E recommended)
- 8x MQ-series gas sensors (MQ-2 to MQ-9)
- DHT22 temperature and humidity sensor
- BMP280 pressure sensor
- ADS1115 ADC converter (optional, ESP32 has built-in ADC)
- Power management module

## Project Structure

```
sender/
├── src/                    # Source code
│   ├── main.cpp           # Main program
│   ├── rots_sender.h      # Main header file
│   ├── rots_sensor_manager.cpp/h    # Sensor management
│   ├── rots_ai_engine.cpp/h         # AI inference engine
│   ├── rots_communication.cpp/h     # Communication module
│   ├── rots_debug.cpp/h             # Debug module
│   └── rots_system_monitor.cpp/h    # System monitoring
├── lib/                   # Library files
├── models/                # AI model files
├── platformio.ini         # PlatformIO configuration
└── README.md              # Documentation
```

## Features

- **Multi-sensor fusion**: 8-channel MQ sensors + environmental sensors
- **Edge AI inference**: Real-time odor recognition and classification
- **WiFi communication**: Connect to cloud server
- **MQTT protocol**: Real-time data transmission
- **Debug support**: Serial debugging and LED indicators
- **System monitoring**: Memory, network, and error monitoring

## Quick Start

### 1. Install PlatformIO

```bash
# Install PlatformIO Core
pip install platformio

# Or use VS Code extension
# Search and install PlatformIO IDE
```

### 2. Clone the project

```bash
git clone https://github.com/yourusername/ROTS.git
cd ROTS/sender
```

### 3. Configure network

Edit network configuration in `src/rots_sender.h`:

```cpp
#define ROTS_WIFI_SSID            "your_wifi_ssid"
#define ROTS_WIFI_PASSWORD        "your_wifi_password"
#define ROTS_MQTT_BROKER_HOST     "your_mqtt_broker"
```

### 4. Build and upload

```bash
# Build project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Hardware Connections

### ESP32 Pin Assignment

#### Analog Input (MQ sensors)
- MQ2: GPIO36 (ADC1_CH0)
- MQ3: GPIO39 (ADC1_CH3)
- MQ4: GPIO34 (ADC1_CH6)
- MQ5: GPIO35 (ADC1_CH7)
- MQ6: GPIO32 (ADC1_CH4)
- MQ7: GPIO33 (ADC1_CH5)
- MQ8: GPIO25 (ADC1_CH8)
- MQ9: GPIO26 (ADC1_CH9)

#### I2C Interface (Environmental sensors)
- SDA: GPIO21
- SCL: GPIO22

#### Status Indicators
- Error LED: GPIO2
- Status LED: GPIO4
- Sensor Power: GPIO5

### Sensor Connections

#### MQ Sensors
```
VCC → 5V
GND → GND
A0 → ESP32 GPIO36-39, 32-35, 25-26
```

#### DHT22
```
VCC → 3.3V
GND → GND
DATA → GPIO21 (SDA)
```

#### BMP280
```
VCC → 3.3V
GND → GND
SDA → GPIO21
SCL → GPIO22
```

## Configuration

### 1. Sensor Calibration

The system automatically calibrates sensors on startup:

```cpp
// Calibrate in clean air
ROTS_SensorManager_CalibrateSensors();
```

### 2. AI Model Configuration

```cpp
// Set confidence threshold
#define ROTS_AI_CONFIDENCE_THRESHOLD  0.7f

// Set inference interval
#define ROTS_AI_INFERENCE_INTERVAL    500    // ms
```

### 3. Communication Configuration

```cpp
// WiFi configuration
#define ROTS_WIFI_SSID            "ROTS_Network"
#define ROTS_WIFI_PASSWORD        "rots_password_2024"

// MQTT configuration
#define ROTS_MQTT_BROKER_HOST     "mqtt.rots-system.com"
#define ROTS_MQTT_BROKER_PORT     1883
```

## Debug Guide

### 1. Serial Debugging

- Baud rate: 115200
- Data bits: 8, Stop bits: 1, Parity: None

### 2. Debug Levels

```cpp
ROTS_DEBUG_ERROR   = 0  // Error messages
ROTS_DEBUG_WARNING = 1  // Warning messages
ROTS_DEBUG_INFO    = 2  // General information
ROTS_DEBUG_DEBUG   = 3  // Debug information
```

### 3. Debug Commands

```cpp
// Set debug level
ROTS_Debug_SetLevel(ROTS_DEBUG_INFO);

// Print debug information
DEBUG_INFO("System started\r\n");
DEBUG_ERROR("Error occurred: %d\r\n", error_code);
```

### 4. LED Indicators

- **Error LED**: Error indicator, blinking indicates error
- **Status LED**: Status indicator, solid indicates WiFi connection

## Performance Optimization

### 1. Memory Optimization

```cpp
// Check memory usage
DEBUG_INFO("Free Heap: %lu bytes\r\n", ESP.getFreeHeap());
DEBUG_INFO("Free PSRAM: %lu bytes\r\n", ESP.getFreePsram());
```

### 2. Power Optimization

```cpp
// Set CPU frequency
setCpuFrequencyMhz(80);  // Reduce power consumption

// Enable deep sleep
esp_deep_sleep_start();
```

### 3. Network Optimization

```cpp
// Set WiFi power
WiFi.setTxPower(WIFI_POWER_11dBm);

// Set MQTT keep-alive
mqtt_client.setKeepAlive(60);
```

## Troubleshooting

### 1. Compilation Errors

- Check PlatformIO configuration
- Check library dependencies
- Check Arduino framework version

### 2. Upload Failures

- Check USB connection
- Check port settings
- Check ESP32 enters download mode

### 3. Runtime Errors

- Check serial output
- Check hardware connections
- Check network configuration

### 4. Sensor Issues

- Check power supply
- Check analog input
- Check calibration parameters

## API Reference

### Sensor Management

```cpp
// Read sensor data
ROTS_StatusTypeDef ROTS_SensorManager_ReadSensors(ROTS_SensorData_t* data);

// Get sensor status
ROTS_StatusTypeDef ROTS_SensorManager_GetStatus(ROTS_SensorStatus_t* status);
```

### AI Engine

```cpp
// Process odor detection
ROTS_StatusTypeDef ROTS_AIEngine_ProcessOdor(ROTS_OdorResult_t* result);

// Get AI status
ROTS_StatusTypeDef ROTS_AIEngine_GetStatus(ROTS_AIStatus_t* status);
```

### Communication Module

```cpp
// Send detection result
ROTS_StatusTypeDef ROTS_Communication_SendOdorDetection(const ROTS_OdorResult_t* result);

// Send status information
ROTS_StatusTypeDef ROTS_Communication_SendStatus(const ROTS_SenderStatus_t* status);
```

## License

This project is licensed under the MIT License - see the main project README for details.
