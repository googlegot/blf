#!/usr/bin/env bash

# This is a simple script for linux userspace gpio-based flashing of atmel mcu devices. 
# This particular version flashes via a cheap and ubiquitous usbasp SPI programmer.

file=$1
pins="9 10 11 25"
mcu=$2

cmd="avrdude -s -p ${mcu} -c usbasp -F -U flash:w:${file}:a"

if [ "${mcu}" == "attiny13" ]; then
    lfuse='0x75'
    hfuse='0xfd'
    cmd="${cmd} -U lfuse:w:${lfuse}:m -U hfuse:w:${hfuse}:m"
elif [ "${mcu}" == "attiny85" ] || [ "${mcu}" == "attiny25" ]; then
    lfuse='0xd2'
    hfuse='0xde'
    efuse='0xff'
    cmd="${cmd} -U lfuse:w:${lfuse}:m -U hfuse:w:${hfuse}:m -U efuse:w:${efuse}:m"
fi

${cmd}
