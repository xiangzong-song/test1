#!/bin/bash -
set -o nounset

PLATFORM_INCLUDE_DIR="../../../../platform/Govee_Light_Lib/include"
PLATFORM_NEED_FILES="hci_test.h ble_manager.h button_manager.h config.h hal_os.h hal_gpio.h hal_pwm.h led.h light_md5.h mcu_i2c.h ringbuffer.h runtime.h soft_i2c.h software_fft.h sys_queue.h device_time.h uart_manager.h utils.h auto_init.h audio_manager.h"

./sdk_build.sh 1
rm ../../platform/Govee_Light_Lib/libs/libGovee_Light_SDK.a
rm ../../platform/Govee_Light_Lib/include/*.h
cp build/libGovee_Light_SDK.a  ../../platform/Govee_Light_Lib/libs

cd build/include
cp ${PLATFORM_NEED_FILES} ${PLATFORM_INCLUDE_DIR}
 
