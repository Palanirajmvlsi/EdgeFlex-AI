# EdgeFlex AI - STM32F411CEU6 firmware build
# ARM Cortex-M4 / 512 KB Flash / 128 KB SRAM

TARGET      = edgeflex_ai
BUILD_DIR   = build
CC          = arm-none-eabi-gcc
OBJCOPY     = arm-none-eabi-objcopy
SIZE        = arm-none-eabi-size

MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
C_DEFS = -DSTM32F411xE -DUSE_HAL_DRIVER

C_INCLUDES = \
  -ICore/Inc \
  -IDrivers/CMSIS/Include \
  -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IDrivers/STM32F4xx_HAL_Driver/Inc \
  -IEdgeFlex_Runtime/Inc \
  -IModels

CFLAGS = $(MCU_FLAGS) $(C_DEFS) $(C_INCLUDES) \
  -Wall -Wextra -Wno-unused-parameter \
  -ffunction-sections -fdata-sections \
  -Og -g3 -std=gnu11 -MMD -MP

LDSCRIPT = STM32F411CEUX_FLASH.ld
LDFLAGS = $(MCU_FLAGS) -specs=nano.specs -specs=nosys.specs \
  -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
  -Wl,--gc-sections -Wl,--print-memory-usage

ASFLAGS = $(MCU_FLAGS) -x assembler-with-cpp -MMD -MP

C_SOURCES = \
  Core/Src/main.c \
  Core/Src/stm32f4xx_it.c \
  Core/Src/syscalls.c \
  Core/Src/sysmem.c \
  Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
  EdgeFlex_Runtime/Src/edgeflex_memory.c \
  EdgeFlex_Runtime/Src/edgeflex_model.c \
  EdgeFlex_Runtime/Src/edgeflex_inference.c \
  EdgeFlex_Runtime/Src/edgeflex_monitor.c \
  EdgeFlex_Runtime/Src/edgeflex_runtime.c \
  Models/tinymlp_model.c

ASM_SOURCES = Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean size
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin size

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(CC) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary -S $< $@

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)
