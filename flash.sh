#!/usr/bin/env bash
file=$1
pins="9 10 11 25"
for i in ${pins}; do
    echo $i > /sys/class/gpio/unexport
done

while true; do
    avrdude -s -p attiny13 -c linuxgpio\
        -U flash:w:${file}:a\
        -U lfuse:w:0x75:m\
        -U hfuse:w:0xfd:m\
        && break
done
