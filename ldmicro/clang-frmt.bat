:for /F %%F in ('dir /b *.i')   do clang-format.exe -i -style=file %%F
:for /F %%F in ('dir /b *.cpp') do clang-format.exe -i -style=file %%F
:for /F %%F in ('dir /b *.hpp') do clang-format.exe -i -style=file %%F
:for /F %%F in ('dir /b *.h')   do clang-format.exe -i -style=file %%F

:for /F %%F in ('dir /b LIBRARIES_FOR\ARDUINO\PwmFrequency\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\ARDUINO\PwmFrequency\%%F
:for /F %%F in ('dir /b LIBRARIES_FOR\ARM\STM32F1\*.c')          do clang-format.exe -i -style=file LIBRARIES_FOR\ARM\STM32F1\%%F
:for /F %%F in ('dir /b LIBRARIES_FOR\ARM\STM32F4\*.c')          do clang-format.exe -i -style=file LIBRARIES_FOR\ARM\STM32F4\%%F
:for /F %%F in ('dir /b LIBRARIES_FOR\AVR\*.c')                  do clang-format.exe -i -style=file LIBRARIES_FOR\AVR\%%F
:for /F %%F in ('dir /b LIBRARIES_FOR\PIC16\*.c')                do clang-format.exe -i -style=file LIBRARIES_FOR\PIC16\%%F
:for /F %%F in ('dir /b LIBRARIES_FOR\PIC18\*.c')                do clang-format.exe -i -style=file LIBRARIES_FOR\PIC18\%%F

rem for /F %%F in ('dir /b LIBRARIES_FOR\ARDUINO\PwmFrequency\*.h') do clang-format.exe -i -style=file LIBRARIES_FOR\ARDUINO\PwmFrequency\%%F
rem for /F %%F in ('dir /b LIBRARIES_FOR\ARM\STM32F1\*.h')          do clang-format.exe -i -style=file LIBRARIES_FOR\ARM\STM32F1\%%F
rem for /F %%F in ('dir /b LIBRARIES_FOR\ARM\STM32F4\*.h')          do clang-format.exe -i -style=file LIBRARIES_FOR\ARM\STM32F4\%%F
rem for /F %%F in ('dir /b LIBRARIES_FOR\AVR\*.h')                  do clang-format.exe -i -style=file LIBRARIES_FOR\AVR\%%F
rem for /F %%F in ('dir /b LIBRARIES_FOR\PIC16\*.h')                do clang-format.exe -i -style=file LIBRARIES_FOR\PIC16\%%F
