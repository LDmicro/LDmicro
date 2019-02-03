@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls

REM %1 = ISA
REM %2 = filename
REM %3 = variant (compiler)
REM %4 = target name

if "%1" == "PIC16" goto PICX
if "%1" == "ARM" goto ARM
if "%1" == "AVR" goto AVRX

if "%1" == "" goto pauses

goto NOT_SUPPORTED

:PICX
if "%3" == "2" goto HTC
goto PIC16)

:AVRX
if "%3" == "2" goto AVRGCC
goto AVRC


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
:AVRGCC
::**************************************************************************
@ECHO ON

REM Compilation avec avr-gcc

SET GCCPATH=C:\Program Files\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain
SET DUDPATH=D:\Programmation\Ladder\Programmes\Tests\AvrGcc\AvrDude
SET COMPORT=COM3

path %path%;%GCCPATH%\bin
path %path%;%DUDPATH%

@REM %~nx2 donne le nom de fichier dans %2 sans le path

REM Compilation des sources
rmdir obj /s /q
rmdir bin /s /q
mkdir obj
mkdir bin

CD lib
for %%F in (*.c) do  avr-gcc.exe -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -MD -MP -mmcu=%4 -MF ..\obj\%%F.d -MT ..\obj\%%F.d -MT ..\obj\%%F.o %%F -o ..\obj\%%F.o 
CD ..

avr-gcc.exe -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -c -std=gnu99 -MD -MP -mmcu=%4 -MF obj\%~nx2.d -MT obj\%~nx2.d -MT obj\%~nx2.o %~nx2.c -o obj\%~nx2.o 

REM Linkage des objets
avr-gcc.exe -o bin\%~nx2.elf obj\*.o -Wl,-Map=obj\%~nx2.map -Wl,--start-group -Wl,-lm -Wl,--end-group -mmcu=%4 

REM Conversion Elf en Hex
avr-objcopy.exe -O ihex -R .eeprom -R .fuse -R .lock -R .signature bin\%~nx2.elf bin\%~nx2.hex

REM Transfert du programme avec AvrDude
avrdude.exe -p %4 -c avr910 -P %COMPORT% -b 19200 -u -v -F -U flash:w:bin\%~nx2.hex

PAUSE
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
     SET OPTIONS=-L

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
:HTC
::**************************************************************************
@ECHO ON
REM Compilation avec HiTech-c (Picc)

SET PCCPATH=C:\Program Files\HI-TECH Software\PICC\9.81
path %path%;%PCCPATH%\bin

@REM %~nx2 donne le nom de fichier dans %2 sans le path

REM Compilation des sources
rmdir obj /s /q
rmdir bin /s /q
mkdir obj
mkdir bin

CD lib
for %%F in (*.c) do  picc.exe --pass1 %%F -q --chip=%4 -P --runtime=default --opt=default -g --asmlist --OBJDIR=../obj
CD ..

picc.exe --pass1 %~nx2.c -q --chip=%4 -P --runtime=default --opt=default  -g --asmlist --OBJDIR=obj

REM Linkage des objets
picc.exe -obin\%~nx2.cof -mbin\%~nx2.map --summary=default --output=default obj/*.p1 --chip=%4 -P --runtime=default --opt=default -g --asmlist --OBJDIR=obj --OUTDIR=bin

REM Conversion Elf en Hex




REM Transfert du programme 




PAUSE
goto exit


@rem =======================================================================
:ARM
::**************************************************************************
@ECHO ON
REM Compilation avec arm-gcc

SET GCCPATH=C:\Program Files\EmIDE\emIDE V2.20\arm
SET JLNPATH=C:\Program Files\SEGGER\JLink_V502j

path %path%;%GCCPATH%\bin;%JLNPATH%

@REM %~nx2 donne le nom de fichier dans %2 sans le path

REM Compilation des sources
rmdir obj /s /q
rmdir bin /s /q
mkdir obj
mkdir bin

arm-none-eabi-g++.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCCPATH%\arm-none-eabi\include" -c lib\CortexM4.S -o obj\cortexM4.o

CD lib
for %%F in (*.c) do arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCCPATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..

arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCCPATH%\arm\arm-none-eabi\include" -c %~n2.c -o obj\%~n2.o

REM Linkage des objets
arm-none-eabi-gcc.exe -o bin\%~nx2.elf obj\*.o -Wl,-Map -Wl,bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m4 -mthumb -Tlib\CortexM4.ln

REM Conversion Elf en Hex
arm-none-eabi-objcopy -O ihex bin\%~nx2.elf bin\%~nx2.hex

REM Creation du script jlink

@ECHO r > bin\cmdfile.jlink
@ECHO loadfile bin\%~nx2.hex >> bin\cmdfile.jlink
@ECHO go >> bin\cmdfile.jlink
@ECHO exit >> bin\cmdfile.jlink

REM Transfert du programme avec J-Link Commander
JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript bin\cmdfile.jlink

JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript bin\cmdfile.jlink
PAUSE
goto exit

@rem =======================================================================
:NOT_SUPPORTED
@echo You can write own command for '%1'.

@rem =======================================================================
:pauses
@echo USE:
@echo "flashMcu.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR hex_file_name"
pause

:exit
::pause
