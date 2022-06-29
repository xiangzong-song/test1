TEMPLATE_PATH := ./

ifneq ($(platform), )
    PLATFORM=$(platform)
else
    PLATFORM=FR8016
endif

ifeq ($(PLATFORM),FR8016)
    PLATFORM_PATH := ESD_FR801xH-Platform
    SDK_NAME := sdk
    LIBRARY_TYPE := 801X
endif

ifeq ($(PLATFORM),FR5089)
    PLATFORM_PATH := ESD_FR5089_Platform
    SDK_NAME := fr801xt_M3
    LIBRARY_TYPE := 508X
endif


include $(TEMPLATE_PATH)/Makefile.param

PROJECT_NAME     := Govee_SDK_Lib
TARGETS          :=$(MODULES_NAME)_$(LIBRARY_TYPE)
OUTPUT_DIRECTORY := build
SDK_ROOT         := ../../platform/$(PLATFORM_PATH)/Freq_SDK/$(SDK_NAME)
GOVEE_SDK_SRC    := ../code/
LINKER_SCRIPT    := ldscript.ld


GOVEE_SDK_FILE_ALL += $(foreach dir, $(GOVEE_SDK_FILE_DIRS), $(wildcard $(GOVEE_SDK_SRC)$(dir)/*.c)) 
GOVEE_SDK_FILE_C = $(filter-out $(IGNORE_FILE), $(GOVEE_SDK_FILE_ALL)) 

#driver
SDK_ROOT_COMPONENTS_DRIVER=$(SDK_ROOT)/components/driver
SDK_ROOT_DRIVER_DIRS = $(shell find $(SDK_ROOT_COMPONENTS_DRIVER) -type d)

#SDK_ROOT_COMPONENTS_BLE=$(SDK_ROOT)/components/ble
#SDK_ROOT_BLE_DIRS = $(shell find $(SDK_ROOT_COMPONENTS_BLE) -type d)
#SRC_FILES += $(foreach dir, $(SDK_ROOT_BLE_DIRS), $(wildcard $(dir)/*.c))

SRC_FILES += $(GOVEE_SDK_FILE_C)
SRC_FILES += $(SRC_FILES_COMM)
# Include folders common to all targets
GOVEE_SDK_FOLDERS += $(foreach dir, $(GOVEE_SDK_FILE_DIRS), $(wildcard $(GOVEE_SDK_SRC)$(dir))) 

SDK_INC_FOLDERS += \
  $(SDK_ROOT)/components/ble/include \
  $(SDK_ROOT)/components/ble/profiles/ble_ota \
  $(SDK_ROOT)/components/modules/os/include \
  $(SDK_ROOT)/components/modules/sys/include \
  $(SDK_ROOT)/components/modules/platform/include \
  $(SDK_ROOT)/components/modules/common/include \
  $(SDK_ROOT)/components/modules/lowpow/include \
  $(SDK_ROOT)/components/ble/include/gap \
  $(SDK_ROOT)/components/ble/include/gatt \
  $(SDK_ROOT)/components/ble/profiles/ble_simple_profile \
  $(SDK_ROOT)/components/ble/include/mesh \

INC_FOLDERS +=$(SDK_INC_FOLDERS)
INC_FOLDERS +=$(SDK_ROOT_DRIVER_DIRS)
INC_FOLDERS +=$(GOVEE_SDK_FOLDERS)

# Optimization flags
#OPT = -Os -g -Wall
OPT = -Os -g -Werror
OPT += -DPLATFORM_TYPE=\"$(LIBRARY_TYPE)\"

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -mcpu=cortex-m3
CFLAGS += -mthumb
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fmessage-length=0 -fsigned-char
CFLAGS += -std=gnu11

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m3
ASMFLAGS += -mthumb

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb
LDFLAGS += -mcpu=cortex-m3
LDFLAGS +=  -T$(LINKER_SCRIPT)
LDFLAGS += $(SDK_ROOT)/components/ble/library/syscall_gcc.txt
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections


include $(TEMPLATE_PATH)/Makefile.common

SDK_INC_FILES += $(foreach dir, $(SDK_INC_FOLDERS), $(wildcard $(dir)/*.h)) 
GOVEE_INC_FILES += $(foreach dir, $(GOVEE_SDK_FOLDERS), $(wildcard $(dir)/*.h)) 

$(shell ./platform_config.sh $(PLATFORM))

$(foreach target, $(TARGETS), $(call define_target, $(target)))

