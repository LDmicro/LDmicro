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

@rem MODE CON[:] [COLS=c] [LINES=n]
@MODE CON: COLS=160 LINES=65

@echo on
@echo This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo This file executes after menu Compile - F5
@rem EXE_PATH from where the ldmicro.exe and *.bat are executes
@SET  EXE_PATH=%~dp0
@echo %EXE_PATH% - EXE_PATH
@echo %1 - Compile target
@echo %2 - Prog.mcu.whichIsa
@echo %3 - LD path without the end backslash, double quoted
@echo %4 - LD file name without extentions and path, double quoted
@SET  COMPILER=%5
@echo %COMPILER% = COMPILER
@echo .
@set P3=%~3
@set P4=%~4
@echo %P3% - LD path without the end backslash and without double quotes
@echo %P4% - LD file name without extentions and path and without double quotes
;
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPLABX\v3.26\mpasmx\mpasmx.exe"
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPASM Suite\MPASMWIN.exe"
@SET PIC_ASM_TOOL="c:\Program Files (x86)\Microchip\MPLABX\v5.20\mpasmx\mpasmx.exe"
;
@SET DIFF_TOOL="D:\Program Files\WinMerge\WinMergeU.exe"
@SET DIFF_TOOL="c:\Program Files (x86)\WinMerge\WinMergeU.exe"
;
@rem if NOT EXIST "%P3%\ladder.h"    copy "%P3%\ladder.h_"   "%P3%\ladder.h"
@rem if NOT EXIST "%P3%\%P4%.ino"    copy "%P3%\%P4%.ino_"   "%P3%\%P4%.ino"
;

:PROTEUS_DEL
@mkdir  "%P3%\PROTEUS"
del "%P3%\PROTEUS\*.h"  > nul
del "%P3%\PROTEUS\*.c"  > nul
del "%P3%\PROTEUS\*.asm"  > nul
del "%P3%\PROTEUS\*.hex"  > nul
del "%P3%\PROTEUS\*.elf"  > nul
del "%P3%\PROTEUS\*.cof"  > nul

:pause
;
@if "%1" == "AVR"       goto AVR
@if "%COMPILER%" == "CCS_PIC_C" goto CCS
@if "%COMPILER%" == "HI_TECH_C" goto HTC
@if "%1" == "PIC16"     goto PIC16
@if "%1" == "ANSIC"     goto ANSIC
@if "%1" == "ARDUINO"   goto ARDUINO
@if "%1" == "PASCAL"    goto PASCAL
@if "%1" == ""          goto pauses
goto "%1"
goto NOT_SUPPOTED
;
@rem =======================================================================
:AVR
:ANSIC
@rem -----------------------------------------------------------------------
:AvrStudio
@mkdir  "%P3%\AvrStudio\%P4%"
copy "%P3%\%P4%.asm"          "%P3%\AvrStudio\%P4%"
copy "%P3%\%P4%.c"            "%P3%\AvrStudio\%P4%"
copy "%P3%\%P4%.h"            "%P3%\AvrStudio\%P4%"
copy "%P3%\ladder.h"          "%P3%\AvrStudio\%P4%"
@rem -----------------------------------------------------------------------
:CodeVisionAVR
@mkdir  "%P3%\CvAvr\%P4%"
copy "%P3%\%P4%.c"            "%P3%\CvAvr\%P4%"
copy "%P3%\%P4%.h"            "%P3%\CvAvr\%P4%"
copy "%P3%\ladder.h"          "%P3%\CvAvr\%P4%"
goto PROTEUS
@rem -----------------------------------------------------------------------
:HTC
:@mkdir  "%P3%\HTC"
:copy "%EXE_PATH%\LIBRARIES_FOR\Pic16\*.c"         "%P3%\HTC"
:copy "%EXE_PATH%\LIBRARIES_FOR\Pic16\*.h"         "%P3%\HTC"
:copy "%P3%\%P4%.c"            "%P3%\HTC"
:copy "%P3%\%P4%.h"            "%P3%\HTC"
:copy "%P3%\ladder.h"          "%P3%\HTC"
goto exit
@rem -----------------------------------------------------------------------
:CCS
@mkdir  "%P3%\CCS\%P4%"
copy "%P3%\%P4%.c"            "%P3%\CCS\%P4%"
copy "%P3%\%P4%.h"            "%P3%\CCS\%P4%"
copy "%P3%\ladder.h"          "%P3%\CCS\%P4%\ladder.h"
goto PROTEUS
@rem -----------------------------------------------------------------------
:MPLAB
@mkdir  "%P3%\MPLAB\%P4%"
copy "%P3%\%P4%.c"            "%P3%\MPLAB\%P4%"
copy "%P3%\%P4%.h"            "%P3%\MPLAB\%P4%"
copy "%P3%\ladder.h"          "%P3%\MPLAB\%P4%\ladder.h"
goto PROTEUS
@rem -----------------------------------------------------------------------
:VMLAB
@mkdir  "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%.asm"          "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%_prj.vmlab"    "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%.c"            "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%.h"            "%P3%\VMLAB\%P4%"
@rem -----------------------------------------------------------------------
:WINAVR
@mkdir  "%P3%\WINAVR\%P4%"
copy "%P3%\%P4%.c"            "%P3%\WINAVR\%P4%"
copy "%P3%\%P4%.h"            "%P3%\WINAVR\%P4%"
@rem -----------------------------------------------------------------------
goto PROTEUS
;
@rem =======================================================================
:PIC16
@rem -----------------------------------------------------------------------
:MPLAB
@mkdir  "%P3%\MPLAB"
copy "%P3%\%P4%.asm"          "%P3%\MPLAB"
;
::goto PROTEUS
;
del "%P3%\MPLAB\%P4%.hex"
@if NOT EXIST "%P3%\%P4%.asm" goto error
%PIC_ASM_TOOL% /d__DEBUG=1 "%P3%\MPLAB\%P4%.asm" /l"%P3%\MPLAB\%P4%.lst" /e"%P3%\MPLAB\%P4%.err"
;              /q
@if NOT EXIST "%P3%\MPLAB\%P4%.hex" notepad.bat "%P3%\MPLAB\%P4%.err" "%P3%\MPLAB\%P4%.asm"
@if NOT EXIST "%P3%\MPLAB\%P4%.hex" goto error
@if NOT EXIST "%P3%\%P4%.hex" goto error
;
start "DIFF" %DIFF_TOOL%  "%P3%\%P4%.hex"  "%P3%\MPLAB\%P4%.hex"
goto PROTEUS
;
@rem -----------------------------------------------------------------------
:PROTEUS
copy "%P3%\%P4%.asm"          "%P3%\PROTEUS"
copy "%P3%\%P4%.hex"          "%P3%\PROTEUS"
goto exit
;
@rem =======================================================================
:ARDUINO
:del /S D:\lds\ARDUINO_BUILD\*.*
rm -r D:\lds\ARDUINO_BUILD
mkdir D:\lds\ARDUINO_BUILD

:del /S /Y "%P3%\ARDUINO\%P4%\BUILD\*.*"
rm -r "%P3%\ARDUINO\%P4%\BUILD"
mkdir "%P3%\ARDUINO\%P4%\BUILD"

mkdir  "%P3%\ARDUINO"
mkdir  "%P3%\ARDUINO\%P4%"
::pause
copy "%P3%\%P4%.cpp"          "%P3%\ARDUINO\%P4%"
copy "%P3%\%P4%.h"            "%P3%\ARDUINO\%P4%"
::copy "%P3%\%P4%.ino_"         "%P3%\ARDUINO\%P4%"
::copy "%P3%\ladder.h_"         "%P3%\ARDUINO\%P4%"
copy "%P3%\ladder.h"          "%P3%\ARDUINO\%P4%"
copy "%P3%\%P4%.ino"          "%P3%\ARDUINO\%P4%"
@if NOT EXIST "%P3%\ARDUINO\%P4%\ladder.h"    copy "%P3%\ladder.h"   "%P3%\ARDUINO\%P4%"
@if NOT EXIST "%P3%\ARDUINO\%P4%\%P4%.ino"    copy "%P3%\%P4%.ino"   "%P3%\ARDUINO\%P4%"
:pause
goto exit
;
@rem =======================================================================
:PASCAL
copy "%P3%\pcports.pas"        C:\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.pas"          C:\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.inc"          C:\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.txt"          C:\MACHINE3\pas\SVARKA

copy "%P3%\pcports.pas"        C:\MACHINE1\trunk\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.pas"          C:\MACHINE1\trunk\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.inc"          C:\MACHINE1\trunk\MACHINE3\pas\SVARKA
:@pause
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
@pause
:exit
