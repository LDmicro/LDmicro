@echo on
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo %1 - Compile target
@echo %2 - LD path
@echo %3 - LD file name without extentions and path

if "%1" == "AVR" goto AVR
if "%1" == "PIC16" goto PIC16
if "%1" == "" goto pauses
goto NOT_SUPPOTED

:AVR
@rem -----------------------------------------------------------------------
:AvrStudio
@md "%2AvrStudio"
@md "%2AvrStudio\%3"
copy "%2%3.asm"  "%2AvrStudio\%3"

@rem -----------------------------------------------------------------------
goto PROTEUS

:PIC16
@rem -----------------------------------------------------------------------
:MPLAB
@md "%2MPLAB"
@md "%2MPLAB\%3"
copy "%2%3.asm"        "%2MPLAB\%3"
goto PROTEUS

@rem -----------------------------------------------------------------------
:PROTEUS
@md "%2PROTEUS"
@md "%2PROTEUS\%3"
copy "%2%3.asm"  "%2PROTEUS\%3"
goto exit

@rem -----------------------------------------------------------------------
:ARDUINO
@md "%2ARDUINO"
@md "%2ARDUINO\%3"
copy "%2%3.cpp"      "%2ARDUINO\%3"
copy "%2%3.h"        "%2ARDUINO\%3"
copy "%2\ladder.h_"  "%2ARDUINO\%3"
copy "%2%3.ino_"     "%2ARDUINO\%3"
@rem -----------------------------------------------------------------------
@rem @pause

:NOT_SUPPOTED
@echo You can write own command for '%1'.

:pauses
@echo USE:
@echo "postCompile.bat AVR|PIC16|ANSIC|INTERPRETED|NETZER|PASCAL|ARDUINO|CAVR"
pause

:exit
rem pause