#!/bin/bash

BUILD_PATH=$(pwd)
PLATFORM_TYPE=FR8016
PLATFORM_NAME=ESD_FR801xH-Platform
SDK_FOLDER=sdk

if [ ! -z $1 ]; then
    if [ $1 = "FR8016" ]; then
        PLATFORM_TYPE=FR8016
        PLATFORM_NAME=ESD_FR801xH-Platform
        SDK_FOLDER=sdk
    elif [ $1 = "FR5089" ]; then
        PLATFORM_TYPE=FR5089
        PLATFORM_NAME=ESD_FR5089_Platform
        SDK_FOLDER=fr801x_M3
    else
        echo "Error : invalid platform '$1'"
        exit
    fi 
fi

echo "****************************************************"
echo "platform type : "$PLATFORM_TYPE
echo "platform name : "$PLATFORM_NAME
echo "****************************************************"

SDK_PATH_LINE=$(cat Makefile| grep ^SDK_ROOT\ )
SDK_PATH_TEXT=${SDK_PATH_LINE#*=}
SDK_PATH=${SDK_PATH_TEXT/'$(PLATFORM_PATH)'/$PLATFORM_NAME}
SDK_PATH=${SDK_PATH/'$(SDK_NAME)'/$SDK_FOLDER}
SDK_PATCH=/../${SDK_FOLDER}_patch
SDK_PATCH_PATH=$SDK_PATH$SDK_PATCH
cd $SDK_PATCH_PATH && sh sdk_patch.sh

cd $BUILD_PATH
echo "make clean and make platform=$PLATFORM_TYPE"

make clean && make platform=$PLATFORM_TYPE
if [ $? -ne "0" ]; then
    echo "make failed!!! please Check error"
    exit
fi
