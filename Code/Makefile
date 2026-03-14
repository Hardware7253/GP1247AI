# Partially ChatGPT generated
CC = arm-none-eabi-gcc
CFLAGS  ?=  -Wextra -Werror -Wshadow -Wdouble-promotion \
            -Wformat-truncation -fno-common \
            -g3 -Os -ffunction-sections -fdata-sections \
            -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP
LDFLAGS ?= -Tlink.ld -nostartfiles -nostdlib --specs nano.specs \
			-lc -lgcc -lm -Wl,--gc-sections -Wl,-Map=$@.map

DEFINES := \
	-DSTM32F407xx
CFLAGS += $(DEFINES)

# Add application specific user libraries here
LIBRARIES := \
	embedded_c_modules \
	u8g2/csrc \
	tinyusb/src \
	usb

INCLUDES := -I. \
	-I./CMSIS_5/CMSIS/Core/Include \
	-I./cmsis-device-f4/Include \
	-I./cmsis-device-f4/Source/Templates \
	-I./stm32f4xx-hal-driver/Inc \
	$(foreach lib,$(LIBRARIES),$(addprefix -I,$(lib))) \
	$(foreach lib,$(LIBRARIES),$(addprefix -I,$(wildcard ./$(lib)/*/)))
CFLAGS += $(INCLUDES)

# Application sources
SOURCES := $(wildcard *.c)

# HAL sources
SOURCE_EXCLUDES := \
	stm32f4xx-hal-driver/Src/stm32f4xx_hal_msp_template.c \
	stm32f4xx-hal-driver/Src/stm32f4xx_hal_timebase_rtc_alarm_template.c \
	stm32f4xx-hal-driver/Src/stm32f4xx_hal_timebase_rtc_wakeup_template.c \
	stm32f4xx-hal-driver/Src/stm32f4xx_hal_timebase_tim_template.c
SOURCES += $(wildcard stm32f4xx-hal-driver/Src/*.c)
SOURCES := $(filter-out $(SOURCE_EXCLUDES), $(SOURCES))

# CMSIS sources
SOURCES += cmsis-device-f4/Source/Templates/gcc/startup_stm32f407xx.s
SOURCES += cmsis-device-f4/Source/Templates/system_stm32f4xx.c

# Add application code and library sources
C_SOURCES := $(filter %.c,$(SOURCES))
S_SOURCES := $(filter %.s,$(SOURCES))
C_SOURCES += $(shell find $(LIBRARIES) -type f -name '*.c')
S_SOURCES += $(shell find $(LIBRARIES) -type f -name '*.s')
OBJECTS := $(addprefix target/,$(C_SOURCES:.c=.o))
OBJECTS += $(addprefix target/,$(S_SOURCES:.s=.o))
DEPENDS := $(OBJECTS:.o=.d)

submodules:
	git submodule init
	git submodule update 

build: submodules target/firmware.elf

target:
	mkdir -p target

target/%.o: %.c | target
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

target/%.o: %.s | target
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

target/firmware.elf: $(OBJECTS) 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) 

target/firmware.bin: target/firmware.elf
	arm-none-eabi-objcopy -O binary $< $@

flash: target/firmware.bin submodules
	st-flash --reset write $< 0x8000000

clean:
	rm -rf target 

rmsub:
	rm -rf CMSIS_5 cmsis-device-f4 stm32f4xx-hal-driver

-include $(DEPENDS)
