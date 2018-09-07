@echo on
@echo This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo This file executes after menu Compile - F5
@echo %1 - Compile target
@echo %2 - Prog.mcu.whichIsa
@echo %3 - LD path without the end backslash, double quoted
@echo %4 - LD file name without extentions and path, double quoted
@set P3=%~3
@set P4=%~4
@echo %P3% - LD path without the end backslash and without double quotes
@echo %P4% - LD file name without extentions and path and without double quotes
;
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPLABX\v3.26\mpasmx\mpasmx.exe"
@SET PIC_ASM_TOOL="D:\Program Files\Microchip\MPASM Suite\MPASMWIN.exe"
;
@SET DIFF_TOOL="D:\Program Files\WinMerge\WinMergeU.exe"
;
@if NOT EXIST "%P3%\ladder.h"    copy "%P3%\ladder.h_"   "%P3%\ladder.h"
@if NOT EXIST "%P3%\%P4%.ino"    copy "%P3%\%P4%.ino_"   "%P3%\%P4%.ino"
;
@rem pause
;
@if "%1" == "AVR"     goto AVR
@if "%1" == "PIC16"   goto PIC16
@if "%1" == "ANSIC"   goto ANSIC
@if "%1" == "ARDUINO" goto ARDUINO
@if "%1" == "PASCAL"  goto PASCAL
@if "%1" == ""        goto pauses
goto "%1"
goto NOT_SUPPOTED
;
@rem =======================================================================
:AVR
:ANSIC
@if "%2" == "PIC16"   goto CCS
@rem -----------------------------------------------------------------------
:AvrStudio
@md  "%P3%\AvrStudio\%P4%"
copy "%P3%\%P4%.asm"          "%P3%\AvrStudio\%P4%"
copy "%P3%\%P4%.c"            "%P3%\AvrStudio\%P4%"
copy "%P3%\%P4%.h"            "%P3%\AvrStudio\%P4%"
copy "%P3%\ladder.h"          "%P3%\AvrStudio\%P4%"
@rem -----------------------------------------------------------------------
:CodeVisionAVR
@md  "%P3%\CvAvr\%P4%"
copy "%P3%\%P4%.c"            "%P3%\CvAvr\%P4%"
copy "%P3%\%P4%.h"            "%P3%\CvAvr\%P4%"
copy "%P3%\ladder.h"          "%P3%\CvAvr\%P4%"
goto PROTEUS
@rem -----------------------------------------------------------------------
:CCS
@md  "%P3%\CCS\%P4%"
copy "%P3%\%P4%.c"            "%P3%\CCS\%P4%"
copy "%P3%\%P4%.h"            "%P3%\CCS\%P4%"
copy "%P3%\ladder.h"          "%P3%\CCS\%P4%\ladder.h"
@rem -----------------------------------------------------------------------
:MPLAB
@md  "%P3%\MPLAB\%P4%"
copy "%P3%\%P4%.c"            "%P3%\MPLAB\%P4%"
copy "%P3%\%P4%.h"            "%P3%\MPLAB\%P4%"
copy "%P3%\ladder.h"          "%P3%\MPLAB\%P4%\ladder.h"
goto PROTEUS
@rem -----------------------------------------------------------------------
:VMLAB
@md  "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%.asm"          "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%_prj.vmlab"    "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%.c"            "%P3%\VMLAB\%P4%"
copy "%P3%\%P4%.h"            "%P3%\VMLAB\%P4%"
@rem -----------------------------------------------------------------------
:WINAVR
@md  "%P3%\WINAVR\%P4%"
copy "%P3%\%P4%.c"            "%P3%\WINAVR\%P4%"
copy "%P3%\%P4%.h"            "%P3%\WINAVR\%P4%"
@rem -----------------------------------------------------------------------
goto PROTEUS
;
@rem =======================================================================
:PIC16
@rem -----------------------------------------------------------------------
:MPLAB
@md  "%P3%\MPLAB"
copy "%P3%\%P4%.asm"          "%P3%\MPLAB"
;
goto PROTEUS
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
@md  "%P3%\PROTEUS"
copy "%P3%\%P4%.asm"          "%P3%\PROTEUS"
copy "%P3%\%P4%.hex"          "%P3%\PROTEUS"
goto exit
;
@rem =======================================================================
:ARDUINO
md  "%P3%\ARDUINO"
md  "%P3%\ARDUINO\%P4%"
@rem @del /S /Y "%P3%\ARDUINO\%P4%\BUILD\*.*"
rm -r "%P3%\ARDUINO\%P4%\BUILD"
@rem pause
copy "%P3%\%P4%.cpp"          "%P3%\ARDUINO\%P4%"
copy "%P3%\%P4%.h"            "%P3%\ARDUINO\%P4%"
@rem copy "%P3%\%P4%.ino_"         "%P3%\ARDUINO\%P4%"
@rem copy "%P3%\ladder.h_"         "%P3%\ARDUINO\%P4%"
copy "%P3%\ladder.h"          "%P3%\ARDUINO\%P4%"
copy "%P3%\%P4%.ino"          "%P3%\ARDUINO\%P4%"
@rem if NOT EXIST "%P3%\ARDUINO\%P4%\ladder.h"    copy "%P3%\ladder.h"   "%P3%\ARDUINO\%P4%"
@rem if NOT EXIST "%P3%\ARDUINO\%P4%\%P4%.ino"    copy "%P3%\%P4%.ino"   "%P3%\ARDUINO\%P4%"
@rem pause
goto exit
;
@rem =======================================================================
:PASCAL
copy "%P3%\pcports.pas"        \MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.pas"          \MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.inc"          \MACHINE3\pas\SVARKA

copy "%P3%\pcports.pas"        C:\MACHINE1\trunk\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.pas"          C:\MACHINE1\trunk\MACHINE3\pas\SVARKA
copy "%P3%\%P4%*.inc"          C:\MACHINE1\trunk\MACHINE3\pas\SVARKA
@pause
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
