#!/usr/bin/env bash

# This is a simple script to build a firmware and extract the ihex.

iname=$1
mcu=$2
oname="${iname}-${mcu}"

mcuvar=$(echo ${mcu} | egrep -o '[0-9]{1,3}')

avr-gcc -Wall -Os -mmcu=${mcu} -D ATTINY=${mcuvar} -o ${oname}.elf ${iname}.c && avr-size -C ${oname}.elf
avr-objcopy -j .text -j .data -O ihex ${oname}.elf ${oname}.hex
