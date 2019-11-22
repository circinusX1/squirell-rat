#!/bin/bash
#
# this file might need tweaks
#
export BOARD_SETUP=done
export ARM_EMBEDDED_SYSROOT=/BOARDS/rpi/ROOTFS
export PATH=$PATH:/home/$USER/BOARDS/rpi/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin

export ARM_EMBEDDED_INCLUDE=${ARM_EMBEDDED_SYSROOT}/usr/include
export ARM_EMBEDDED_INCLUDE_EABI=${ARM_EMBEDDED_SYSROOT}/usr/include/arm-linux-gnueabihf
export ARM_EMBEDDED_LIB_EABI=${ARM_EMBEDDED_SYSROOT}/usr/lib/arm-linux-gnueabihf/

export GCC_PREFIX=arm-bcm2708hardfp-linux-gnueabi-
export GPP_PREFIX=arm-bcm2708hardfp-linux-gnueabi-
export CROSS_COMPILE=${GCC_PREFIX}
export ARCH=arm

