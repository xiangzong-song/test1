#!/bin/sh
#copy ota file sdk
prj_path=$(pwd)

frq_sdk_path_line=$(cat Makefile| grep ^SDK_ROOT\ )
frq_sdk_path_=${frq_sdk_path_line#*=}
frq_sdk_path_=`eval echo $frq_sdk_path_|tr -d '\\r'`
frq_sdk_path=`eval echo $frq_sdk_path_`

frq_sdk_patch="/../sdk_patch"
echo "FREQ_SDK_PATH  = $frq_sdk_path"
echo "FREQ_SDK_PATCH = $frq_sdk_path$frq_sdk_patch"
cd $frq_sdk_path$frq_sdk_patch && sh sdk_patch.sh
cd $prj_path
echo "make clean and make"
make clean && make
if [ $? -ne "0" ]; then
    echo "make failed!!! please Check error"
    exit
fi
