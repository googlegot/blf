 ____  _     _____      _    __         
| __ )| |   |  ___|    / \  / /_    _   
|  _ \| |   | |_      / _ \| '_ \ _| |_ 
| |_) | |___|  _|    / ___ \ (_) |_   _|
|____/|_____|_|     /_/   \_\___/  |_|  
--------------------------------------------
README
--------------------------------------------
This firmware adds several key items to seriously extend the
functionality of the attiny13a-based BLF-A6 driver, and compatibles.
This repo will contain up-to-date attiny13/25/45/85 firmwares in both
.elf and .hex formats.

--------------------------------------------
NEW FEATURES
--------------------------------------------
- Moon mode disable (config option 3)
- Muggle mode (config option 1)
- Mode order reversal (config option 4)
- Medium press disablement (config option 6)
- Mode 'lock' (config option 7)
- Config reset (config option 8)
- Bistro-style config mode
- SOS mode
- Beacon mode
                                      
--------------------------------------------
USAGE
--------------------------------------------

Glossary:
---------
Short Press - A short tap that quickly breaks the circuit for <0.5 seconds
Medium Press - A middling tap that breaks the circuit for 1-1.5 seconds
Long Press - A long press that breaks the circuit for >3 seconds

Functionality:
--------------
Short press to advance to the next mode.

Medium press (if enabled) to reverse to the previous mode.  If you are at 
the first mode, you will enter the "Hidden" modes (see "Hidden Modes").

Long press to reset to the first mode, unless mode memory is enabled. If
mode memory is enabled, a long press will return to the last mode you were
at.

Normal Modes:
-------------
1. Mode group 1 
 - 8 modes, moon to turbo, visually linear

2. Mode group 2 
 - 4 modes, moon to high, visally jumps from moon to medium,
then linear to one step below turbo. (modes 1,3,5,7 from mode group 1)

Hidden modes:
-------------
Hidden modes cannot be accessed without medium press enabled.

To access hidden modes, medium press at 'moon', the dimmest normal mode.

Hidden modes can only be cycled through with a medium press, a short press
will bring you back to the first mode of the mode group you are in.

Hidden modes are always in this order:

1. Battery Check (1 flash per 25%, 5 for 100%)
2. Turbo
3. Strobe 
4. Biking Strobe (High, flashing 4 times quickly once every second)
5. SOS ( Flash out ...---...)
6. Beacon (1 flash every 2.5 seconds)

Config Mode:
------------
Short press 15 times in quick succession to get to config mode.

Config mode will flash for the config option number, wait a second, then
'buzz' (a fast, dim strobe) for ~1.5 seconds.  To set the config option, short/medium/long press.

Config options:
---------------
1. Muggle mode
- This mode disables medium press, mode order reversal, and reduces the maximum output to brightness level 6 (or 3 in mode group 2)

2. Mode Memory
- When set, the light will remember your the last mode you were in.

3. Moon Mode Disable
- When set, this disables the lowest output mode (moon mode)

4. Mode Order
- This selects the order in which you advance through the modes (bright to dim, or dim to bright)

5. Mode group
- This alternates between normal mode groups listed above.

6. Medium Press Disable
- This disables medium press functionality.  This is useful for beginners,
since medium press can be difficult to master at first.

7. Mode Locking
- This 'locks' a mode if you stay in that mode for >3 seconds.  Short presses will
not change your mode.  To unlock, long press.

This is made for biking, if you hit a bump and it disconnects the battery for a
short time, you can jump to the next mode.  In biking strobe, this means you will
drop to moon, which is a worst-case scenerio and very dangerous. USE MODE LOCKING
WHEN USING YOUR FLASHLIGHT AS YOUR BIKE LIGHT.

Mode locking only works for normal modes and biking_strobe.

8. Reset
- Reset wipes your config settings, back to the default of mode group 2 (4 modes) with med press disabled.
