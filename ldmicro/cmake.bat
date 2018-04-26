:DEL_LDMICRO
@del ldmicro.exe
@if exist ldmicro.exe echo   NOT DELETED ldmicro.exe !!!
@if exist ldmicro.exe PAUSE
@if exist ldmicro.exe goto DEL_LDMICRO

@del build\Debug\ldmicro.exe
@if exist build\Debug\ldmicro.exe echo   NOT DELETED build\Debug\ldmicro.exe !!!
@if exist build\Debug\ldmicro.exe PAUSE
@if exist build\Debug\ldmicro.exe goto DEL_LDMICRO

@SET PROGECT=e:\SVARKA2\svarka1.ld
@SET PROGECT=e:\SVARKA2\svarka2.ld
@SET PROGECT=e:\SVARKA2\svarka3.ld
@SET PROGECT=e:\SVARKA2\tests.ld
@SET PROGECT=e:\SVARKA2\svarka.ld

if "%1" == "" goto DO_BUILD
@SET PROGECT="%1"
:DO_BUILD

md build
cd build
cmake.exe ..
cmake.exe --build .
cd ..

copy build\Debug\ldmicro.exe .
start ldmicro.exe %PROGECT%
