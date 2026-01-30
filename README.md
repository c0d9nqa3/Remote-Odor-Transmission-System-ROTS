# Remote Odor Transmission System (ROTS)

A distributed IoT system for remote odor recognition and transmission using edge AI and multi-sensor fusion.

## Overview

ROTS implements digital olfaction through sensor arrays, edge AI on ESP32, and precise odor generation on STM32. The sender detects odors and streams results to a cloud server; the receiver consumes commands and drives pumps/valves for scent delivery.

### Key Features

- **Edge AI**: Real-time odor recognition on ESP32 (<500ms)
- **Multi-Sensor Fusion**: 8-channel MQ gas sensors + DHT22/BMP280
- **Remote Link**: MQTT over WiFi between sender, cloud, and receiver
- **Odor Generation**: 5-channel pumps + valves + mixing; recipe storage on SPI Flash
- **Modular Firmware**: Debug, display (SSD1306), system monitor, emergency stop

## System Architecture

```
┌─────────────────┐    WiFi    ┌─────────────┐    WiFi    ┌─────────────────┐
│ Sender (ESP32)  │───────────▶│ Cloud       │◀───────────│ Receiver (STM32)│
│ Sensors + AI    │            │ MQTT+MySQL  │            │ ESP8266+Actuators│
│ MQTT            │◀───────────│ Web API     │───────────▶│ OLED            │
└─────────────────┘            └─────────────┘            └─────────────────┘
```

- **Sender**: Read sensors → AI inference → publish to MQTT  
- **Cloud**: Store data, route commands, serve Web UI  
- **Receiver**: Subscribe MQTT → run recipes → drive pumps/valves → report status  

## Hardware Requirements

### Sender Unit (Odor Detection)

| Component | Specification | Quantity | Notes |
|-----------|--------------|----------|-------|
| **MCU Board** | ESP32-WROOM-32E DevKit | 1 | 4MB Flash, WiFi/BT |
| **Gas Sensors** | MQ-2, MQ-3, MQ-4, MQ-5, MQ-6, MQ-7, MQ-8, MQ-9 | 8 | One of each type |
| **Temperature/Humidity** | DHT22 (AM2302) | 1 | I²C/Digital |
| **Pressure Sensor** | BMP280 | 1 | I²C |
| **ADC** | ADS1115 16-bit (optional) | 1 | ESP32 has built-in ADC |
| **Power Supply** | 5V/2A USB or DC | 1 | |
| **Indicator LEDs** | 3mm/5mm LED (red, green) | 2 | Error & status |

### Receiver Unit (Odor Generation)

| Component | Specification | Quantity | Notes |
|-----------|--------------|----------|-------|
| **MCU Board** | STM32F407VET6 DevKit | 1 | Black/Red PCB board |
| **WiFi Module** | ESP8266 (ESP-01S or NodeMCU) | 1 | UART, 3.3V |
| **Display** | SSD1306 OLED 128×64, I²C | 1 | 0.96 inch |
| **SPI Flash** | W25Q128 (16MB) | 1 | Recipe storage |
| **Pumps** | Micro peristaltic pump 6V/12V | 5 | For liquid/gel scents |
| **Valves** | Solenoid valve 12V | 5 | On/off control |
| **Fans** | DC cooling fan 5V/12V | 2 | Air circulation |
| **Power Supply** | 12V/2A DC adapter | 1 | For pumps/valves |
| **Voltage Regulator** | LM2596 (12V→5V, 3.3V) | 2 | Step-down modules |
| **Programmer** | ST-Link V2 | 1 | For STM32 flashing |

### Cloud Server (Optional for Local Testing)

- **Computer**: Linux/Windows with Node.js installed
- **Database**: MySQL 5.7+ or MariaDB
- **MQTT Broker**: Mosquitto or similar

### Additional Materials

- Breadboards, jumper wires, resistors (220Ω for LEDs)
- 3D-printed mixing chamber (STL in `models/` if available)
- Scent cartridges/containers (5 units, 10–50ml each)
- USB cables (Type-C for ESP32, Micro-USB for STM32)

### Where to Purchase

- **Development Boards**: AliExpress, Digi-Key, Mouser, Amazon
- **Sensors (MQ-series)**: AliExpress, Amazon, eBay (search "MQ-2 gas sensor module")
- **DHT22 & BMP280**: Adafruit, SparkFun, or generic modules on AliExpress
- **Pumps & Valves**: AliExpress, eBay (search "micro peristaltic pump 12V", "solenoid valve 12V")
- **ST-Link V2**: Amazon, AliExpress (~$2–10, official or clone)
- **Power Supplies**: Standard 12V/2A DC adapter, USB chargers

**Estimated Total Cost**: ~$80–150 USD depending on component quality and seller.

## Technical Specifications

| Item | Sender | Receiver |
|------|--------|----------|
| MCU | ESP32-WROOM-32E | STM32F407VET6 |
| Connectivity | WiFi, MQTT | ESP8266 (UART), MQTT |
| Sensors | MQ-2–MQ-9, DHT22, BMP280 | — |
| Actuators | — | 5× pumps, 5× valves, 2× fans |
| Storage | — | 16MB SPI Flash (W25Q128) |
| Display | — | SSD1306 OLED (I²C) |

## Project Structure

```
ROTS/
├── build.sh              # Linux build script (receiver + sender)
├── sender/               # ESP32 firmware (PlatformIO)
│   ├── src/
│   └── models/
├── receiver/             # STM32 firmware (Makefile + HAL)
│   ├── src/
│   ├── config/
│   ├── scripts/
│   │   └── fetch_cube.sh # Fetches STM32CubeF4 into vendor/
│   ├── vendor/           # STM32CubeF4 (git clone, not committed)
│   └── build/            # Build output
└── cloud-server/         # Node.js (Express + MQTT + MySQL)
    ├── app.js
    ├── public/
    └── .env.example
```

## Getting Started

### Prerequisites

**For Receiver (STM32):**
- ARM GCC toolchain: `gcc-arm-none-eabi`, `make`, `git`
- OpenOCD (for flashing via ST-Link)

**For Sender (ESP32):**
- PlatformIO (via pipx recommended on modern Python)

**For Cloud Server:**
- Node.js 14+, npm
- MySQL or MariaDB
- MQTT broker (e.g. Mosquitto)

### Installation (Linux)

#### 1. Install build tools

```bash
# Debian/Ubuntu/WSL
sudo apt-get update
sudo apt-get install -y gcc-arm-none-eabi make git pipx

# Fedora
sudo dnf install -y arm-none-eabi-gcc make git
pip install --user pipx
```

#### 2. Install PlatformIO (for ESP32 sender)

```bash
pipx install platformio
pipx ensurepath
```

**Important**: Close and reopen your terminal after `pipx ensurepath` for PATH to update. Then verify:

```bash
pio --version
```

If you see version 6.x or higher, you're ready. If `pio: command not found`, ensure `~/.local/bin` is in PATH or run `export PATH="$HOME/.local/bin:$PATH"` and add to `~/.bashrc`.

#### 3. Fix line endings (WSL or Windows checkout only)

If cloned on Windows, shell scripts may have CRLF. Fix in WSL/Linux:

```bash
sed -i 's/\r$//' build.sh receiver/scripts/fetch_cube.sh fix_crlf.sh sender/run.sh
```

### Building the Firmware

#### Build everything (receiver + sender)

From repo root:

```bash
sh build.sh
```

- **Receiver**: Auto-fetches STM32CubeF4 into `receiver/vendor/` (first time, ~1GB download), then compiles. Output: `receiver/build/rots_receiver.{elf,hex,bin}`.
- **Sender**: Compiles with PlatformIO. Output: `sender/.pio/build/esp32dev/firmware.bin`.

#### Build receiver only

```bash
cd receiver
make
```

The `make` command automatically runs `make deps` to fetch STM32CubeF4 if needed. To use a different HAL path:

```bash
export STM32_CUBE_DIR=/path/to/STM32CubeF4
cd receiver && make
```

#### Build sender only

```bash
cd sender
pio run
# Or if pio has issues:
sh run.sh
```

### Flashing to Hardware

#### Receiver (STM32)

Connect ST-Link to the STM32 board, then:

```bash
cd receiver
make flash
```

Uses OpenOCD with ST-Link interface. Alternatively, use STM32CubeProgrammer or J-Link with the generated `.hex` or `.bin` file.

#### Sender (ESP32)

Connect ESP32 via USB (ensure driver installed), then:

```bash
cd sender
pio run -t upload
# Or:
sh run.sh -t upload
```

Monitor serial output:

```bash
pio device monitor
# Or:
python3 -m platformio device monitor
```

### Running the Cloud Server

#### 1. Install dependencies

```bash
cd cloud-server
npm install
```

#### 2. Set up MySQL database

```bash
# Install MySQL (if not already)
sudo apt-get install mysql-server
mysql -u root -p
```

In MySQL shell:

```sql
CREATE DATABASE rots_db;
CREATE USER 'rots'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON rots_db.* TO 'rots'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

#### 3. Install and configure MQTT broker

```bash
sudo apt-get install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

#### 4. Configure environment

```bash
cp .env.example .env
nano .env
```

Edit `.env` with your database and MQTT settings:

```env
DB_HOST=localhost
DB_USER=rots
DB_PASSWORD=your_password
DB_NAME=rots_db
MQTT_URL=mqtt://localhost:1883
PORT=3000
```

#### 5. Start server

```bash
npm start
# Or for development with auto-reload:
npm run dev
```

Access Web UI at `http://localhost:3000`.

### System Operation

#### Complete Setup Workflow

1. **Configure WiFi/MQTT** in sender and receiver source files (see Configuration section)
2. **Build firmware** for both sender and receiver (see Building section)
3. **Flash firmware** to ESP32 and STM32 boards (see Flashing section)
4. **Set up cloud server**: MySQL + Mosquitto + Node.js (see Running the Cloud Server)
5. **Power on sender**: ESP32 connects WiFi → reads sensors → AI inference → publishes to `rots/detection/001`
6. **Power on receiver**: STM32 connects WiFi (ESP8266) → subscribes `rots/command/001` → waits for commands
7. **Send commands**: Via Web UI (`http://localhost:3000`) or direct MQTT publish
8. **Monitor system**: Serial UART output (115200 baud), OLED display on receiver, or Web UI logs

#### Command Format (MQTT)

Publish to `rots/command/001` (or device-specific topic):

```json
{
  "message_type": 1,
  "odor_type": 1,
  "intensity": 80,
  "duration": 30,
  "pump_config": [70, 50, 30, 20, 10],
  "timestamp": 1234567890,
  "checksum": 12345
}
```

- `odor_type`: 1=Coffee, 2=Alcohol, 3=Lemon, 4=Mint, 5=Lavender
- `intensity`: 0–100%
- `duration`: seconds
- `pump_config`: speed for each of 5 pumps (0–100%)

### Troubleshooting

**Build Issues:**

- **`set: Illegal option -` or `: not found`** — CRLF line endings on Windows checkout. Fix in WSL:  
  ```bash
  sed -i 's/\r$//' build.sh receiver/scripts/fetch_cube.sh fix_crlf.sh sender/run.sh
  ```

- **`arm-none-eabi-gcc: command not found`** — Install toolchain: `sudo apt-get install gcc-arm-none-eabi`

- **`STM32CubeF4 fetch fails`** — Ensure git is installed and network available. Manual download: clone [STM32CubeF4](https://github.com/STMicroelectronics/STM32CubeF4) to `receiver/vendor/STM32CubeF4`.

**PlatformIO Issues:**

- **`pio: command not found`** — Install via pipx: `pipx install platformio && pipx ensurepath`, then restart terminal.

- **`AttributeError: resultcallback`** — System PlatformIO (4.3.4) incompatible with Python 3.12. **Solution**:  
  ```bash
  # Uninstall old system package and use pipx
  sudo apt remove platformio
  pipx install platformio
  pipx ensurepath
  # Restart terminal, then:
  cd sender && pio run
  ```
  
  Or use the wrapper without upgrading:  
  ```bash
  cd sender && sh run.sh        # Build
  cd sender && sh run.sh -t upload  # Flash
  ```

**Runtime Issues:**

- **Receiver not connecting to WiFi** — Check ESP8266 wiring (PA2/PA3), WiFi credentials in `receiver/src/rots_communication.h`.

- **Sender MQTT connection fails** — Verify broker address in `sender/src/rots_sender.h` and network connectivity.

- **Cloud server database errors** — Ensure MySQL is running and `.env` has correct credentials.

## Configuration

### Receiver (STM32)

Edit **`receiver/src/rots_communication.h`**:

```c
#define ROTS_WIFI_SSID            "YourWiFiSSID"
#define ROTS_WIFI_PASSWORD        "YourWiFiPassword"
#define ROTS_MQTT_BROKER_HOST     "mqtt.your-server.com"  // Or IP
#define ROTS_MQTT_BROKER_PORT     1883
#define ROTS_MQTT_CLIENT_ID       "ROTS_RECEIVER_001"
```

System parameters: **`receiver/config/rots_config.h`** (pump speeds, safety limits, etc.).

### Sender (ESP32)

Edit **`sender/src/rots_sender.h`**:

```cpp
#define ROTS_WIFI_SSID            "YourWiFiSSID"
#define ROTS_WIFI_PASSWORD        "YourWiFiPassword"
#define ROTS_MQTT_BROKER_HOST     "mqtt.your-server.com"
#define ROTS_MQTT_BROKER_PORT     1883
#define ROTS_MQTT_CLIENT_ID       "ROTS_SENDER_001"
```

Or override at compile time in **`sender/platformio.ini`** (build flags).

### Cloud Server

Copy and edit `.env`:

```bash
cd cloud-server
cp .env.example .env
nano .env
```

Example `.env`:

```env
DB_HOST=localhost
DB_USER=rots
DB_PASSWORD=your_db_password
DB_NAME=rots_db
MQTT_URL=mqtt://localhost:1883
PORT=3000
```

## Wiring & Pin Assignments

Detailed pin configurations are in the sub-project READMEs:

- **Sender**: See `sender/README.md` — MQ sensors on A0–A7, DHT22/BMP280 on I²C (GPIO21/22), status LEDs on GPIO2/4.
- **Receiver**: See `receiver/README.md` and `receiver/DEBUG_GUIDE.md` — USART1 (PA9/PA10) for debug, USART2 (PA2/PA3) for ESP8266, I²C (PB6/PB7) for OLED, TIM2/TIM3 PWM for pumps, GPIO for valves/fans, SPI1 (PA5/6/7 + PC5 CS) for Flash.

Pin conflicts are resolved in firmware (e.g. TIM2 uses PA0/1 + PB10/11 to avoid USART2). Adjust hardware wiring to match firmware pin definitions in `receiver/src/rots_receiver.h` and `sender/src/rots_sender.h`.

## Project Components

| Directory | Platform | Description | Details |
|-----------|----------|-------------|---------|
| [sender/](sender/) | ESP32 | Odor detection with 8× MQ sensors, edge AI inference, WiFi/MQTT | See `sender/README.md` |
| [receiver/](receiver/) | STM32F407VET6 | Odor generation: 5× pumps/valves, recipe manager, ESP8266 WiFi, OLED display | See `receiver/README.md` |
| [cloud-server/](cloud-server/) | Node.js | MQTT broker bridge, MySQL storage, REST API, Web UI | See `cloud-server/README.md` |

## License

GNU GENERAL PUBLIC LICENSE — see [LICENSE](LICENSE).
