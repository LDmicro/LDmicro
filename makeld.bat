
set "VSCMD_START_DIR=%CD%"

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
echo Set environ

cd ldmicro

PATH=d:\bin\msys64\usr\bin;%PATH%

call make.bat