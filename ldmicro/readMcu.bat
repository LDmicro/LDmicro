@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls
if "%1" == "" goto PIC16
if "%1" == "" goto AVR

if "%1" == "AVR" goto AVR
if "%1" == "PIC16" goto PIC16
if "%1" == "" goto pauses
goto NOT_SUPPOTED

@rem =======================================================================
:AVR
::**************************************************************************
::Usage: avrdude.exe [options]
::Options:
::  -p <partno>                Required. Specify AVR device.
::  -b <baudrate>              Override RS-232 baud rate.
::  -B <bitclock>              Specify JTAG/STK500v2 bit clock period (us).
::  -C <config-file>           Specify location of configuration file.
::  -c <programmer>            Specify programmer type.
::  -D                         Disable auto erase for flash memory
::  -i <delay>                 ISP Clock Delay [in microseconds]
::  -P <port>                  Specify connection port.
::  -F                         Override invalid signature check.
::  -e                         Perform a chip erase.
::  -O                         Perform RC oscillator calibration (see AVR053).
::  -U <memtype>:r|w|v:<filename>[:format]
::                             Memory operation specification.
::                             Multiple -U options are allowed, each request
::                             is performed in the order specified.
::  -n                         Do not write anything to the device.
::  -V                         Do not verify.
::  -u                         Disable safemode, default when running from a script.
::  -s                         Silent safemode operation, will not ask you if
::                             fuses should be changed back.
::  -t                         Enter terminal mode.
::  -E <exitspec>[,<exitspec>] List programmer exit specifications.
::  -x <extended_param>        Pass <extended_param> to programmer.
::  -y                         Count # erase cycles in EEPROM.
::  -Y <number>                Initialize erase cycle # in EEPROM.
::  -v                         Verbose output. -v -v for more.
::  -q                         Quell progress output. -q -q for less.
::  -l logfile                 Use logfile rather than stderr for diagnostics.
::  -?                         Display this usage.
::
::avrdude version 6.3, URL: <http://savannah.nongnu.org/projects/avrdude/>
::**************************************************************************
@rem *** Set up avrdude.exe path. ***
@rem     It may be:
@rem SET AVRDUDE_PATH=D:\WinAVR\bin\
@rem SET AVRDUDE_PATH=D:\Arduino\hardware\tools\avr\bin\
@rem SET AVRDUDE_PATH=D:\Program\Electronique\Ldmicro\
@rem SET AVRDUDE_PATH=D:\AVRDUDE\BIN\
     SET AVRDUDE_PATH=C:\Users\nii\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9\bin\

@rem *** Set up your avrdude config file. ***
@rem SET AVRDUDE_CONF=
@rem SET AVRDUDE_CONF=-C %AVRDUDE_PATH%avrdude.conf
     SET AVRDUDE_CONF=-C %AVRDUDE_PATH%..\etc\avrdude.conf

@rem *** Set up your hardware avrdude programmer. ***
@rem   See avrdude.conf programmer id.
@rem Duemilanove/Nano(ATmega328)    is stk500   at 57600  baud (-b  57600 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Duemilanove/Nano(ATmega168)    is stk500   at 19200  baud (-b  19200 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Uno(ATmega328)                 is stk500   at 115200 baud (-b 115200 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Mega(ATMEGA1280)               is stk500   at 57600  baud (-b  57600 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Mega(ATMEGA2560)               is stk500v2 at 115200 baud (-b 115200 otion for %AVRDUDE_PATH%avrdude.exe)
@rem "wiring" is Basically STK500v2 protocol, with some glue to trigger the arduino bootloader.

@rem SET AVRDUDE_PROGRAMMER_ID=dapa
@rem SET AVRDUDE_PROGRAMMER_ID=stk500
@rem SET AVRDUDE_PROGRAMMER_ID=stk200s5
@rem SET AVRDUDE_PROGRAMMER_ID=stk500v2 -P COM9 -b 115200
     SET AVRDUDE_PROGRAMMER_ID=wiring -P COM9 -b 115200

@rem *** Set up your avrdude Atmel Microcontroller. ***
@rem   See avrdude.conf part id.
@rem ATmega8                        is m8
@rem ATmega328P                     is m328p
@rem ATmega32U4                     is m32u4
@rem ATmega2560                     is m2560
@rem ATmega32u4                     is m32u4
@rem
@rem Duemilanove/Nano(ATmega328)    is m328p
@rem Duemilanove/Nano(ATmega168)    is m168
@rem Uno(ATmega328)                 is m328p
@rem Mega(ATMEGA1280)               is m1280
@rem Mega(ATMEGA2560)               is m2560
@rem Leonardo                       is m32u4
@rem
@rem SET AVRDUDE_PART_ID=m8
@rem SET AVRDUDE_PART_ID=m328p
@rem SET AVRDUDE_PART_ID=m32u4
     SET AVRDUDE_PART_ID=m2560

@rem Set up your avrdude options
@rem SET AVRDUDE_OPTIONS=-n -D -v -v -l readMcu.log
     SET AVRDUDE_OPTIONS=-n -D -v

@rem *** Read signature. ***
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U signature:r:%2_signature.hex:r
@echo ERRORLEVEL=%ERRORLEVEL%
if ERRORLEVEL==1 goto pauses

@rem *** Read flash. ***
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:r:%2_flash.hex:i
@rem *** Read eeprom. ***
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:%2_eeprom.hex:r
@rem *** Read fuses. ***
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U fuse:r:%2_fuse.hex:r
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U lfuse:r:%2_lfuse.hex:r
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U hfuse:r:%2_hfuse.hex:r
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U efuse:r:%2_efuse.hex:r
@rem *** Read calibration. ***
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U calibration:r:%2_calibration.hex:r
@rem *** Read lock. ***
%AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U lock:r:%2_lock.hex:r
goto exit

@rem =======================================================================
:PIC16
@rem *** Set up PIC reading tool. ***
@rem SET TOOL="d:\Program Files\Microchip\PICkit 3 v1\PICkit 3.exe"
     SET TOOL="d:\Program Files\Microchip\MPLAB IDE\Programmer Utilities\PICkit3\PK3CMD.exe"

@rem *** Part Selection. ***
@rem Part name should be mentioned without Family Information like PIC/dsPIC
@rem Example: PIC16F628 should be mentioned as 16F628.
@rem SET PART=-P16F628
     SET PART=-P16F877A

@rem *** Set up additional options of TOOL program. ***
     SET OPTIONS=-V5.00

@rem *** Read PROGRAM Memory ***
%TOOL% %PART% %OPTIONS% -GPF%2_program.hex ?E

@rem *** Read CONFIG Memory ***
%TOOL% %PART% %OPTIONS% -GCF%2_config.hex

@rem *** Read EEPROM Memory ***
%TOOL% %PART% %OPTIONS% -GEF%2_eeprom.hex

@rem *** Read USER ID Memory ***
%TOOL% %PART% %OPTIONS% -GIF%2_user_id.hex

@rem *** Read BOOT Memory ***
%TOOL% %PART% %OPTIONS% -GBF%2_boot.hex

@echo
@echo You can write own command for read PIC's.
pause
goto exit

@rem =======================================================================
:NOT_SUPPOTED
@echo You can write own command for '%1'.

@rem =======================================================================
:pauses
@echo USE:
@echo "readMcu.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR"
pause

:exit
::pause
