:: BUILD Script for ARM targets

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

@rmdir ARMGCC\obj /s /q
@rmdir ARMGCC\bin /s /q
@mkdir ARMGCC\obj
@mkdir ARMGCC\bin
@mkdir ARMGCC\lib

@REM EXE_PATH from where the ldmicro.exe and *.bat are run
@SET EXE_PATH=%~dp0

@IF "%3" == "stm32f40x" SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\ARM\STM32F4
@IF "%3" == "stm32f10x" SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\ARM\STM32F1

IF not exist ARMGCC\lib\Lib_usr.c copy %LIB_PATH%\*.* ARMGCC\lib
dir ARMGCC\lib\*.c

SET GCC_PATH=C:\Program Files\EmIDE\emIDE V2.20\arm
PATH %GCC_PATH%\BIN;%PATH%

@IF "%3" == "stm32f10x" goto STM32F1


:STM32F4

:: compile libraries
arm-none-eabi-g++.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm-none-eabi\include" -c ARMGCC\lib\CortexM4.S -o ARMGCC\obj\cortexM4.o

CD ARMGCC\lib
FOR %%F in (*.c) do arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..\..

:: compile main file
arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -IARMGCC\lib\ -c %~n2.c -o ARMGCC\obj\%~n2.o

:: link object files
arm-none-eabi-gcc.exe -o ARMGCC\bin\%~nx2.elf ARMGCC\obj\*.o -Wl,-Map -Wl,ARMGCC\bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m4 -mthumb -TARMGCC\lib\CortexM4.ln

:: convert Elf to Hex
arm-none-eabi-objcopy -O ihex ARMGCC\bin\%~nx2.elf ARMGCC\bin\%~nx2.hex

:: Creation of the J-Link script
@echo r > ARMGCC\bin\cmdfile.jlink
@echo loadfile ARMGCC\bin\%2.hex >> ARMGCC\bin\cmdfile.jlink
@echo go >> ARMGCC\bin\cmdfile.jlink
@echo exit >> ARMGCC\bin\cmdfile.jlink

@GOTO END


:STM32F1

:: compile libraries
arm-none-eabi-g++.exe -O0 -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm-none-eabi\include" -c ARMGCC\lib\CortexM3.S -o ARMGCC\obj\cortexM3.o

CD ARMGCC\lib
FOR %%F in (*.c) do arm-none-eabi-gcc.exe -O0 -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..\..

:: compile main file
arm-none-eabi-gcc.exe -O0 -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -IARMGCC\lib\ -c %~n2.c -o ARMGCC\obj\%~n2.o

:: link object files
arm-none-eabi-gcc.exe -o ARMGCC\bin\%~nx2.elf ARMGCC\obj\*.o -Wl,-Map -Wl,ARMGCC\bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m3 -mthumb -TARMGCC\lib\CortexM3.ln

:: convert Elf to Hex
arm-none-eabi-objcopy -O ihex ARMGCC\bin\%~nx2.elf ARMGCC\bin\%~nx2.hex


:END
@echo ...
@pause
