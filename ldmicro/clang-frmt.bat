rem for /F %%F in ('dir /b *.cpp') do clang-format.exe -i -style=file %%F

for /F %%F in ('dir /b LIBRARIES_FOR\AVR\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\AVR\%%F
for /F %%F in ('dir /b LIBRARIES_FOR\ARM\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\ARM\%%F
for /F %%F in ('dir /b LIBRARIES_FOR\PIC16\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR\PIC16\%%F

for /F %%F in ('dir /b LIBRARIES_FOR_zip\AVR\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\AVR\%%F
for /F %%F in ('dir /b LIBRARIES_FOR_zip\ARM\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\ARM\%%F
for /F %%F in ('dir /b LIBRARIES_FOR_zip\PIC16\*.c') do clang-format.exe -i -style=file LIBRARIES_FOR_zip\PIC16\%%F

rem for /F %%F in ('dir /b *.h'  ) do clang-format.exe -i -style=file %%F
