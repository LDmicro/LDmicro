@echo See details inside lang-sort.py
:DEL_LDMICRO
@del lang.txt
@if exist lang.txt echo   NOT DELETED lang.txt !!!
@if exist lang.txt PAUSE
@if exist lang.txt goto DEL_LDMICRO
lang-make.pl lang.txt > lang.txt
@rem
@rem lang-sort.py [NOSORT] | [NOADD]
lang-sort.py