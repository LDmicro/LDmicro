@echo off
@rem This file is part of LDmicro and must be in the same directory where LDmicro.exe is located.

REM EXE_PATH from where the ldmicro.exe and *.bat are run
SET EXE_PATH=%~dp0

@echo ISA type = %1
@echo Filename = %2
@echo Compiler = %3
@echo Target name = %4
@echo ...

@REM %~d2 = Drive letter in path
@REM %~p2 = Path to ld file
@REM %~nx2 = Filename without path

@SET COMPILER=%3

if %1 == "PIC16" goto PIC16
if %1 == "PIC18" goto PIC18
if %1 == "AVR" goto AVR
if %1 == "ARM" goto ARM

goto NOT_SUPPORTED

@rem =======================================================================
:AVR

@rem *** Set up avrdude.exe configuration ***
SET AVRDUDE_PATH="D:\Programmation\Ladder\Programmes\Tests\Avr\AvrDude"
SET PATH=%PATH%;%AVRDUDE_PATH%\bin
SET AVRDUDE_CONF=%AVRDUDE_PATH%\avrdude.conf
SET AVRDUDE_PROGRAMMER=stk500
SET AVRDUDE_OPTIONS=-b 19200 -u -v -F
SET COMPORT=COM3

@ECHO on

@rem *** Flash the AVR ***
:: avrdude.exe" -C "%AVRDUDE_CONF%" %AVRDUDE_OPTIONS% -p %4 -c %AVRDUDE_PROGRAMMER% -P %COMPORT% -U flash:w:"%~nx2.hex"
avrdude.exe -p %4 -c avr910 -P %COMPORT% -b 19200 -u -v -F -U flash:w:AVRGCC\bin\%~nx2.hex

@GOTO exit


@rem =======================================================================
:PIC16

@rem *** Set up PIC flashing tool ***
SET TOOL_PATH="C:\Program Files\Microchip\MPLAB IDE\Programmer Utilities\PICkit3"
SET PATH=%PATH%;%TOOL_PATH%
SET TOOL_OPTIONS=-E -L -M -Y

@ECHO on

@rem *** Flash the PIC16 ***
PK3CMD.exe -P%4 -FHTC\bin\%~nx2.hex %TOOL_OPTIONS%

@GOTO exit


@rem =======================================================================
:PIC18

@rem *** Set up PIC flashing tool ***
SET TOOL_PATH="C:\Program Files\Microchip\MPLAB IDE\Programmer Utilities\PICkit3"
SET PATH=%PATH%;%TOOL_PATH%
SET TOOL_OPTIONS=-E -L -M -Y

@ECHO on

@rem *** Flash the PIC18 ***
PK3CMD.exe -P%4 -FHTC\bin\%~nx2.hex %TOOL_OPTIONS%

@GOTO exit


@rem =======================================================================
:ARM

IF "%4" == "stm32f10x" goto STM32F1

:STM32F4

@rem *** Set up J-LINK utility ***
SET JLN_PATH="C:\Program Files\SEGGER\JLink_V502j"
SET PATH=%PATH%;%JLN_PATH%

@ECHO on

@rem *** Flash the STM32 with J-Link ***
JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript ARMGCC\bin\cmdfile.jlink
JLink.exe -device stm32f407zg -if JTAG -speed 1000 -CommanderScript ARMGCC\bin\cmdfile.jlink

@GOTO exit

:STM32F1

@rem *** Set up ST-LINK utility ***
SET STL_PATH="%ProgramFiles%\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility"
SET PATH=%PATH%;%STL_PATH%

@ECHO on

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
