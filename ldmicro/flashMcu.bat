@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls
REM EXE_PATH from where the ldmicro.exe and *.bat are executes
SET EXE_PATH=%~dp0

@echo %EXE_PATH% = EXE_PATH
@echo %1 = ISA
@echo %2 = full filename '.ld' with the path
@SET  COMPILER=%3
@echo %COMPILER% = COMPILER
@echo %4 = deviceName, target name
@echo . . .
:@echo %2 - full ld file name wit the path
@echo %~nx2 - gives the file name without the path
@echo %~d2 - gives the drive letter to LD
@echo %~p2 - gives the path to LD
@echo . . .

:pause

if "%COMPILER%" == "HI_TECH_C" goto HTC
if "%1" == "PIC16" goto PIC

if "%COMPILER%" == "AVRGCC" goto AVRGCC
if "%1" == "AVR" goto AVR

if "%COMPILER%" == "ARMGCC" goto ARMGCC
if "%1" == "ARM" goto ARMGCC

if "%1" == "" goto pauses

goto NOT_SUPPORTED

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
@rem SET AVRDUDE_PATH=C:\Users\nii\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9\bin\
     SET AVRDUDE_PATH=%ProgramFiles%\Arduino\hardware\tools\avr\bin\

@rem *** Set up your avrdude config file. ***
@rem SET AVRDUDE_CONF=
@rem SET AVRDUDE_CONF=%AVRDUDE_PATH%avrdude.conf
     SET AVRDUDE_CONF=%AVRDUDE_PATH%..\etc\avrdude.conf
@rem SET AVRDUDE_CONF=%ProgramFiles%\Arduino\hardware\tools\avr\\etc\avrdude.conf

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
@rem "%AVRDUDE_PATH%avrdude.exe" -C "%AVRDUDE_CONF%" %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a
@rem "%AVRDUDE_PATH%avrdude.exe" -C "%AVRDUDE_CONF%" %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a
     "%AVRDUDE_PATH%avrdude.exe" -C "%AVRDUDE_CONF%" %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a

@echo ERRORLEVEL=%ERRORLEVEL%
if ERRORLEVEL==1 goto pauses

@rem *** Read eeprom after flashing. ***
rem %AVRDUDE_PATH%avrdude.exe %AVRDUDE_CONF% %AVRDUDE_OPTIONS% -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:eeprom_read2:r
goto exit


@rem =======================================================================
:AVRGCC
::**************************************************************************
@ECHO ON
REM Compilation with avr-gcc

@rem SET GCC_PATH=C:\Program Files\Atmel\AVR-Gcc-8.2.0
@rem SET GCC_PATH=C:\Program Files\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain
@rem SET GCC_PATH=D:\WinAVR
     SET GCC_PATH=c:\WinAVR-20100110

     SET AVRDUDE_PATH=D:\Programmation\Ladder\Programmes\Tests\Avr\AvrDude
@rem SET AVRDUDE_PATH=D:\AVRDUDE
@rem SET AVRDUDE_PATH=%ProgramFiles%\Arduino\hardware\tools\avr\bin\

SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\AVR
SET COMPORT=COM3

path %GCC_PATH%\BIN;%AVRDUDE_PATH%\BIN;%path%

%~d2
chdir %~p2

REM Compilation of sources
rmdir AVRGCC\obj /s /q
rmdir AVRGCC\bin /s /q
mkdir AVRGCC\obj
mkdir AVRGCC\bin
mkdir AVRGCC\lib

if not exist AVRGCC\lib\UsrLib.c copy %LIB_PATH%\*.* AVRGCC\lib

for %%F in (AVRGCC\lib\*.c) do avr-gcc.exe -I%~dp2 -IAVRGCC\lib\ -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -MD -MP -mmcu=%4 -MF AVRGCC\obj\%%~nF.d -MT AVRGCC\obj\%%~nF.d -MT AVRGCC\obj\%%~nF.o %%F -o AVRGCC\obj\%%~nF.o

avr-gcc.exe -IAVRGCC\lib\ -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -c -std=gnu99 -MD -MP -mmcu=%4 -MF AVRGCC\obj\%~nx2.d -MT AVRGCC\obj\%~nx2.d -MT AVRGCC\obj\%~nx2.o %~f2.c -o AVRGCC\obj\%~nx2.o

REM Linkage of objects
avr-gcc.exe -o AVRGCC\bin\%~nx2.elf AVRGCC\obj\*.o -Wl,-Map=AVRGCC\obj\%~nx2.map -Wl,--start-group -Wl,-lm -Wl,--end-group -mmcu=%4

REM Convert Elf to Hex
avr-objcopy.exe -O ihex -R .eeprom -R .fuse -R .lock -R .signature AVRGCC\bin\%~nx2.elf AVRGCC\bin\%~nx2.hex

REM Transfer of the program with AvrDude
:avrdude.exe -p %4 -c avr910 -P %COMPORT% -b 19200 -u -v -F -U flash:w:AVRGCC\bin\%~nx2.hex

pause
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
@rem SET TOOL="C:\Program Files (x86)\Microchip\PICkit 2 v2\PICkit2V2.exe"
@rem SET TOOL="C:\Program Files (x86)\Microchip\PICkit 3 v3\PICkit3.exe"
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
REM Compilation with HI-TECH C (picc.exe)

@rem SET PCC_PATH="C:\Program Files (x86)\HI-TECH Software\PICC\9.82"
SET PCC_PATH=C:\Program Files\HI-TECH Software\PICC\9.81

SET PICKIT_PATH=C:\Program Files\Microchip\MPLAB IDE\Programmer Utilities\PICkit3

SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\PIC16

path %path%;%PCC_PATH%\bin;%PICKIT_PATH%

%~d2
chdir %~p2

REM Compilation of sources
rmdir HTC\obj /s /q
rmdir HTC\bin /s /q
mkdir HTC\obj
mkdir HTC\bin
mkdir HTC\lib

if not exist HTC\lib\UsrLib.c copy %LIB_PATH%\*.* HTC\lib
:if not exist         UsrLib.c copy %LIB_PATH%\*.* .

:copy *.h PROTEUS
:copy *.c PROTEUS

for %%F in (HTC\lib\*.c) do  picc.exe --pass1 %%F -q --chip=%4 -P -I%~p2 -I%~p2\HTC\lib --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj
:for %%F in (*.c) do  picc.exe --pass1 %%F -q --chip=%4 -P -I%~p2 -I%~p2 --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj

picc.exe --pass1 %~nx2.c -q --chip=%4 -P --runtime=default -IHTC\lib --opt=default -g --asmlist --OBJDIR=HTC\obj

REM Linkage of objects
picc.exe -oHTC\bin\%~nx2.cof -mHTC\bin\%~nx2.map --summary=default --output=default HTC\obj\*.p1 --chip=%4 -P --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj --OUTDIR=HTC\bin

REM Transfer of the program with Pickit3
PK3CMD.exe -P%4A -FHTC\bin\%~nx2.hex -E -L -M -Y

pause
goto exit


@rem =======================================================================
:ARMGCC
::**************************************************************************
@ECHO ON
REM Compilation with arm-gcc

SET GCC_PATH=C:\Program Files\EmIDE\emIDE V2.20\arm
SET JLN_PATH=C:\Program Files\SEGGER\JLink_V502j
SET STL_PATH=C:\Program Files\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility

if "%4" == "stm32f40x" SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\ARM\STM32F4
if "%4" == "stm32f10x" SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\ARM\STM32F1

path %path%;%GCC_PATH%\bin;%JLN_PATH%;%STL_PATH%

%~d2
chdir %~p2

REM Compilation of sources
rmdir ARMGCC\obj /s /q
rmdir ARMGCC\bin /s /q
mkdir ARMGCC\obj
mkdir ARMGCC\bin
mkdir ARMGCC\lib

if not exist ARMGCC\lib\Lib_usr.c copy %LIB_PATH%\*.* ARMGCC\lib

if "%4" == "stm32f10x" goto STM32F1

:STM32F4

arm-none-eabi-g++.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm-none-eabi\include" -c ARMGCC\lib\CortexM4.S -o ARMGCC\obj\cortexM4.o

CD ARMGCC\lib
for %%F in (*.c) do arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..\..

arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -IARMGCC\lib\ -c %~n2.c -o ARMGCC\obj\%~n2.o

REM Linkage of objects
arm-none-eabi-gcc.exe -o ARMGCC\bin\%~nx2.elf ARMGCC\obj\*.o -Wl,-Map -Wl,ARMGCC\bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m4 -mthumb -TARMGCC\lib\CortexM4.ln

REM Convert Elf to Hex
arm-none-eabi-objcopy -O ihex ARMGCC\bin\%~nx2.elf ARMGCC\bin\%~nx2.hex

REM Creation of the J-Link script

@ECHO r > ARMGCC\bin\cmdfile.jlink
@ECHO loadfile ARMGCC\bin\%~nx2.hex >> ARMGCC\bin\cmdfile.jlink
@ECHO go >> ARMGCC\bin\cmdfile.jlink
@ECHO exit >> ARMGCC\bin\cmdfile.jlink

REM Transfer of the program with J-Link Commander
JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript ARMGCC\bin\cmdfile.jlink

JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript ARMGCC\bin\cmdfile.jlink
pause
goto exit

:STM32F1

arm-none-eabi-g++.exe -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm-none-eabi\include" -c ARMGCC\lib\CortexM3.S -o ARMGCC\obj\cortexM3.o

CD ARMGCC\lib
for %%F in (*.c) do arm-none-eabi-gcc.exe -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..\..

arm-none-eabi-gcc.exe -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -IARMGCC\lib\ -c %~n2.c -o ARMGCC\obj\%~n2.o

REM Linkage of objects
arm-none-eabi-gcc.exe -o ARMGCC\bin\%~nx2.elf ARMGCC\obj\*.o -Wl,-Map -Wl,ARMGCC\bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m3 -mthumb -TARMGCC\lib\CortexM3.ln

REM Convert Elf to Hex
arm-none-eabi-objcopy -O ihex ARMGCC\bin\%~nx2.elf ARMGCC\bin\%~nx2.hex

REM Transfer of the program with ST-Link CLI

ST-LINK_CLI.exe -c SWD -P ARMGCC\bin\%~nx2.hex -V "after_programming" -Run

pause
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
