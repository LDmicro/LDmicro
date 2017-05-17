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
rem pause
IF NOT EXIST "%2\ladder.h"  copy "%2\ladder.h_"  "%2\ladder.h"
IF NOT EXIST "%2\%3.ino"    copy "%2\%3.ino_"    "%2\%3.ino"
;
if "%1" == "AVR"     goto AVR
if "%1" == "PIC16"   goto PIC16
if "%1" == "ANSIC"   goto ANSIC
if "%1" == "ARDUINO" goto ARDUINO
if "%1" == ""        goto pauses
goto "%1"
goto NOT_SUPPOTED
;
@rem =======================================================================
:AVR
:ANSIC
@rem -----------------------------------------------------------------------
:AvrStudio
@md  "%2\AvrStudio\%3"
copy "%2\%3.asm"          "%2\AvrStudio\%3"
copy "%2\%3.c"            "%2\AvrStudio\%3"
copy "%2\%3.h"            "%2\AvrStudio\%3"
copy "%2\ladder.h"        "%2\AvrStudio\%3"
@rem -----------------------------------------------------------------------
:CodeVisionAVR
@md  "%2\CvAvr\%3"
copy "%2\%3.c"            "%2\CvAvr\%3"
copy "%2\%3.h"            "%2\CvAvr\%3"
copy "%2\ladder.h"        "%2\CvAvr\%3"
goto PROTEUS
@rem -----------------------------------------------------------------------
:VMLAB
@md  "%2\VMLAB\%3"
copy "%2\%3.asm"          "%2\VMLAB\%3"
copy "%2\%3_prj.vmlab"    "%2\VMLAB\%3"
copy "%2\%3.c"            "%2\VMLAB\%3"
copy "%2\%3.h"            "%2\VMLAB\%3"
@rem -----------------------------------------------------------------------
:WINAVR
@md  "%2\WINAVR\%3"
copy "%2\%3.c"            "%2\WINAVR\%3"
copy "%2\%3.h"            "%2\WINAVR\%3"
@rem -----------------------------------------------------------------------
rem pause
goto PROTEUS
;
@rem =======================================================================
:PIC16
@rem -----------------------------------------------------------------------
:MPLAB
goto PROTEUS

@md  "%2\MPLAB"
copy "%2\%3.asm"          "%2\MPLAB"
goto PROTEUS

@md  "%2\%3\MPLAB"
copy "%2\%3.asm"          "%2\%3\MPLAB"
goto PROTEUS

del "%2\%3\MPLAB\%3.hex"
IF NOT EXIST "%2\%3.asm" goto error
%PIC_ASM_TOOL% "%2\%3\MPLAB\%3.asm" /l"%2\%3\MPLAB\%3.lst" /e"%2\%3\MPLAB\%3.err" /d__DEBUG=1
IF NOT EXIST "%2\%3\MPLAB\%3.hex" notepad.bat "%2\%3\MPLAB\%3.err" "%2\%3\MPLAB\%3.asm"
IF NOT EXIST "%2\%3\MPLAB\%3.hex" goto error
IF NOT EXIST "%2\%3.hex" goto error
goto PROTEUS
;
%DIFF_TOOL%  "%2\%3.hex"  "%2\%3\MPLAB\%3.hex"
goto PROTEUS
;
@rem -----------------------------------------------------------------------
:PROTEUS
rem goto exit
@md  "%2\PROTEUS"
copy "%2\%3.asm"          "%2\PROTEUS"
copy "%2\%3.hex"          "%2\PROTEUS"
goto exit
;
@rem =======================================================================
:ARDUINO
@md  "%2\ARDUINO"
@md  "%2\ARDUINO\%3"
copy "%2\%3.cpp"          "%2\ARDUINO\%3"
copy "%2\%3.ino"          "%2\ARDUINO\%3"
copy "%2\%3.ino_"         "%2\ARDUINO\%3"
copy "%2\%3.h"            "%2\ARDUINO\%3"
copy "%2\ladder.h_"       "%2\ARDUINO\%3"
rem pause
goto exit
;
@rem =======================================================================
:NOT_SUPPOTED
@echo You can write own command for '%1'.
;
:pauses
@echo USE:
@echo "postCompile.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR"
;
:error
pause
:exit
rem pause