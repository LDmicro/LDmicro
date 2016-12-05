@echo on
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo %1 - Compile target
@echo %2 - LD path without the end backslash, double quoted
@echo %3 - LD file name without extentions and path, double quoted
;
rem goto exit
;
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPLABX\v3.26\mpasmx\mpasmx.exe"
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPASM Suite\MPASMWIN.exe"
;
@SET DIFF_TOOL="D:\Program Files\WinMerge\WinMergeU.exe"
;
@md %2\%3
;
if "%1" == "AVR" goto AVR
if "%1" == "PIC16" goto PIC16
if "%1" == "ARDUINO" goto ARDUINO
if "%1" == "" goto pauses
goto "%1"
goto NOT_SUPPOTED
;
@rem =======================================================================
:AVR
@rem -----------------------------------------------------------------------
:AvrStudio
@md  "%2\%3\AvrStudio"
copy "%2\%3.asm"  "%2\%3\AvrStudio"
@rem -----------------------------------------------------------------------
:VMLAB
;
@rem -----------------------------------------------------------------------
goto PROTEUS
;
@rem =======================================================================
:PIC16
@rem -----------------------------------------------------------------------
:MPLAB
goto PROTEUS

@md  %2\MPLAB
copy %2\%3.asm   %2\MPLAB
goto PROTEUS

@md  %2\%3\MPLAB
copy %2\%3.asm   %2\%3\MPLAB
goto PROTEUS

del "%2\%3\MPLAB\%3.hex"
IF NOT EXIST "%2\%3.asm" goto error
%PIC_ASM_TOOL% "%2\%3\MPLAB\%3.asm" /l"%2\%3\MPLAB\%3.lst" /e"%2\%3\MPLAB\%3.err" /d__DEBUG=1
IF NOT EXIST "%2\%3\MPLAB\%3.hex" notepad.bat "%2\%3\MPLAB\%3.err" "%2\%3\MPLAB\%3.asm"
IF NOT EXIST "%2\%3\MPLAB\%3.hex" goto error
IF NOT EXIST "%2\%3.hex" goto error
goto PROTEUS
;
%DIFF_TOOL%  "%2\%3.hex" "%2\%3\MPLAB\%3.hex"
goto PROTEUS
;
@rem -----------------------------------------------------------------------
:PROTEUS
rem goto exit

@md  %2\PROTEUS
copy %2\%3.asm   %2\PROTEUS
copy %2\%3.hex   %2\PROTEUS
goto exit

@md  %2\%3\PROTEUS
copy %2\%3.asm   %2\%3\PROTEUS
copy %2\%3.hex   %2\%3\PROTEUS
goto exit
;
@rem =======================================================================
:ARDUINO
@md  "%2\%3\ARDUINO"
copy "%2\%3.cpp"      "%2\%3\ARDUINO"
copy "%2\%3.h"        "%2\%3\ARDUINO"
copy "%2\%3.ino_"     "%2\%3\ARDUINO"
copy "%2\ladder.h_"   "%2\%3\ARDUINO"
rem pause
goto exit
;
@rem =======================================================================
:NOT_SUPPOTED
@echo You can write own command for '%1'.

:pauses
@echo USE:
@echo "postCompile.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR"
pause

:error
pause
:exit
rem pause