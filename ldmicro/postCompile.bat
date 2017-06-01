@echo on
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo %1 - Compile target
@echo %2 - Prog.mcu.whichIsa
@echo %3 - LD path without the end backslash, double quoted
@echo %4 - LD file name without extentions and path, double quoted
;
rem goto exit
;
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPLABX\v3.26\mpasmx\mpasmx.exe"
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPASM Suite\MPASMWIN.exe"
;
@SET DIFF_TOOL="D:\Program Files\WinMerge\WinMergeU.exe"
;
rem pause
IF NOT EXIST "%3\ladder.h"  copy "%3\ladder.h_"  "%3\ladder.h"
IF NOT EXIST "%3\%4.ino"    copy "%3\%4.ino_"    "%3\%4.ino"
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
if "%2" == "PIC16"   goto CCS
@rem -----------------------------------------------------------------------
:AvrStudio
@md  "%3\AvrStudio\%4"
copy "%3\%4.asm"          "%3\AvrStudio\%4"
copy "%3\%4.c"            "%3\AvrStudio\%4"
copy "%3\%4.h"            "%3\AvrStudio\%4"
copy "%3\ladder.h"        "%3\AvrStudio\%4"
@rem -----------------------------------------------------------------------
:CodeVisionAVR
@md  "%3\CvAvr\%4"
copy "%3\%4.c"            "%3\CvAvr\%4"
copy "%3\%4.h"            "%3\CvAvr\%4"
copy "%3\ladder.h"        "%3\CvAvr\%4"
goto PROTEUS
@rem -----------------------------------------------------------------------
:CCS
@md  "%3\CCS\%4"
copy "%3\%4.c"            "%3\CCS\%4"
copy "%3\%4.h"            "%3\CCS\%4"
copy "%3\ladder.h"        "%3\CCS\%4\ladder.h"
@rem -----------------------------------------------------------------------
:MPLAB
@md  "%3\MPLAB\%4"
copy "%3\%4.c"            "%3\MPLAB\%4"
copy "%3\%4.h"            "%3\MPLAB\%4"
copy "%3\ladder.h"        "%3\MPLAB\%4\ladder.h"
goto PROTEUS
@rem -----------------------------------------------------------------------
:VMLAB
@md  "%3\VMLAB\%4"
copy "%3\%4.asm"          "%3\VMLAB\%4"
copy "%3\%4_prj.vmlab"    "%3\VMLAB\%4"
copy "%3\%4.c"            "%3\VMLAB\%4"
copy "%3\%4.h"            "%3\VMLAB\%4"
@rem -----------------------------------------------------------------------
:WINAVR
@md  "%3\WINAVR\%4"
copy "%3\%4.c"            "%3\WINAVR\%4"
copy "%3\%4.h"            "%3\WINAVR\%4"
@rem -----------------------------------------------------------------------
rem pause
goto PROTEUS
;
@rem =======================================================================
:PIC16
@rem -----------------------------------------------------------------------
:MPLAB
goto PROTEUS

@md  "%3\MPLAB"
copy "%3\%4.asm"          "%3\MPLAB"
goto PROTEUS

@md  "%3\%4\MPLAB"
copy "%3\%4.asm"          "%3\%4\MPLAB"
goto PROTEUS

del "%3\%4\MPLAB\%4.hex"
IF NOT EXIST "%3\%4.asm" goto error
%PIC_ASM_TOOL% "%3\%4\MPLAB\%4.asm" /l"%3\%4\MPLAB\%4.lst" /e"%3\%4\MPLAB\%4.err" /d__DEBUG=1
IF NOT EXIST "%3\%4\MPLAB\%4.hex" notepad.bat "%3\%4\MPLAB\%4.err" "%3\%4\MPLAB\%4.asm"
IF NOT EXIST "%3\%4\MPLAB\%4.hex" goto error
IF NOT EXIST "%3\%4.hex" goto error
goto PROTEUS
;
%DIFF_TOOL%  "%3\%4.hex"  "%3\%4\MPLAB\%4.hex"
goto PROTEUS
;
@rem -----------------------------------------------------------------------
:PROTEUS
rem goto exit
@md  "%3\PROTEUS"
copy "%3\%4.asm"          "%3\PROTEUS"
copy "%3\%4.hex"          "%3\PROTEUS"
goto exit
;
@rem =======================================================================
:ARDUINO
@md  "%3\ARDUINO"
@md  "%3\ARDUINO\%4"
copy "%3\%4.cpp"          "%3\ARDUINO\%4"
copy "%3\%4.h"            "%3\ARDUINO\%4"
copy "%3\%4.ino_"         "%3\ARDUINO\%4"
copy "%3\ladder.h_"       "%3\ARDUINO\%4"
IF NOT EXIST "%3\ARDUINO\%4\ladder.h"  copy "%3\ladder.h"   "%3\ARDUINO\%4"
IF NOT EXIST "%3\ARDUINO\%4\%4.ino"    copy "%3\%4.ino"     "%3\ARDUINO\%4"
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