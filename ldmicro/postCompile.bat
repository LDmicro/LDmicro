@echo on
@rem This file is part of LDmicro project and must be located in same directory where LDmicro.exe located.
@echo %1 - LD path
@echo %2 - LD file name without extentions and path
goto ARDUINO
@rem -----------------------------------------------------------------------
:ARDUINO
@md "%1ARDUINO"
@md "%1ARDUINO\%2"
copy "%1%2.cpp"      "%1ARDUINO\%2"
copy "%1%2.h"        "%1ARDUINO\%2"
copy "%1\ladder.h_"  "%1ARDUINO\%2"
copy "%1%2.ino_"     "%1ARDUINO\%2"
@rem -----------------------------------------------------------------------
@rem @pause
