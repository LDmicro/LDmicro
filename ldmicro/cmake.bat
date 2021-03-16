:DEL_LDMICRO
@del ldmicro.exe
@if exist ldmicro.exe echo   NOT DELETED ldmicro.exe !!!
@if exist ldmicro.exe PAUSE
@if exist ldmicro.exe goto DEL_LDMICRO

:DEL_LDMICRO_DEBUG
@del build\Debug\ldmicro.exe
@if exist build\Debug\ldmicro.exe echo   NOT DELETED build\Debug\ldmicro.exe !!!
@if exist build\Debug\ldmicro.exe PAUSE
@if exist build\Debug\ldmicro.exe goto DEL_LDMICRO_DEBUG

@SET PROGECT=D:\LDs\SVARKA2\svarka1.ld
@SET PROGECT=D:\LDs\SVARKA2\svarka2.ld
@SET PROGECT=D:\LDs\SVARKA2\svarka3.ld
:SVARKA1 - первый
@SET PROGECT=D:\LDs\SVARKA1\svarka.ld
:SVARKA2 - второй
@SET PROGECT=D:\LDs\SVARKA2\svarka.ld

@SET PROGECT=d:\lds\tmp\TMP.ld
@SET PROGECT=D:\LDs\uart\UART_M_ATmega2560_5.ld

@SET PROGECT=C:\LDs\ERR\TEST1.ld

@SET PROGECT="d:\lds\ATmega1284 44-Pin\ATmega1284 44-Pin.ld"
@SET PROGECT="d:\lds\ATmega1284 40-Pin\ATmega1284 40-Pin.ld"

@SET PROGECT=d:\lds\PresistTest\ATmega8\PresistTest.ld
@SET PROGECT=D:\lds\spi\228_ATmega2560.ld

if "%1" == "" goto DO_BUILD
@SET PROGECT="%1"
:DO_BUILD

md build
cd build
:cmake.exe ..
cmake.exe .. -G "Visual Studio 10 2010"
:cmake.exe .. -G "Visual Studio 12 2013"
:cmake.exe .. -G "Visual Studio 12 2013 Win64"
:cmake.exe .. -G "Visual Studio 14 2015"
:cmake.exe .. -G "Visual Studio 14 2015 Win64"
:cmake.exe .. -G "Visual Studio 15 2017"
:cmake.exe .. -G "Visual Studio 15 2017 Win64"
:cmake.exe .. -G "Visual Studio 16 2019" -A Win32
:cmake.exe .. -G "Visual Studio 16 2019" -A x64
@pause
cmake.exe --build .
cd ..

@if not exist build\bin\Debug\ldmicro.exe goto EXIT
start build\bin\Debug\ldmicro.exe %PROGECT%

:EXIT
