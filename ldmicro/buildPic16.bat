:: BUILD Script for PIC16 targets

:: %1 = project_path
:: %2 = file_name (.ld)
:: %3 = target_name
:: %4 = compiler_path
:: %5 = prog_tool

@ECHO Running script = %0
@ECHO Working dir = %1
@ECHO File name = %2
@ECHO Target name = %3
@Echo ...

@REM %~d0 = Drive in full name
@REM %~p0 = Path in full name
@REM %~n0 = Name without extension
@REM %~dp0 = Drive + Path in full name
@REM %~nx0 = Name + extension

@rmdir HTC\obj /s /q
@rmdir HTC\bin /s /q
@mkdir HTC\obj
@mkdir HTC\bin
@mkdir HTC\lib

@REM EXE_PATH from where the ldmicro.exe and *.bat are run
@SET EXE_PATH=%~dp0

@SET LIB_PATH=%EXE_PATH%\LIBRARIES_FOR\PIC16

IF not exist HTC\lib\UsrLib.c copy %LIB_PATH%\*.* HTC\lib
dir HTC\lib\*.c

SET PCC_PATH=C:\Program Files\HI-TECH Software\PICC\9.81
SET PICKIT_PATH=C:\Program Files\Microchip\MPLAB IDE\Programmer Utilities\PICkit3
PATH %PCC_PATH%\BIN;%PICKIT_PATH%;%PATH%

:: compile libraries
:: %4\picc.exe --pass1 UsrLib.c -q --chip=%3 -P -I%1 -I%1\HTC\lib --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj
FOR %%F in (HTC\lib\*.c) do  picc.exe --pass1 %%F -q --chip=%3 -P -I%1 -I%1\HTC\lib --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj

:: compile main file
:: %4\picc.exe --pass1 %2.c -q --chip=%3 -P --runtime=default -I%1\HTC\lib --opt=default -g --asmlist --OBJDIR=HTC\obj
picc.exe --pass1 %2.c -q --chip=%3 -P --runtime=default -I%1\HTC\lib --opt=default -g --asmlist --OBJDIR=HTC\obj

:: link object files
:: %4\picc.exe -oHTC\bin\%2.cof -mHTC\bin\%2.map --summary=default --output=default HTC\obj\*.p1 --chip=%3 -P --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj --OUTDIR=HTC\bin
picc.exe -oHTC\bin\%2.cof -mHTC\bin\%2.map --summary=default --output=default HTC\obj\*.p1 --chip=%3 -P --runtime=default --opt=default -g --asmlist --OBJDIR=HTC\obj --OUTDIR=HTC\bin

@echo ...
@pause
