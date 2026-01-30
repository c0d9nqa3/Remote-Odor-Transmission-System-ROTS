# ROTS Receiver Makefile for STM32F407VET6
# Author: ROTS Team
# Date: 2024

# Project name
PROJECT = rots_receiver

# Target MCU
MCU = STM32F407VET6

# Compiler and tools
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# Directories
SRC_DIR = src
INC_DIR = src
CONFIG_DIR = config
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
# Startup file (from STM32CubeF4)
STARTUP_SRC = $(STM32_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s
STARTUP_OBJ = $(OBJ_DIR)/startup_stm32f407xx.o
# HAL driver sources (minimal set for ROTS)
HAL_SRC_DIR = $(STM32_CUBE_DIR)/Drivers/STM32F4xx_HAL_Driver/Src
HAL_SRCS = stm32f4xx_hal.c stm32f4xx_hal_rcc.c stm32f4xx_hal_gpio.c stm32f4xx_hal_cortex.c \
           stm32f4xx_hal_uart.c stm32f4xx_hal_i2c.c stm32f4xx_hal_adc.c stm32f4xx_hal_spi.c \
           stm32f4xx_hal_tim.c stm32f4xx_hal_dma.c
HAL_OBJS = $(addprefix $(OBJ_DIR)/,$(HAL_SRCS:.c=.o))
SYS_SRC = $(STM32_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
SYS_OBJ = $(OBJ_DIR)/system_stm32f4xx.o
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) $(STARTUP_OBJ) $(HAL_OBJS) $(SYS_OBJ)

# Include directories
# Use vendor/STM32CubeF4 (run 'make deps' or ./scripts/fetch_cube.sh first). Override with STM32_CUBE_DIR if needed.
VENDOR_CUBE := $(shell pwd)/vendor/STM32CubeF4
STM32_CUBE_DIR ?= $(VENDOR_CUBE)
INCLUDES = -I$(INC_DIR) -I$(CONFIG_DIR) -I$(STM32_CUBE_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc \
           -I$(STM32_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
           -I$(STM32_CUBE_DIR)/Drivers/CMSIS/Include

# Compiler flags
CFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS += -DSTM32F407xx -DUSE_HAL_DRIVER
CFLAGS += -DHAL_CAN_MODULE_ENABLED=0 -DHAL_CAN_LEGACY_MODULE_ENABLED=0
CFLAGS += -Wall -Wextra -Wpedantic -Werror
CFLAGS += -O2 -g3 -ggdb
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += $(INCLUDES)

# Linker flags
LDFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
LDFLAGS += -specs=nano.specs -T$(LINKER_SCRIPT)
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage

# Linker script
LINKER_SCRIPT = $(shell pwd)/STM32F407VETx_FLASH.ld

# Fetch STM32CubeF4 if missing (run from receiver/). Use 'sh' to avoid shebang/CRLF issues.
deps:
	@if [ ! -d "$(VENDOR_CUBE)/Drivers" ]; then \
		sh "$(shell pwd)/scripts/fetch_cube.sh"; \
	fi

# Default target
all: deps $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin

# Create build directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile startup file
$(STARTUP_OBJ): $(STARTUP_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile HAL driver sources
$(OBJ_DIR)/stm32f4xx_hal%.o: $(HAL_SRC_DIR)/stm32f4xx_hal%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile system file
$(SYS_OBJ): $(SYS_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

# Generate hex file
$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

# Generate binary file
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary -S $< $@

# Generate disassembly
$(BUILD_DIR)/$(PROJECT).lst: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJDUMP) -h -S $< > $@

# Flash to MCU
flash: $(BUILD_DIR)/$(PROJECT).hex
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $< verify reset exit"

# Debug with GDB
debug: $(BUILD_DIR)/$(PROJECT).elf
	arm-none-eabi-gdb $< -ex "target remote localhost:3333" -ex "monitor reset halt" -ex "load" -ex "monitor reset halt"

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Show help
help:
	@echo "Available targets:"
	@echo "  deps     - Fetch STM32CubeF4 into vendor/ (git required)"
	@echo "  all      - Build project (default, runs deps if needed)"
	@echo "  flash    - Flash to MCU"
	@echo "  debug    - Start GDB debug session"
	@echo "  clean    - Clean build files"
	@echo "  help     - Show this help"

# Dependencies
-include $(OBJECTS:.o=.d)

.PHONY: all deps flash debug clean help
