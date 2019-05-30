rem for /F %%F in ('dir /b *.i') do clang-format.exe -i -style=file %%F
rem for /F %%F in ('dir /b *.cpp') do clang-format.exe -i -style=file %%F

for /F %%F in ('dir /b LIBRARIES_FOR\ARM\STM32F1\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\ARM\STM32F1\%%F
for /F %%F in ('dir /b LIBRARIES_FOR\AVR\STM32F4\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\AVR\STM32F4\%%F
for /F %%F in ('dir /b LIBRARIES_FOR\PIC16\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\PIC16\%%F

for /F %%F in ('dir /b LIBRARIES_FOR_zip\ARM\STM32F1\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\ARM\STM32F1\%%F
for /F %%F in ('dir /b LIBRARIES_FOR_zip\ARM\STM32F4\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\ARM\STM32F4\%%F
for /F %%F in ('dir /b LIBRARIES_FOR_zip\AVR\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\AVR\%%F
for /F %%F in ('dir /b LIBRARIES_FOR_zip\PIC16\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\PIC16\%%F

rem for /F %%F in ('dir /b *.h'  ) do clang-format.exe -i -style=file %%F
