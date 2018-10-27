# TODO: Update when .h files change

# Sources

SRCS = main.c startup_stm32f0xx.c
SRCS += system_stm32f0xx.c stm32f0xx_it.c
SRCS += src/stepper.c \
		src/usart.c \
		src/i2c.c \
		src/quaternion_filters.c \
		src/std_utils.c
S_SRCS = 

# Project name

PROJ_NAME = stm32f0xx-gcc-barebones
OUTPATH = build

OUTPATH := $(abspath $(OUTPATH))

BASEDIR := $(abspath ./)

AS=$(BINPATH)arm-none-eabi-as
CC=$(BINPATH)arm-none-eabi-gcc
LD=$(BINPATH)arm-none-eabi-gcc
OBJCOPY=$(BINPATH)arm-none-eabi-objcopy
OBJDUMP=$(BINPATH)arm-none-eabi-objdump
SIZE=$(BINPATH)arm-none-eabi-size
GDBTUI=$(BINPATH)arm-none-eabi-gdb

LINKER_SCRIPT = stm32f051r8_flash.ld

CPU = -mcpu=cortex-m0 -mthumb

CFLAGS  = $(CPU) -c -std=gnu99 -g -O2 -Wall
LDFLAGS  = $(CPU) -mlittle-endian -mthumb-interwork -nostartfiles -Wl,--gc-sections,-Map=$(OUTPATH)/$(PROJ_NAME).map,--cref --specs=nano.specs

CFLAGS += -msoft-float

# Default to STM32F051 if no device is passed
ifeq ($(DEVICE_DEF), )
DEVICE_DEF = STM32F051
# Configure lib/CMSIS/Include/arm_math.h
MATH_HEADER = ARM_MATH_CM0
endif

CFLAGS += -D$(DEVICE_DEF)
CFLAGS += -D$(MATH_HEADER)

#vpath %.c src
vpath %.a lib

# Includes
INCLUDE_PATHS = -I$(BASEDIR) -I$(BASEDIR)/lib/CMSIS/Include -I$(BASEDIR)/lib/CMSIS/Device/ST/STM32F0xx/Include

# Library paths
LIBPATHS = -L$(BASEDIR)/lib/STM32F0xx_StdPeriph_Driver/

# Libraries to link
LIBS = -lstdperiph -lc -lgcc -lnosys -lm

# Extra includes
INCLUDE_PATHS += -I$(BASEDIR)/lib/STM32F0xx_StdPeriph_Driver/inc

#CFLAGS += -Map $(OUTPATH)/$(PROJ_NAME).map

OBJS = $(SRCS:.c=.o)
OBJS += $(S_SRCS:.s=.o)

CROSS_ELF := $(OUTPATH)/$(PROJ_NAME).elf
CROSS_TARGET := $(OUTPATH)/$(PROJ_NAME).bin

###################################################

.PHONY: lib proj

all: lib proj
	$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf

lib:
	$(MAKE) -C lib FLOAT_TYPE=$(FLOAT_TYPE) BINPATH=$(BINPATH) DEVICE_DEF=$(DEVICE_DEF) BASEDIR=$(BASEDIR)

proj: $(OUTPATH)/$(PROJ_NAME).elf

.s.o:
	$(AS) $(CPU) -o $(addprefix $(OUTPATH)/, $@) $<

.c.o:
	$(CC) $(CFLAGS) -std=gnu99 $(INCLUDE_PATHS) -o $(addprefix  $(OUTPATH)/, $@) $<

$(OUTPATH)/$(PROJ_NAME).elf: $(OBJS)
	$(LD) $(LDFLAGS) -T$(LINKER_SCRIPT) $(LIBPATHS) -o $@ $(addprefix $(OUTPATH)/, $^) $(LIBS) $(LD_SYS_LIBS)
	$(OBJCOPY) -O ihex $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).bin
	$(OBJDUMP) -S --disassemble $(OUTPATH)/$(PROJ_NAME).elf > $(OUTPATH)/$(PROJ_NAME).dis

clean:
	rm -f $(OUTPATH)/*.o
	rm -f $(OUTPATH)/$(PROJ_NAME).elf
	rm -f $(OUTPATH)/$(PROJ_NAME).hex
	rm -f $(OUTPATH)/$(PROJ_NAME).bin
	rm -f $(OUTPATH)/$(PROJ_NAME).dis
	rm -f $(OUTPATH)/$(PROJ_NAME).map
	# Remove this line if you don't want to clean the libs as well
	$(MAKE) clean -C lib
	

.PHONY: flash
flash: proj $(CROSS_TARGET)
	st-flash write $(CROSS_TARGET) 0x8000000

.PHONY: erase
erase:
	st-flash erase

.PHONY: debug
debug: $(CROSS_ELF)
	xterm -e st-util &
	$(GDBTUI) --eval-command="target remote localhost:4242" $(CROSS_ELF) -ex 'load'
