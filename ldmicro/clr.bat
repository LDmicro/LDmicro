md BAK
move  *.bak BAK
move  *.orig BAK

del lang.txt

del _*.fil
del *.log
del *.obj
attrib -R -A -S -H Thumbs.db /S
del /S Thumbs.db
del /S *.pas
del acceleration_deceleration.txt
del ldmicro.clp
del ldmicro.tmp
del ldmicro.exe
del ldmicro.res
del ldinterpret.exe
del ldxinterpret.exe
del /S aaa
del *.hex

@rem Cpp Express/Studio
del /S *.cdf
del /S *.cache
del /S *.obj
del /S *.ilk
del /S *.resources
del /S *.tlb
del /S *.tli
del /S *.tlh
del /S *.tmp
del /S *.rsp
del /S *.pgc
del /S *.pgd
del /S *.meta
del /S *.tlog
del /S *.res
del /S *.pch
del /S *.exp
del /S *.idb
del /S *.rep
del /S *.xdc
del /S *_manifest.rc
del /S *.bsc
del /S *.sbr
del /S *.xml
