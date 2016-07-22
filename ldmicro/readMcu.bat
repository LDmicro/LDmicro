@echo OFF
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
cls
if "%1" == "" goto AVR
if "%1" == "AVR" goto AVR
if "%1" == "PIC16" goto PIC16
if "%1" == "" goto pauses
goto NOT_SUPPOTED

@rem =======================================================================
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
@rem ATmega8=M8
@rem SET AVRDUDE_PART_ID=m8
@rem ATmega328P=m328p
SET AVRDUDE_PART_ID=m328p

@rem read signature.
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U signature:r:%2_signature.hex:r
@echo ERRORLEVEL=%ERRORLEVEL%
if ERRORLEVEL==1 goto pauses

@rem read flash.
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U flash:r:%2_flash.hex:i
@rem read eeprom.
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U eeprom:r:%2_eeprom.hex:r
@rem read fuses.
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U fuse:r:%2_fuse.hex:r
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U lfuse:r:%2_lfuse.hex:r
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U hfuse:r:%2_hfuse.hex:r
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U efuse:r:%2_efuse.hex:r
@rem read calibration.
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U calibration:r:%2_calibration.hex:r
@rem read lock.
%AVRDUDE_PATH%avrdude.exe -y -c %AVRDUDE_PROGRAMMER_ID% -F -p %AVRDUDE_PART_ID% -U lock:r:%2_lock.hex:r
goto exit

@rem =======================================================================
:PIC16
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
rem pause