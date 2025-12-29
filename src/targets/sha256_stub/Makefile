PROJECT_NAME     := ruuvigw
TARGETS          := sha256_stub
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../nRF5_SDK_15.3.0_59ac345
PROJ_DIR := ../..

$(OUTPUT_DIRECTORY)/sha256_stub.out: \
  LINKER_SCRIPT  := sha256_stub_ram.ld


# make DEBUG=1 for debug build with traces
DEBUG ?= 1
# make OPT=0 for non-optimized binary
OPTLVL ?= s
# make DBGSYM=1 to include debug symbols
DBGSYM ?=0

# Source files
SRC_FILES += \
  $(PROJ_DIR)/sha256_stub.c

#  $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52.S

# Optimization flags
OPT ?= -O$(OPTLVL)

ifeq ($(DEBUG), 1)
    OPT += -g3
    CUSTOM_CFLAGS += -DRUUVI_TRACES=1
    CUSTOM_CFLAGS += -DDEBUG_NRF
    CUSTOM_CFLAGS += -DDEBUG_NRFX
    CUSTOM_CFLAGS += -DDEBUG
    CUSTOM_ASMFLAGS += -DDEBUG_NRF
    CUSTOM_ASMFLAGS += -DDEBUG_NRFX
    CUSTOM_ASMFLAGS += -DDEBUG
else
ifeq ($(DBGSYM), 1)
    OPT += -g3
endif
endif

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DAPPLICATION_DRIVER_CONFIGURED
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_RUUVIGW_NRF
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DINITIALIZE_USER_SECTIONS
CFLAGS += -DNO_VTOR_CONFIG
CFLAGS += -DNRF52
CFLAGS += -DNRF52811_XXAA
CFLAGS += -DFLOAT_ABI_SOFT
CFLAGS += -DNRF52_PAN_74
CFLAGS += -DNRF_SD_BLE_API_VERSION=6
CFLAGS += -DSWI_DISABLE0
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=soft
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fstack-usage
CFLAGS += -fno-builtin -fshort-enums
CFLAGS += -ffreestanding -nostdlib
CFLAGS += $(CUSTOM_CFLAGS)

# C++ flags common to all targets
CXXFLAGS += $(OPT)


# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=soft
ASMFLAGS += -DBL_SETTINGS_ACCESS_ONLY
ASMFLAGS += -DBOARD_RUUVIGW_NRF
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_SOFT
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52811_XXAA
ASMFLAGS += -DNRF52_PAN_74
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DUSE_APP_CONFIG
ASMFLAGS += $(CUSTOM_ASMFLAGS)

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=soft
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs
LDFLAGS += -DUSE_APP_CONFIG
LDFLAGS += $(CUSTOM_LDFLAGS)

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES +=

.PHONY: default print_cflags

# Default target - first one defined
default: sha256_stub

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc

include $(TEMPLATE_PATH)/Makefile.common
$(foreach target, $(TARGETS), $(call define_target, $(target)))

print_cflags:
	which arm-none-eabi-gcc
	@echo "CFLAGS: " $(CFLAGS) "\n"
