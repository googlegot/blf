#!/usr/bin/env bash

# This is a simple script to build a firmware and extract the ihex.

iname=$1
mcu=$2
oname="${iname}-${mcu}"

if [ "${mcu}" == "attiny13" ]; then
    mcuvar='13'
elif [ "${mcu}" == "attiny85" ]; then
    mcuvar='85'
fi
avr-gcc -Wall -Os -mmcu=${mcu} -D ATTINY=${mcuvar} -o ${oname}.elf ${iname}.c && avr-size -C ${oname}.elf
avr-objcopy -j .text -j .data -O ihex ${oname}.elf ${oname}.hex
