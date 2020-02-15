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

:: BUILD Script for AVR targets

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

@if exist AVRGCC\obj rmdir AVRGCC\obj /s /q
@if exist AVRGCC\bin rmdir AVRGCC\bin /s /q
@if not exist AVRGCC\obj mkdir AVRGCC\obj
@if not exist AVRGCC\bin mkdir AVRGCC\bin
@if not exist AVRGCC\lib mkdir AVRGCC\lib

@rem EXE_PATH from where the ldmicro.exe and *.bat are run
@SET EXE_PATH=%~dp0

@SET LIB_PATH=%EXE_PATH%\LIBRARIES_FOR\AVR

if not exist AVRGCC\lib\UsrLib.c copy %LIB_PATH%\*.* AVRGCC\lib
dir AVRGCC\lib\*.c

::SET GCC_PATH=%ProgramFiles%\Atmel\AVR-Gcc-8.2.0
  SET GCC_PATH=%ProgramFiles%\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin
::SET GCC_PATH=%ProgramFiles%\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain
::SET GCC_PATH=%ProgramFiles%\Arduino\hardware\tools\avr\bin
::SET GCC_PATH=%ProgramFiles%\Labcenter Electronics\Proteus 8.7 SP3 Professional\Tools\ARDUINO\hardware\tools\avr\bin
::SET GCC_PATH=%ProgramFiles%\Labcenter Electronics\Proteus 8 Professional\Tools\ARDUINO\hardware\tools\avr\bin
::SET GCC_PATH=D:\01 - Programas Plc\Atmel\AVR-Gcc-8.2.0
::SET GCC_PATH=D:\01 - Programas Plc\Arduino 1.8.0\hardware\tools\avr
::SET GCC_PATH=D:\WinAVR
::SET GCC_PATH=c:\WinAVR-20100110
::SET GCC_PATH=c:\avr-gcc-9.1.0-x64-mingw
::SET GCC_PATH=c:\avr8-gnu-toolchain-win32_x86

PATH %GCC_PATH%\BIN;%PATH%

@rem Compile libraries
:: %4\avr-gcc.exe -I%1 -I%1\AVRGCC\lib\ -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -MD -MP -mmcu=%3 -MF AVRGCC\obj\UsrLib.d -MT AVRGCC\obj\UsrLib.d -MT AVRGCC\obj\UsrLib.o %1\AVRGCC\lib\UsrLib.c -o AVRGCC\obj\UsrLib.o
FOR %%F in (AVRGCC\lib\*.c) do avr-gcc.exe -I%1 -I%1\AVRGCC\lib\ -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -MD -MP -mmcu=%3 -MF AVRGCC\obj\%%~nF.d -MT AVRGCC\obj\%%~nF.d -MT AVRGCC\obj\%%~nF.o %1\AVRGCC\lib\%%~nF.c -o AVRGCC\obj\%%~nF.o

@echo Compile main file
:: %4\avr-gcc.exe -I%1\AVRGCC\lib\ -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -c -std=gnu99 -MD -MP -mmcu=%3 -MF AVRGCC\obj\%2.d -MT AVRGCC\obj\%2.d -MT AVRGCC\obj\%2.o %1\%2.c -o AVRGCC\obj\%2.o
avr-gcc.exe -IAVRGCC\lib\ -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -c -std=gnu99 -MD -MP -mmcu=%3 -MF "AVRGCC\obj\%~2.d" -MT "AVRGCC\obj\%~2.d" -MT "AVRGCC\obj\%~2.o" "%~nx2.c" -o "AVRGCC\obj\%~2.o"

@echo Link object files
:: %4\avr-gcc.exe -o AVRGCC\bin\%2.elf AVRGCC\obj\*.o -Wl,-Map=AVRGCC\obj\%2.map -Wl,--start-group -Wl,-lm -Wl,--end-group -mmcu=%3
avr-gcc.exe -o "AVRGCC\bin\%~2.elf" AVRGCC\obj\*.o -Wl,-Map="AVRGCC\obj\%~2.map" -Wl,--start-group -Wl,-lm -Wl,--end-group -mmcu=%3

@echo Convert Elf to Hex
:: %4\avr-objcopy.exe -O ihex -R .eeprom -R .fuse -R .lock -R .signature AVRGCC\bin\%2.elf AVRGCC\bin\%2.hex
avr-objcopy.exe -O ihex -R .eeprom -R .fuse -R .lock -R .signature "AVRGCC\bin\%~2.elf" "AVRGCC\bin\%~2.hex"

@echo off
:mkdir PROTEUS
if not exist PROTEUS goto skipPROTEUS
del PROTEUS\*.hex  > nul
del PROTEUS\*.elf  > nul
del PROTEUS\*.cof  > nul
REM Copy source code for debugging in Proteus
copy AVRGCC\lib\*.h PROTEUS > nul
copy AVRGCC\lib\*.c PROTEUS > nul
copy *.h PROTEUS > nul
copy *.c PROTEUS > nul
copy AVRGCC\BIN\*.hex PROTEUS > nul
copy AVRGCC\BIN\*.elf PROTEUS > nul
:skipPROTEUS

@echo ...
@pause
