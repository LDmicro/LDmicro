@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls

if "%1" == "AVR" goto AVR
if "%1" == "PIC16" goto PIC16
if "%1" == "" goto pauses
goto NOT_SUPPOTED

:AVR
@rem *** Set up avrdude.exe path. ***
@rem     It may be:
@rem SET AVRDUDE_PATH=D:\WinAVR\bin\
     SET AVRDUDE_PATH=D:\Arduino\hardware\tools\avr\bin\
@rem SET AVRDUDE_PATH=D:\Program\Electronique\Ldmicro\

@rem *** Set up your hardware avrdude programmer. ***
@rem     See avrdude.conf programmer id.
@rem Duemilanove/Nano(ATmega328)    is stk500   at 57600  baud (-b  57600 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Duemilanove/Nano(ATmega168)    is stk500   at 19200  baud (-b  19200 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Uno(ATmega328)                 is stk500   at 115200 baud (-b 115200 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Mega(ATMEGA1280)               is stk500   at 57600  baud (-b  57600 otion for %AVRDUDE_PATH%avrdude.exe)
@rem Mega(ATMEGA2560)               is stk500v2 at 115200 baud (-b 115200 otion for %AVRDUDE_PATH%avrdude.exe)
@rem SET AVRDUDE_PROGRAMMER_ID=stk500
@rem SET AVRDUDE_PROGRAMMER_ID=stk200s5
     SET AVRDUDE_PROGRAMMER_ID=dapa

@rem *** Set up your avrdude Atmel Microcontroller. ***
@rem     See avrdude.conf part id.
@rem ATmega8                        is m8
@rem ATmega328P                     is m328p
@rem ATmega32U4                     is m32u4
@rem
@rem Duemilanove/Nano(ATmega328)    is m328p
@rem Duemilanove/Nano(ATmega168)    is m168
@rem Uno(ATmega328)                 is m328p
@rem Mega(ATMEGA1280)               is m1280
@rem Mega(ATMEGA2560)               is atmega2560
SET AVRDUDE_PART_ID=m32u4

@rem *** Read eeprom before flashing. ***
rem %AVRDUDE_PATH%avrdude.exe -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:eeprom_read1:r

@rem *** Flashing the AVR. ***
@rem set the -P <port>       Set the serial com port for the arduino ex -P COM3
@rem set the -b <baudrate>   baud rate for programmers (see the Set up your hardware avrdude programmer).
@rem Modify this                                                                 COMX    BAUD
@rem %AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a -E noreset, novcc
@rem %AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -P COM4 -b 57600 -U flash:w:"%2.hex":a
     %AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a -E noreset, vcc

@echo ERRORLEVEL=%ERRORLEVEL%
if ERRORLEVEL==1 goto pauses

@rem read eeprom after flashing.
rem %AVRDUDE_PATH%avrdude.exe -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:eeprom_read2:r

goto exit

:PIC16
@echo You can write own command for flash PIC.
pause
goto exit

:NOT_SUPPOTED
@echo You can write own command for '%1'.

:pauses
@echo USE:
@echo "flashMcu.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR some_file_name"
pause

:exit
