# ROTS Receiver Build Instructions

## Prerequisites

### Required Tools
1. **ARM GCC Toolchain**
   - Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
   - Version: 10.3 or later
   - Add to PATH: `export PATH=$PATH:/path/to/gcc-arm-none-eabi-10.3/bin`

2. **STM32CubeF4 HAL Library**
   - Download from: https://www.st.com/en/embedded-software/stm32cubef4.html
   - Extract to a known location (e.g., `/opt/STM32CubeF4`)
   - Set environment variable: `export STM32_CUBE_DIR=/path/to/STM32CubeF4`

3. **Make**
   - Linux/Mac: Usually pre-installed
   - Windows: Install via MinGW or use WSL

4. **OpenOCD** (for flashing/debugging)
   - Download from: http://openocd.org/
   - Or install via package manager:
     - Ubuntu/Debian: `sudo apt-get install openocd`
     - macOS: `brew install openocd`

## Building the Project

### 1. Set Environment Variables

```bash
export STM32_CUBE_DIR=/path/to/STM32CubeF4
export PATH=$PATH:/path/to/gcc-arm-none-eabi-10.3/bin
```

### 2. Configure Build Settings

Edit `Makefile` and update:
- `STM32_CUBE_DIR`: Path to STM32CubeF4 directory
- `LINKER_SCRIPT`: Path to linker script (should be `STM32F407VETx_FLASH.ld`)

### 3. Build

```bash
cd receiver
make clean
make all
```

This will generate:
- `build/rots_receiver.elf` - ELF executable
- `build/rots_receiver.hex` - Intel HEX format
- `build/rots_receiver.bin` - Binary format

### 4. Flash to MCU

Using OpenOCD and ST-Link:

```bash
make flash
```

Or manually:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/rots_receiver.hex verify reset exit"
```

### 5. Debug

Terminal 1 (OpenOCD):
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

Terminal 2 (GDB):
```bash
make debug
```

## Project Structure

```
receiver/
├── src/                    # Source files
│   ├── main.c             # Main application
│   ├── rots_*.c/h         # ROTS modules
│   └── stm32f4xx_it.c     # Interrupt handlers
├── config/                # Configuration files
│   └── rots_config.h     # System configuration
├── build/                 # Build output (generated)
├── Makefile              # Build configuration
└── STM32F407VETx_FLASH.ld # Linker script
```

## Troubleshooting

### Build Errors

1. **"Cannot find stm32f4xx_hal.h"**
   - Check `STM32_CUBE_DIR` environment variable
   - Verify STM32CubeF4 is installed correctly

2. **"Linker script not found"**
   - Ensure `STM32F407VETx_FLASH.ld` exists in receiver directory
   - Check `LINKER_SCRIPT` path in Makefile

3. **"arm-none-eabi-gcc: command not found"**
   - Add ARM GCC toolchain to PATH
   - Verify installation: `arm-none-eabi-gcc --version`

### Flash Errors

1. **"No ST-Link found"**
   - Check USB connection
   - Install ST-Link drivers
   - Verify with: `lsusb | grep ST-Link`

2. **"Target not responding"**
   - Press reset button on board
   - Check BOOT0 pin (should be LOW)
   - Try different USB port/cable

## Configuration

Edit `config/rots_config.h` to modify:
- WiFi credentials
- MQTT broker settings
- Hardware pin assignments
- System parameters

## Notes

- The project uses STM32 HAL library for hardware abstraction
- Debug output is available via USART1 (115200 baud)
- ESP8266 communication uses USART2 (115200 baud)
- I2C1 is used for OLED display (SSD1306)
