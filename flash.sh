#!/usr/bin/env bash

# This is a simple script for linux userspace gpio-based flashing of atmel mcu devices. 

file=$1
pins="9 10 11 25"
mcu='attiny13'
lfuse='0x75'
hfuse='0xfd'

for i in ${pins}; do
    echo $i > /sys/class/gpio/unexport
done

while true; do
    avrdude -s -p ${mcu} -c linuxgpio\
        -U flash:w:${file}:a\
        -U lfuse:w:${lfuse}:m\
        -U hfuse:w:${hfuse}:m\
        && break
done
