@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls

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
     SET AVRDUDE_PATH=D:\Arduino\hardware\tools\avr\bin\
@rem SET AVRDUDE_PATH=D:\Program\Electronique\Ldmicro\
@rem SET AVRDUDE_PATH=D:\AVRDUDE\BIN\
@rem SET AVRDUDE_PATH=C:\Users\nii\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9\bin\
@rem SET AVRDUDE_PATH="%ProgramFiles%\Arduino\hardware\tools\avr\bin\"

@rem *** Set up your avrdude config file. ***
@rem SET AVRDUDE_CONF=
@rem SET AVRDUDE_CONF=-C %AVRDUDE_PATH%avrdude.conf
     SET AVRDUDE_CONF=-C %AVRDUDE_PATH%..\etc\avrdude.conf
@rem SET AVRDUDE_CONF=-C "%ProgramFiles%\Arduino\hardware\tools\avr\\etc\avrdude.conf"

@rem *** Set up your hardware avrdude programmer. ***
@rem     See avrdude.conf programmer id.
@rem Duemilanove/Nano(ATmega328)    is stk500   at 57600  baud rate
@rem Duemilanove/Nano(ATmega168)    is stk500   at 19200  baud rate
@rem Uno(ATmega328)                 is stk500   at 115200 baud rate
@rem Mega(ATMEGA1280)               is stk500   at 57600  baud rate
@rem Mega(ATMEGA2560)               is stk500v2 at 115200 baud rate
@rem "wiring" is Basically STK500v2 protocol, with some glue to trigger the arduino bootloader.
@rem
@rem Additionally, you can set the serial port number and baud rate.
@rem   -P <port>       Set the serial com port for the Arduino bootloader.
@rem   -b <baudrate>   Set the baud rate for programmers.
@rem Modify this:                       COMX    BAUD
@rem SET AVRDUDE_PROGRAMMER_ID=dapa
@rem SET AVRDUDE_PROGRAMMER_ID=stk500
@rem SET AVRDUDE_PROGRAMMER_ID=stk200s5
@rem SET AVRDUDE_PROGRAMMER_ID=stk500v2 -P COM9 -b 115200
@rem SET AVRDUDE_PROGRAMMER_ID=wiring -P COM3 -b 115200
     SET AVRDUDE_PROGRAMMER_ID=wiring -P COM15 -b 115200

@rem *** Set up your avrdude Atmel Microcontroller. ***
@rem     See avrdude.conf part id.
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

@rem *** Set up your avrdude options ***
@rem SET AVRDUDE_OPTIONS=-y -D -v -E noreset, vcc
@rem SET AVRDUDE_OPTIONS=-y -D -v -v -l readMcu.log
     SET AVRDUDE_OPTIONS=-D

@rem *** Read eeprom before flashing. ***
rem %AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:eeprom_read1:r

@rem *** Flashing the AVR. ***
@rem %AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a
@rem %AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a
     %AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a

@echo ERRORLEVEL=%ERRORLEVEL%
if ERRORLEVEL==1 goto pauses

@rem *** Read eeprom after flashing. ***
rem %AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:eeprom_read2:r
goto exit

@rem =======================================================================
:PIC16
::**************************************************************************
::Here is a part of 'Readme for PK3CMD.txt' file:
::Release Notes for PICkit 3 Command Line Interface
::
::10. Command Line Options
::--------------------------------------------------------------------------
::The following commands are available in the command line interface.
:: Description          Options
::--------------------------------------------------------------------------
::B                     Batch Mode Operation
::C                     Blank Check Device
::E                     Erase Flash Device
::PKOB                  To connect PKOB
::F<file>               Hex File Selection
::G<region><Type><path> Read functions
::                         region:
::                         E = EEPROM
::                         I = ID memory
::                         C = Configuration memory
::                         B = Boot Flash Memory
::                         If no region is entered, the entire
::                         device will be read.
::
::                         Type F: = read into hex file, Path = full file path
::H                     Hold In Reset
::L                     Release from Reset
::I                     Use High Voltage MCLR
::M<region>             Program Device
::                         region:
::                         P = Program memory
::                         E = EEPROM
::                         I = ID memory
::                         C = Configuration memory
::                         B = Boot Flash Memory
::                         If no region is entered, the entire
::                         device will be programmed.
::N####,####            Program Memory Range
::                         #### is a hexidecimal number representing
::                         Start and End Address in sequence
::P<part>               Part Selection. Example: 18F67J50
::R<file>               Reporgram
::S<file>               SQTP File Selection
::U##                   Program calibration memory,where:
::                         ## is a hexidecimal number representing
::                         the least significant byte of the
::                         calibration value to be programmed
::V<Voltage>            Power Target from PICkit3
::Y                     Verify Device against selected HEX File
::?                     Help Screen
::?E                    Displays Exit Code
::
::Note: Commands are not case sensitive. Escape character can be a  -  or /
::**************************************************************************

@rem *** Set up PIC flashing tool. ***
@rem SET TOOL="d:\Program Files\Microchip\PICkit 3 v1\PICkit 3.exe"
     SET TOOL="d:\Program Files\Microchip\MPLAB IDE\Programmer Utilities\PICkit3\PK3CMD.exe"

@rem *** Part Selection. ***
@rem Part name should be mentioned without Family Information like PIC/dsPIC.
@rem Example: PIC16F628 should be mentioned as -P16F628.
@rem SET PART=-P16F628
     SET PART=-P16F877A

@rem *** Set up additional options of TOOL program. ***
@rem SET OPTIONS=-L -Y -V5.00
     SET OPTIONS=-L -Y

@rem *** Program Entire Device ***
%TOOL% %PART% %OPTIONS% -F"%2.hex" -M ?E
@echo .
@echo ERRORLEVEL=%ERRORLEVEL%
if ERRORLEVEL==1 pause
goto exit

@echo .
@echo You can write own command for flash PIC.
pause
goto exit

@rem =======================================================================
:NOT_SUPPOTED
@echo You can write own command for '%1'.

@rem =======================================================================
:pauses
@echo USE:
@echo "flashMcu.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR hex_file_name"
pause

:exit
::pause
