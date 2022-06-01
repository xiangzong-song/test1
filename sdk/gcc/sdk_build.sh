#!/bin/sh

BUILD_PATH=$(pwd)
PLATFORM_TYPE=FR8016
PLATFORM_NAME=ESD_FR801xH-Platform

if [ ! -z $1 ]; then
    if [ $1 = "FR8016" ]; then
        PLATFORM_TYPE=FR8016
        PLATFORM_NAME=ESD_FR801xH-Platform
    elif [ $1 = "FR5089" ]; then
        PLATFORM_TYPE=FR5089
        PLATFORM_NAME=ESD_FR5089_Platform
    else
        echo "Error : invalid platform '$1'"
        exit
    fi 
fi

echo $PLATFORM_TYPE
echo $PLATFORM_NAME

SDK_PATH_LINE=$(cat Makefile| grep ^SDK_ROOT\ )
SDK_PATH_TEXT=${SDK_PATH_LINE#*=}
SDK_PATH=${SDK_PATH_TEXT/'$(PLATFORM_PATH)'/$PLATFORM_NAME}
SDK_PATCH="/../sdk_patch"
SDK_PATCH_PATH=$SDK_PATH$SDK_PATCH

if [ $PLATFORM_TYPE = "FR8016" ]; then
    cd $SDK_PATCH_PATH && sh sdk_patch.sh
fi

cd $BUILD_PATH
echo "make clean and make platform=$PLATFORM_TYPE"

make clean && make platform=$PLATFORM_TYPE
if [ $? -ne "0" ]; then
    echo "make failed!!! please Check error"
    exit
fi
