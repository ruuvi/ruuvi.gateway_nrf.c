PROJECT_NAME     := ruuvigw
TARGETS          := nrf52832_xxaa
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../nRF5_SDK_15.3.0_59ac345
PROJ_DIR := ../../
TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc
LINKER_PATH := $(SDK_ROOT)/modules/nrfx/mdk

include ${PROJ_DIR}/gcc_sources.make

$(OUTPUT_DIRECTORY)/nrf52832_xxaa.out: \
  LINKER_SCRIPT  := pca10040.ld

# make DEBUG=1 for debug build with traces
DEBUG ?= 1
# make OPT=0 for non-optimized binary
OPT ?= 3
# make DBGSYM=1 to include debug symbols
DBGSYM ?=0

# Source files
SRC_FILES += \
  $(common_sources) \
  $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52.S

# Include folders common to all targets
INC_FOLDERS += $(common_includes)

# Libraries common to all targets
LIB_FILES += \

# Optimization flags
INTOPT = -O$(OPT)

ifeq ($(DEBUG), 1)
    INTOPT += -g3
    CUSTOM_CFLAGS += -DRUUVI_TRACES=1
    CUSTOM_CFLAGS += -DDEBUG_NRF
    CUSTOM_CFLAGS += -DDEBUG_NRFX
    CUSTOM_CFLAGS += -DDEBUG
    CUSTOM_ASMFLAGS += -DDEBUG_NRF
    CUSTOM_ASMFLAGS += -DDEBUG_NRFX
    CUSTOM_ASMFLAGS += -DDEBUG
else
ifeq ($(DBGSYM), 1)
    INTOPT += -g3
endif
endif

# C flags common to all targets
CFLAGS += $(INTOPT)
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_PCA10040
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52
CFLAGS += -DNRF52832_XXAA
CFLAGS += -DNRF52_PAN_74
CFLAGS += -DNRF_SD_BLE_API_VERSION=6
CFLAGS += -DS132
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DUSE_APP_CONFIG
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums
CFLAGS += $(CUSTOM_CFLAGS)

# C++ flags common to all targets
CXXFLAGS += $(INTOPT)

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_PCA10040
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52832_XXAA
ASMFLAGS += -DNRF52_PAN_74
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=6
ASMFLAGS += -DS132
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DUSE_APP_CONFIG
ASMFLAGS += $(CUSTOM_ASMFLAGS)

# Linker flags
LDFLAGS += $(INTOPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs
LDFLAGS += -DUSE_APP_CONFIG
LDFLAGS += $(CUSTOM_LDFLAGS)

nrf52832_xxaa: CFLAGS += -D__HEAP_SIZE=8192
nrf52832_xxaa: CFLAGS += -D__STACK_SIZE=8192
nrf52832_xxaa: ASMFLAGS += -D__HEAP_SIZE=8192
nrf52832_xxaa: ASMFLAGS += -D__STACK_SIZE=8192

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm


.PHONY: default help

# Default target - first one defined
default: nrf52832_xxaa

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo		nrf52832_xxaa
	@echo		flash_softdevice
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash flash_softdevice erase

# Flash the program
flash: default
	@echo Flashing: $(OUTPUT_DIRECTORY)/nrf52832_xxaa.hex
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/nrf52832_xxaa.hex --sectorerase
	nrfjprog -f nrf52 --reset

# Flash softdevice
flash_softdevice:
	@echo Flashing: s132_nrf52_6.1.1_softdevice.hex
	nrfjprog -f nrf52 --program $(SDK_ROOT)/components/softdevice/s132/hex/s132_nrf52_6.1.1_softdevice.hex --sectorerase
	nrfjprog -f nrf52 --reset

erase:
	nrfjprog -f nrf52 --eraseall
