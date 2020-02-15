@rem COLOR [background][foreground]
@rem 0 = Black   8 = Gray
@rem 1 = Blue    9 = Light Blue
@rem 2 = Green   A = Light Green
@rem 3 = Aqua    B = Light Aqua
@rem 4 = Red     C = Light Red
@rem 5 = Purple  D = Light Purple
@rem 6 = Yellow  E = Light Yellow
@rem 7 = White   F = Bright White
@COLOR F0

@echo off
@rem This file is part of LDmicro and must be in the same directory where LDmicro.exe is located.

REM EXE_PATH from where the ldmicro.exe and *.bat are run
SET EXE_PATH=%~dp0

@echo ISA type = %1
@echo Filename = %2
@echo Compiler = %3
@echo Target name = %4
@echo ...

@rem %~d2 = Drive letter in path
@rem %~p2 = Path to ld file
@rem %~nx2 = Filename without path

@SET COMPILER=%3

if %1 == "PIC16" goto PIC16
if %1 == "PIC18" goto PIC18
if %1 == "AVR" goto AVR
if %1 == "ARM" goto ARM

goto NOT_SUPPORTED

@rem =======================================================================
:AVR
SET AVRDUDE_PART_ID=%4
if %4 == "ATmega8"      SET AVRDUDE_PART_ID=m8
if %4 == "ATmega328"    SET AVRDUDE_PART_ID=m328p
if %4 == "ATmega328P"   SET AVRDUDE_PART_ID=m328p
if %4 == "ATmega32U4"   SET AVRDUDE_PART_ID=m32u4
if %4 == "ATmega2560"   SET AVRDUDE_PART_ID=m2560
if %4 == "ATmega32u4"   SET AVRDUDE_PART_ID=m32u4
echo AVRDUDE_PART_ID = %AVRDUDE_PART_ID%

@rem *** Set up avrdude.exe configuration ***
SET AVRDUDE_PATH="D:\Programmation\Ladder\Programmes\Tests\Avr\AvrDude"
SET PATH=%PATH%;%AVRDUDE_PATH%\bin
SET AVRDUDE_CONF=%AVRDUDE_PATH%\avrdude.conf
SET AVRDUDE_PROGRAMMER=stk500
SET AVRDUDE_OPTIONS=-b 19200 -u -v -F
SET COMPORT=COM5

@echo on

@rem *** Flash the AVR ***
::avrdude.exe" -C "%AVRDUDE_CONF%" %AVRDUDE_OPTIONS% -p %4 -c %AVRDUDE_PROGRAMMER% -P %COMPORT% -U flash:w:"%~nx2.hex"
avrdude.exe -p %AVRDUDE_PART_ID% -c avr910 -P %COMPORT% -b 19200 -u -v -F -U flash:w:"AVRGCC\bin\%~nx2.hex"

@GOTO exit


@rem =======================================================================
:PIC16

@rem *** Set up PIC flashing tool ***
::SET TOOL_PATH="%ProgramFiles%\Microchip\MPLABX\v5.20\mplab_platform\mplab_ipe"
::SET TOOL_PATH="%ProgramFiles%\Microchip\PICkit 3 v3"
  SET TOOL_PATH="%ProgramFiles%\Microchip\MPLAB IDE\Programmer Utilities\PICkit3"

SET PATH=%PATH%;%TOOL_PATH%
SET TOOL_OPTIONS=-E -L -M -Y

@echo on

@rem *** Flash the PIC16 ***
PK3CMD.exe -P%4 -F"HTC\bin\%~nx2.hex" %TOOL_OPTIONS%

@GOTO exit


@rem =======================================================================
:PIC18

@rem *** Set up PIC flashing tool ***
::SET TOOL_PATH="%ProgramFiles%\Microchip\MPLABX\v5.20\mplab_platform\mplab_ipe"
  SET TOOL_PATH="%ProgramFiles%\Microchip\MPLAB IDE\Programmer Utilities\PICkit3"

SET PATH=%PATH%;%TOOL_PATH%
SET TOOL_OPTIONS=-E -L -M -Y

@echo on

@rem *** Flash the PIC18 ***
PK3CMD.exe -P%4 -F"HTC\bin\%~nx2.hex" %TOOL_OPTIONS%

@GOTO exit


@rem =======================================================================
:ARM

IF %4 == "stm32f10x" goto STM32F1
IF %4 == "stm32f40x" goto STM32F4
goto NOT_SUPPORTED

:STM32F4

@rem *** Set up J-LINK utility ***
::SET JLN_PATH="%ProgramFiles%\SEGGER\JLink"
  SET JLN_PATH="%ProgramFiles%\SEGGER\JLink_V502j"

SET PATH=%PATH%;%JLN_PATH%

@echo on

@rem *** Flash the STM32 with J-Link ***
JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript ARMGCC\bin\cmdfile.jlink
JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript ARMGCC\bin\cmdfile.jlink

@GOTO exit

:STM32F1

@rem *** Set up ST-LINK utility ***
SET STL_PATH="%ProgramFiles%\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility"
SET PATH=%PATH%;%STL_PATH%

@echo on

@rem *** Flash the STM32 with ST-Link ***
ST-LINK_CLI.exe -c SWD -P ARMGCC\bin\%~nx2.hex -V "after_programming" -Run

@GOTO exit


@rem =======================================================================
:NOT_SUPPORTED
@echo Target not supported !!!


@rem =======================================================================
:exit
@echo ...
@pause
