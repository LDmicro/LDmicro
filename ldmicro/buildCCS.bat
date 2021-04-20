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

@rem This file is part of LDmicro project and must be in the same directory where LDmicro.exe is located.

@title BUILD Script CCS C for PIC targets

@rem Note! All batch file arguments(parameters) are enclosed in quotation marks!
:: %1 = project_path
:: %2 = file_name (.ld)
:: %3 = target_name
:: %4 = compiler_path
:: %5 = prog_tool

@set P1=%~1
@set P2=%~2
@set P3=%~3
@set P4=%~4
@set P5=%~5

@echo Running script = %0
@echo Working dir = %1
@echo File name = %2
@echo Target name = %3
@echo ...

@rem %~d0 = Drive in full name
@rem %~p0 = Path in full name
@rem %~n0 = Name without extension
@rem %~dp0 = Drive + Path in full name
@rem %~nx0 = Name + extension

@rem Select working drive and directory
%~d1
chdir %~pn1

@if exist CCS\obj rmdir CCS\obj /s /q
@if exist CCS\bin rmdir CCS\bin /s /q
::@if not exist CCS\obj mkdir CCS\obj
::@if not exist CCS\bin mkdir CCS\bin
@if not exist CCS\lib mkdir CCS\lib

@rem EXE_PATH from where the ldmicro.exe and *.bat are run
@SET EXE_PATH=%~dp0

@SET LIB_PATH=%EXE_PATH%\LIBRARIES_FOR\CCS_PIC_C

if not exist CCS\lib\UsrLib.c copy %LIB_PATH%\*.* CCS\lib

dir CCS\lib\*.c

SET CCS_PATH=%ProgramFiles%\PICC

PATH %CCS_PATH%;%PATH%

@echo Compile libraries
FOR %%F in (CCS\lib\*.c) do ccsc.exe +P +DS +DF I="%CCS_PATH%\DEVICES" I+="%P1%\CCS\lib" "%P1%\CCS\lib\%%~nF.c"

@echo Compile main file
ccsc.exe +P +DS +DF I="%CCS_PATH%\DEVICES" I+="%P1%\CCS\lib" I+="%P1%\CCS\%~nx2" "%P1%\CCS\%~nx2\%~nx2.c"

::@echo Link object files
::ccsc.exe -o "CCS\bin\%~2.elf" CCS\obj\*.o -Wl,-Map="CCS\obj\%~2.map" -Wl,--start-group -Wl,-lm -Wl,--end-group -mmcu=%3

::@echo Convert Elf to Hex
::-objcopy.exe -O ihex -R .eeprom -R .fuse -R .lock -R .signature "CCS\bin\%~2.elf" "CCS\bin\%~2.hex"

::@echo off
:mkdir PROTEUS
if not exist PROTEUS goto skipPROTEUS
del PROTEUS\*.hex /Q > nul
del PROTEUS\*.elf /Q > nul
del PROTEUS\*.cof /Q > nul
REM Copy source code for debugging in Proteus
copy CCS\lib\*.h PROTEUS > nul
copy CCS\lib\*.c PROTEUS > nul
copy *.h PROTEUS > nul
copy *.c PROTEUS > nul
copy CCS\BIN\*.hex PROTEUS > nul
copy CCS\BIN\*.elf PROTEUS > nul
:skipPROTEUS

@echo ...
@pause
