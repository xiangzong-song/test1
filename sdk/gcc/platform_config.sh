#! /bin/bash

if [ $1 = "FR8016" ]; then
    sed -i 's/ID_DEFAULT/PLATFORM_FR8016HA/g' ../code/sdk/system/platform.h
    sed -i 's/SIZE_DEFAULT/0x3A000/g' ../code/sdk/system/platform.h
elif [ $1 = "FR5089" ]; then
    sed -i 's/ID_DEFAULT/PLATFORM_FR5089D2/g' ../code/sdk/system/platform.h
    sed -i 's/SIZE_DEFAULT/0x3A000/g' ../code/sdk/system/platform.h    
else
    echo "Error : invalid platform '$1'"
    exit
fi 