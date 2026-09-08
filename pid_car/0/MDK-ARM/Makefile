# ******************************************************************************
# Makefile for STM32F103C8 Balance Car Project
# Generated for ARM GNU Toolchain (arm-none-eabi-gcc)
# Based on Keil MDK project.uvprojx
# ******************************************************************************

# Project Name
TARGET = project

# Toolchain
PREFIX = arm-none-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
AS      = $(PREFIX)gcc -x assembler-with-cpp
CP      = $(PREFIX)objcopy
SZ      = $(PREFIX)size
HEX     = $(CP) -O ihex
BIN     = $(CP) -O binary -S

# CPU
CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb

# Defines
DEFS = -DUSE_HAL_DRIVER -DSTM32F103xB

# Project root directory
ROOT = ..

# Directories
BUILD_DIR = build
CORE_DIR  = $(ROOT)/Core
HAL_DIR   = $(ROOT)/Drivers/STM32F1xx_HAL_Driver/Src
CMSIS_DIR = $(ROOT)/Drivers/CMSIS
HW_DIR    = $(ROOT)/Hardware/Src

# Include paths
INCLUDES = \
  -I$(CORE_DIR)/Inc \
  -I$(HAL_DIR)/../Inc \
  -I$(HAL_DIR)/../Inc/Legacy \
  -I$(CMSIS_DIR)/Device/ST/STM32F1xx/Include \
  -I$(CMSIS_DIR)/Include \
  -I$(HW_DIR)/../Inc \
  -I$(ROOT)/Hardware/Inc

# Compiler flags
CFLAGS  = $(MCU) $(DEFS) $(INCLUDES)
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -Os -g -std=gnu11

# Linker flags
LDSCRIPT = STM32F103C8TX_FLASH.ld
LDFLAGS  = $(MCU) -specs=nosys.specs -T$(LDSCRIPT)
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -lm -lc -lgcc

# Assembler flags
ASFLAGS = $(MCU) $(DEFS) $(INCLUDES) -Wall

# ============================================================================
# Source files (deduplicated)
# ============================================================================

# GCC startup file
ASM_SRC = $(CMSIS_DIR)/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s

# Core sources
C_SRC  = $(CORE_DIR)/Src/main.c
C_SRC += $(CORE_DIR)/Src/balance.c
C_SRC += $(CORE_DIR)/Src/gpio.c
C_SRC += $(CORE_DIR)/Src/i2c.c
C_SRC += $(CORE_DIR)/Src/tim.c
C_SRC += $(CORE_DIR)/Src/usart.c
C_SRC += $(CORE_DIR)/Src/stm32f1xx_it.c
C_SRC += $(CORE_DIR)/Src/stm32f1xx_hal_msp.c
C_SRC += $(CORE_DIR)/Src/system_stm32f1xx.c

# HAL Driver sources
C_SRC += $(HAL_DIR)/stm32f1xx_hal.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_cortex.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_dma.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_exti.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_flash.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_flash_ex.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_gpio.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_gpio_ex.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_i2c.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_pwr.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_rcc.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_rcc_ex.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_tim.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_tim_ex.c
C_SRC += $(HAL_DIR)/stm32f1xx_hal_uart.c

# Hardware sources
C_SRC += $(HW_DIR)/LED.c
C_SRC += $(HW_DIR)/Key.c
C_SRC += $(HW_DIR)/Motor.c
C_SRC += $(HW_DIR)/MPU6050.c
C_SRC += $(HW_DIR)/OLED.c
C_SRC += $(HW_DIR)/PWM.c
C_SRC += $(HW_DIR)/Serial.c
C_SRC += $(HW_DIR)/BlueSerial.c
C_SRC += $(HW_DIR)/Encoder.c
C_SRC += $(HW_DIR)/PID.c
C_SRC += $(HW_DIR)/DebugMode.c

# ============================================================================
# Object files
# ============================================================================
OBJECTS  = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SRC:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SRC:.s=.o)))

# VPATH for finding source files in their directories
vpath %.c $(sort $(dir $(C_SRC)))
vpath %.s $(sort $(dir $(ASM_SRC)))

# ============================================================================
# Build rules
# ============================================================================

.PHONY: all clean flash

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@echo "Build complete: $(TARGET)"
	@$(SZ) $(BUILD_DIR)/$(TARGET).elf

# Create build directory
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

# Link
$(BUILD_DIR)/$(TARGET).elf: $(BUILD_DIR) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Linking complete."

# Compile C sources
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Assemble
$(BUILD_DIR)/%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

# Hex file
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(HEX) $< $@

# Binary file
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(BIN) $< $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Include auto-generated dependencies
-include $(wildcard $(BUILD_DIR)/*.d)
