#Set this to @ to keep the makefile quiet
SILENCE = @
#---- Outputs ----#
COMPONENT_NAME = GoveeLightSDK
TARGET_LIB = \
	lib/lib$(COMPONENT_NAME).a
TEST_TARGET = \
	$(COMPONENT_NAME)_tests

CPPUTEST_HOME=../cpputest
GOVEE_LIGHT_SDK_HOME=../Govee_Light_SDK
GOVEE_LIGHT_LIB_HOME=../platform/Govee_Light_Lib
#--- Inputs ----#
PROJECT_HOME_DIR = .
CPP_PLATFORM = Gcc
CPPUTEST_OBJS_DIR=./build/
CPPUTEST_LIB_DIR=./build/lib/
CPPUTEST_ENABLE_DEBUG := Y
CPPUTEST_USE_EXTENSIONS = Y
CPPUTEST_WARNINGFLAGS += -Wall 
# CPPUTEST_WARNINGFLAGS += -Werror 
CPPUTEST_WARNINGFLAGS += -Wswitch-default 
CPPUTEST_WARNINGFLAGS += -Wswitch-enum  
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    #CCFLAGS += -D LINUX
	CPPUTEST_WARNINGFLAGS += -Wno-unused-parameter  
	CPPUTEST_WARNINGFLAGS += -Wno-unused-parameter -Wno-pedantic -Wno-comment -Wno-cast-function-type -Wno-switch-enum
endif
ifeq ($(UNAME_S),Darwin)
	CPPUTEST_WARNINGFLAGS += -Wno-suggest-override -Wno-suggest-destructor-override -Wno-poison-system-directories -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
	CPPUTEST_WARNINGFLAGS += -Wno-self-assign -Wno-unused-macros -Wno-unused-function
	CPPUTEST_WARNINGFLAGS += -Wno-reserved-id-macro -Wno-keyword-macro -Wno-unused-parameter -Wno-pedantic -Wno-shadow -Wno-format-nonliteral -Wno-documentation -Wno-shorten-64-to-32 -Wno-covered-switch-default
	CPPUTEST_WARNINGFLAGS +=-Wno-switch-enum -Wno-missing-variable-declarations -Wno-cast-align -Wno-unreachable-code
endif
CPPUTEST_CFLAGS += -std=c99
#CPPUTEST_CFLAGS += -std=gnu89
CPPUTEST_CFLAGS += -Wextra 
CPPUTEST_CFLAGS += -pedantic 
#CPPUTEST_CFLAGS += -Tldscript.ld
CPPUTEST_CFLAGS += -Wstrict-prototypes
#CPPUTEST_LDFLAGS+=-fuse-ld=lld
#CPPUTEST_LDFLAGS +=-T ldscript.ld
#CPPUTEST_MAP_FILE=Y
SILENCE=
SRC_DIRS = \
	$(GOVEE_LIGHT_SDK_HOME)/code/sdk/service/audio_manager/\
	$(GOVEE_LIGHT_SDK_HOME)/code/sdk/library/fft/\
	src/platform/linux/

TEST_SRC_DIRS = \
	 tests/application/system \
	 tests/\
	 mocks
INCLUDES_FILTER	=$(GOVEE_LIGHT_LIB_HOME)/include/system_private.h
INCLUDE_DIRS =\
	$(GOVEE_LIGHT_SDK_HOME)/code/hal/os/\
	$(GOVEE_LIGHT_SDK_HOME)/code/sdk/service/audio_manager/\
	$(GOVEE_LIGHT_SDK_HOME)/code/sdk/library/fft
#   $(APPLICATION_HOME)/system/\
#   $(APPLICATION_HOME)/include/\
  $(CPPUTEST_HOME)/include/\
  $(GOVEE_LIGHT_LIB_HOME)/include/\
  mocks

ifeq ($(CPPUTEST_HOME),)
$(info CPPUTEST_HOME not set! See README.txt)
else  
include $(CPPUTEST_HOME)/build/MakefileWorker.mk
endif
