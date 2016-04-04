@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls
if "%1" == "AVR" goto AVR
if "%1" == "PIC16" goto PIC16
if "%1" == "" goto pauses
goto NOT_SUPPOTED

:AVR
@rem Set up avrdude.exe path. It may be:
@rem SET AVRDUDE_PATH=D:\WinAVR\bin\
@rem SET AVRDUDE_PATH=D:\Arduino\hardware\tools\avr\bin\

@rem Set up your hardware avrdude programmer.
@rem   See avrdude.conf programmer id.
SET AVRDUDE_PROGRAMMER_ID=dapa
@rem SET AVRDUDE_PROGRAMMER_ID=stk200s5

@rem Set up your avrdude Atmel Microcontroller.
@rem   See avrdude.conf part id.
@rem ATmega8==m8
@rem SET AVRDUDE_PART_ID=m8
@rem ATmega328P==m328p
SET AVRDUDE_PART_ID=m328p

@rem read eeprom before flashing.
rem %AVRDUDE_PATH%avrdude.exe -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U eeprom:r:eeprom_read1:r

rem %AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -p %AVRDUDE_PART_ID% -U flash:w:"%2.hex":a -E noreset, novcc
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
