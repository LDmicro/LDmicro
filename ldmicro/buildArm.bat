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

:: BUILD Script for ARM targets

:: %1 = project_path
:: %2 = file_name (.ld)
:: %3 = target_name
:: %4 = compiler_path
:: %5 = prog_tool

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

@if exist ARMGCC\obj rmdir ARMGCC\obj /s /q
@if exist ARMGCC\bin rmdir ARMGCC\bin /s /q
@if not exist ARMGCC\obj mkdir ARMGCC\obj
@if not exist ARMGCC\bin mkdir ARMGCC\bin
@if not exist ARMGCC\lib mkdir ARMGCC\lib

@rem EXE_PATH from where the ldmicro.exe and *.bat are run
@SET EXE_PATH=%~dp0

@IF "%3" == "stm32f40x" SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\ARM\STM32F4
@IF "%3" == "stm32f10x" SET LIB_PATH=%EXE_PATH%LIBRARIES_FOR\ARM\STM32F1

if not exist ARMGCC\lib\Lib_usr.c copy %LIB_PATH%\*.* ARMGCC\lib
dir ARMGCC\lib\*.c

  SET GCC_PATH=%ProgramFiles%\emIDE\emIDE V2.20\arm
::SET GCC_PATH=%ProgramFiles%\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain
::SET GCC_PATH=%ProgramFiles%\GNU Tools ARM Embedded\7 2018-q2-update
::SET GCC_PATH=%ProgramFiles%\Atmel\AVR-Gcc-8.2.0
::SET GCC_PATH=%ProgramFiles%\EmIDE\emIDE V2.20\arm
::SET GCC_PATH=%ProgramFiles%\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain
::SET GCC_PATH=D:\01 - Programas Plc\Atmel\AVR-Gcc-8.2.0
::SET GCC_PATH=D:\01 - Programas Plc\Arduino 1.8.0\hardware\tools\avr
::SET GCC_PATH=D:\WinAVR
::SET GCC_PATH=c:\WinAVR-20100110
::SET GCC_PATH=c:\avr-gcc-9.1.0-x64-mingw
::SET GCC_PATH=c:\avr8-gnu-toolchain-win32_x86

PATH %GCC_PATH%\BIN;%PATH%

@IF "%3" == "stm32f10x" goto STM32F1


:STM32F4

@rem Compile libraries
arm-none-eabi-g++.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm-none-eabi\include" -c ARMGCC\lib\CortexM4.S -o ARMGCC\obj\cortexM4.o

CD ARMGCC\lib
FOR %%F in (*.c) do arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..\..

@rem Compile main file
arm-none-eabi-gcc.exe -mcpu=cortex-m4 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -IARMGCC\lib\ -c %~nx2.c -o ARMGCC\obj\%~n2.o

@rem Link object files
arm-none-eabi-gcc.exe -o ARMGCC\bin\%~nx2.elf ARMGCC\obj\*.o -Wl,-Map -Wl,ARMGCC\bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m4 -mthumb -TARMGCC\lib\CortexM4.ln

@rem Convert Elf to Hex
arm-none-eabi-objcopy -O ihex ARMGCC\bin\%~nx2.elf ARMGCC\bin\%~nx2.hex

@rem Creation of the J-Link script
@echo r > ARMGCC\bin\cmdfile.jlink
@echo loadfile ARMGCC\bin\%2.hex >> ARMGCC\bin\cmdfile.jlink
@echo go >> ARMGCC\bin\cmdfile.jlink
@echo exit >> ARMGCC\bin\cmdfile.jlink

@GOTO END


:STM32F1

@rem Compile libraries
arm-none-eabi-g++.exe -O0 -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm-none-eabi\include" -c ARMGCC\lib\CortexM3.S -o ARMGCC\obj\cortexM3.o

CD ARMGCC\lib
FOR %%F in (*.c) do arm-none-eabi-gcc.exe -O0 -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -c %%F -o ..\obj\%%F.o
CD ..\..

@rem Compile main file
arm-none-eabi-gcc.exe -O0 -mcpu=cortex-m3 -mthumb -g -IInc -I"%GCC_PATH%\arm\arm-none-eabi\include" -IARMGCC\lib\ -c %~n2.c -o ARMGCC\obj\%~n2.o

@rem Link object files
arm-none-eabi-gcc.exe -o ARMGCC\bin\%~nx2.elf ARMGCC\obj\*.o -Wl,-Map -Wl,ARMGCC\bin\%~nx2.elf.map -Wl,--gc-sections -n -Wl,-cref -mcpu=cortex-m3 -mthumb -TARMGCC\lib\CortexM3.ln

@rem Convert Elf to Hex
arm-none-eabi-objcopy -O ihex ARMGCC\bin\%~nx2.elf ARMGCC\bin\%~nx2.hex


:END
@echo ...
@pause
