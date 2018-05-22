rem @SET PROGECT=

:DEL_LDMICRO
@del ldmicro.exe
@if exist ldmicro.exe echo   NOT DELETED ldmicro.exe !!!
@if exist ldmicro.exe PAUSE
@if exist ldmicro.exe goto DEL_LDMICRO

@rem @nmake clean
@rem @nmake LDLANG=LDLANG_RU %*
@rem @nmake LDLANG=LDLANG_TR %*
@nmake LDLANG=LDLANG_EN %*

@if exist ldmicro.exe start ldmicro.exe %PROGECT%
