#!/bin/bash

PLATFORM_HEADER_FILES="platform.h hci_test.h ble_manager.h button_manager.h config.h hal_os.h hal_gpio.h hal_pwm.h led.h light_md5.h mcu_i2c.h ringbuffer.h runtime.h soft_i2c.h software_fft.h sys_queue.h device_time.h uart_manager.h utils.h auto_init.h audio_manager.h watchdog_manager.h light_system.h"

PLATFORM_8016_INC="../../platform/ESD_FR801xH-Platform/Govee_Light_Lib/include"
PLATFORM_8016_LIB="../../platform/ESD_FR801xH-Platform/Govee_Light_Lib/libs"
PLATFORM_5089_INC="../../platform/ESD_FR5089_Platform/Govee_Light_Lib/include"
PLATFORM_5089_LIB="../../platform/ESD_FR5089_Platform/Govee_Light_Lib/libs"

CURRENT_PATH=$(pwd)

if [  -z $1 ]; then
    ./sdk_build.sh FR8016
    rm ${PLATFORM_8016_INC}/*.h
    rm ${PLATFORM_8016_LIB}/*.a
    cp build/*.a $PLATFORM_8016_LIB
    cd build/include
    cp $PLATFORM_HEADER_FILES ../../${PLATFORM_8016_INC}
else
    if [ $1 = "FR8016" ]; then
        ./sdk_build.sh FR8016
        rm ${PLATFORM_8016_INC}/*.h
        rm ${PLATFORM_8016_LIB}/*.a
        cp build/*.a $PLATFORM_8016_LIB
        cd build/include
        cp $PLATFORM_HEADER_FILES ../../${PLATFORM_8016_INC}
    elif [ $1 = "FR5089" ]; then
        ./sdk_build.sh FR5089
        rm ${PLATFORM_5089_INC}/*.h
        rm ${PLATFORM_5089_LIB}/*.a
        cp build/*.a $PLATFORM_5089_LIB
        cd build/include
        cp $PLATFORM_HEADER_FILES ../../${PLATFORM_5089_INC}
    elif [ $1 = "all" ]; then
        ./sdk_build.sh FR8016
        rm ${PLATFORM_8016_INC}/*.h
        rm ${PLATFORM_8016_LIB}/*.a
        cp build/*.a $PLATFORM_8016_LIB
        cd build/include
        cp $PLATFORM_HEADER_FILES ../../${PLATFORM_8016_INC}
        cd $CURRENT_PATH
        ./sdk_build.sh FR5089
        rm ${PLATFORM_5089_INC}/*.h
        rm ${PLATFORM_5089_LIB}/*.a
        cp build/*.a $PLATFORM_5089_LIB
        cd build/include
        cp $PLATFORM_HEADER_FILES ../../${PLATFORM_5089_INC}
    else
        echo "Error : invalid platform '$1'"
        exit
    fi 
fi

