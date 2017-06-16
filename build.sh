avr-gcc -Wall -Os -mmcu=attiny13a -o blf-a6-rmm-tiny13a.elf blf-a6-rmm.c && avr-size -C blf-a6-rmm-tiny13a.elf
avr-objcopy -j .text -j .data -O ihex blf-a6-rmm-tiny13a.elf blf-a6-rmm-tiny13a.hex
