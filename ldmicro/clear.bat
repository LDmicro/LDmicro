@echo on
@echo This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo This file executes after menu Compile - F5
@echo 1. %1 - LD path without the end backslash, double quoted
@echo 2. %2 - LD file name without extentions and path, double quoted
@echo 3. %3 - Compile path without the end backslash, double quoted
@echo 4. %4 - Compile file name without extentions and path, double quoted
@echo 5. %5 -
@set P1=%~1
@set P2=%~2
@set P3=%~3
@set P4=%~4
@set P5=%~5
@echo P1. %P1%
@echo P2. %P2%
@echo P3. %P3%
@echo P4. %P4%
@echo P5. %P5%

@SET SL1="\"
if "%P1%" == "" SET SL1=""
@echo SL1 %SL1%

@SET SL3="\"
if "%P3%" == "" SET SL3=""
@echo SL3 %SL3%
;
del /S "%P1%%SL1%t.pl"
del /S "%P1%%SL1%t.pl"
del /S "%P1%%SL1%%P2%.pl"
del /S "%P3%%SL3%%P2%.pl"
;
del /S "%P1%%SL1%*.lst"
del /S "%P3%%SL3%*.lst"
;
del /S "%P1%%SL1%*.dbk"
del /S "%P3%%SL3%*.dbk"
del /S "%P1%%SL1%*.dmp"
del /S "%P3%%SL3%*.dmp"
del /S "%P1%%SL1%*.sdi"
del /S "%P3%%SL3%*.sdi"
;
del /S "%P1%%SL1%aaa"
del /S "%P3%%SL3%aaa"
del /S "%P1%%SL1%*.log"
del /S "%P3%%SL3%*.log"
del /S "%P1%%SL1%*.tmp"
del /S "%P3%%SL3%*.tmp"
del /S "%P1%%SL1%*.bak"
del /S "%P3%%SL3%*.bak"
;
attrib -R -A -S -H Thumbs.db /S
del /S Thumbs.db
;
del *.Chromium
del *.file
del *.Google
del *.LLVM
del *.Mozilla
del *.Webkit
;
goto exit
;
@rem pause
del /S "%P1%%SL1%*.hex"
del /S "%P3%%SL3%*.hex"
;
del /S "%P1%%SL1%*.asm"
del /S "%P3%%SL3%*.asm"
;
del /S "%P1%%SL1%*.ino"
del /S "%P3%%SL3%*.ino"
del /S "%P1%%SL1%*.ino_"
del /S "%P3%%SL3%*.ino_"
del /S "%P1%%SL1%*.h_"
del /S "%P3%%SL3%*.h_"
;
if exist ldmicro.cpp echo   NOT DELETED ldmicro.cpp !!!
if exist ldmicro.cpp echo   NOT DELETED ldmicro.cpp !!!
if exist ldmicro.cpp echo   NOT DELETED ldmicro.cpp !!!
;
if exist "%P1%%SL1%ldmicro.cpp" echo   NOT DELETED ldmicro.cpp !!!
if exist "%P1%%SL1%ldmicro.cpp" echo   NOT DELETED ldmicro.cpp !!!
if exist "%P1%%SL1%ldmicro.cpp" echo   NOT DELETED ldmicro.cpp !!!
;
if exist "%P3%%SL3%ldmicro.cpp" echo   NOT DELETED ldmicro.cpp !!!
if exist "%P3%%SL3%ldmicro.cpp" echo   NOT DELETED ldmicro.cpp !!!
if exist "%P3%%SL3%ldmicro.cpp" echo   NOT DELETED ldmicro.cpp !!!
pause
if exist ldmicro.cpp goto exit
if exist "%P1%%SL1%ldmicro.cpp" goto exit
if exist "%P3%%SL3%ldmicro.cpp" goto exit
;
del /S "%P1%%SL1%*.cpp"
del /S "%P3%%SL3%*.cpp"
del /S "%P1%%SL1%*.c"
del /S "%P3%%SL3%*.c"
del /S "%P1%%SL1%*.h"
del /S "%P3%%SL3%*.h"
;
:pauses
@echo USE:
@echo "clear.bat"
;
:error
@pause
:exit
