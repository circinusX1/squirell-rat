#!/bin/bash

#
# this file might need tweaks
#

export BOARD_SETUP=done
export ARM_EMBEDDED_SYSROOT=/home/$USER/BOARDS/bbb/debfs
export PATH=$PATH:/home/$USER/BOARDS/bbb/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf/bin


export ARM_EMBEDDED_INCLUDE=$ARM_EMBEDDED_SYSROOT/usr/include
export ARM_EMBEDDED_INCLUDE_EABI=$ARM_EMBEDDED_SYSROOT/usr/include/arm-linux-gnueabihf
export ARM_EMBEDDED_LIB_EABI=$ARM_EMBEDDED_SYSROOT/usr/lib/arm-linux-gnueabihf/

export GCC_PREFIX=arm-linux-gnueabihf-
export GPP_PREFIX=arm-linux-gnueabihf-
export CROSS_COMPILE=${GCC_PREFIX}
export ARCH=arm
