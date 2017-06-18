#!/usr/bin/env bash

# This is a simple script to build a firmware and extract the ihex.

oname=$2
iname=$1
mcu='attiny13a'

avr-gcc -Wall -Os -mmcu=${mcu} -o ${oname}.elf ${iname}.c && avr-size -C ${oname}.elf
avr-objcopy -j .text -j .data -O ihex ${oname}.elf ${oname}.hex
