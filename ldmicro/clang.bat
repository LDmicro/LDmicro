clang-format.exe -style=file     %1 > %1.file
:clang-format.exe -style=LLVM     %1 > %1.LLVM
:clang-format.exe -style=Google   %1 > %1.Google
:clang-format.exe -style=Chromium %1 > %1.Chromium
:clang-format.exe -style=Mozilla  %1 > %1.Mozilla
:clang-format.exe -style=Webkit   %1 > %1.Webkit
