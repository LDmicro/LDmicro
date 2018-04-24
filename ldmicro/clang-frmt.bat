for /F %%F in ('dir /b *.cpp') do clang-format.exe -i -style=file %%F
rem for /F %%F in ('dir /b *.h'  ) do clang-format.exe -i -style=file %%F
