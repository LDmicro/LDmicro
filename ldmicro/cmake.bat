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
:SVARKA2 - второй
@SET PROGECT=D:\LDs\SVARKA2\svarka.ld
:SVARKA1 - первый
@SET PROGECT=D:\LDs\SVARKA1\svarka.ld

if "%1" == "" goto DO_BUILD
@SET PROGECT="%1"
:DO_BUILD

md build
cd build
cmake.exe ..
cmake.exe --build .
cd ..

@if not exist build\bin\Debug\ldmicro.exe goto EXIT
start build\bin\Debug\ldmicro.exe %PROGECT%

:EXIT
