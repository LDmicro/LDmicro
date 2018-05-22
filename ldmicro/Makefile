
PERL=perl
LDLANG=LDLANG_EN

DEFINES = /D_WIN32_WINNT=0x500 /DISOLATION_AWARE_ENABLED /D_WIN32_IE=0x400 /DWIN32_LEAN_AND_MEAN /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE /D$(LDLANG)
CFLAGS  = /W3 /nologo -I. -I..\common\win32 -I..\common\gsl-lite\include -I..\common\gsl-lite\include\gsl -I..\common\stringarray -I..\common\utils /O2 /Zi  /EHsc /J

HEADERS = ..\common\win32\freeze.h ldmicro.h mcutable.h intcode.h

OBJDIR = obj

FREEZE   = $(OBJDIR)\freeze.obj

LDOBJS   = $(OBJDIR)\ldmicro.obj \
           $(OBJDIR)\intcode.obj \
           $(OBJDIR)\maincontrols.obj \
           $(OBJDIR)\helpdialog.obj \
           $(OBJDIR)\schematic.obj \
           $(OBJDIR)\draw.obj \
           $(OBJDIR)\draw_outputdev.obj \
           $(OBJDIR)\circuit.obj \
           $(OBJDIR)\undoredo.obj \
           $(OBJDIR)\loadsave.obj \
           $(OBJDIR)\simulate.obj \
           $(OBJDIR)\commentdialog.obj \
           $(OBJDIR)\contactsdialog.obj \
           $(OBJDIR)\coildialog.obj \
           $(OBJDIR)\simpledialog.obj \
           $(OBJDIR)\resetdialog.obj \
           $(OBJDIR)\lutdialog.obj \
           $(OBJDIR)\confdialog.obj \
           $(OBJDIR)\colorDialog.obj \
           $(OBJDIR)\iolist.obj \
           $(OBJDIR)\miscutil.obj \
           $(OBJDIR)\compilecommon.obj \
           $(OBJDIR)\ansic.obj \
           $(OBJDIR)\netzer.obj \
           $(OBJDIR)\interpreted.obj \
           $(OBJDIR)\xinterpreted.obj \
           $(OBJDIR)\pic16.obj \
           $(OBJDIR)\avr.obj \
           $(OBJDIR)\pascal.obj \
           $(OBJDIR)\pcports.obj \
           $(OBJDIR)\translit.obj \
           $(OBJDIR)\mcutable.obj \
           $(OBJDIR)\display.obj

LDLANGOBJ=$(OBJDIR)\lang.obj 

HELPOBJ  = $(OBJDIR)\helptext.obj

PRECOMP_H=stdafx.h
PRECOMP_CPP=stdafx.cpp
PRECOMP_OBJ=$(OBJDIR)\$(PRECOMP_CPP).obj
PRECOMP_PCH=$(OBJDIR)\stdafx.pch

LIBS = user32.lib gdi32.lib comctl32.lib advapi32.lib shell32.lib

all: dir $(OBJDIR)/ldmicro.exe $(OBJDIR)/ldinterpret.exe $(OBJDIR)/ldxinterpret.exe
    @copy $(OBJDIR)\ldmicro.exe .
    @copy $(OBJDIR)\ldinterpret.exe .
    @copy $(OBJDIR)\ldxinterpret.exe .
    @cd reg
    rem @go.bat
    @cd ..

clean:
    if not exist $(OBJDIR) mkdir $(OBJDIR)
    if exist $(OBJDIR) del /f /q $(OBJDIR)
	del /f /q *.pdb

lang.cpp: $(OBJDIR)/lang-tables.h

dir:
    if not exist $(OBJDIR) mkdir $(OBJDIR)

$(OBJDIR)/lang-tables.h: lang*.txt
    $(PERL) lang-tables.pl > $(OBJDIR)/lang-tables.h

$(OBJDIR)/helptext.cpp: manual.txt manual-*.txt
    $(PERL) txt2c.pl > $(OBJDIR)/helptext.cpp

$(OBJDIR)/ldinterpret.exe: ldinterpret.cpp
    @$(CC) $(CFLAGS) $(DEFINES) -c -Fo$(OBJDIR)/ldinterpret.cpp.obj ldinterpret.cpp
    @$(CC) $(CFLAGS) -Fe$(OBJDIR)/ldinterpret.exe $(LIBS) $(OBJDIR)/ldinterpret.cpp.obj

$(OBJDIR)/ldxinterpret.exe: ldxinterpret.cpp
    @$(CC) $(CFLAGS) $(DEFINES) -c -Fo$(OBJDIR)/ldxinterpret.cpp.obj ldinterpret.cpp
    @$(CC) $(CFLAGS) -Fe$(OBJDIR)/ldxinterpret.exe $(LIBS) $(OBJDIR)/ldxinterpret.cpp.obj

$(OBJDIR)/ldmicro.exe: $(PRECOMP_OBJ) $(LDOBJS) $(LDLANGOBJ) $(FREEZE) $(HELPOBJ) $(OBJDIR)/ldmicro.res
    echo @$(CC) $(DEFINES) $(CFLAGS) -Fe$(OBJDIR)/ldmicro.exe $(LDOBJS) $(FREEZE) $(HELPOBJ) $(OBJDIR)/ldmicro.res $(LIBS)
    @$(CC) $(DEFINES) $(CFLAGS) -Fe$(OBJDIR)/ldmicro.exe $(LDOBJS) $(LDLANGOBJ) $(PRECOMP_OBJ) $(FREEZE) $(HELPOBJ) $(OBJDIR)/ldmicro.res $(LIBS)

$(OBJDIR)/ldmicro.res: ldmicro.rc ldmicro.ico
    @rc ldmicro.rc
    @copy ldmicro.res $(OBJDIR)

$(PRECOMP_OBJ):  stdafx.h
    @echo $(CC) $(CFLAGS) $(DEFINES) /Fp$(PRECOMP_PCH) /Yc$(PRECOMP_H) -c -Fo$(PRECOMP_OBJ) $(PRECOMP_CPP)
    @$(CC) $(CFLAGS) $(DEFINES) /Fp$(PRECOMP_PCH) /Yc$(PRECOMP_H) -c -Fo$(PRECOMP_OBJ) $(PRECOMP_CPP)

$(LDOBJS): $(@B).cpp $(HEADERS)
    @echo $(CC) $(CFLAGS) $(DEFINES) /Yustdafx.h /Fp$(PRECOMP_PCH) -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp
    @$(CC) $(CFLAGS) $(DEFINES) /FI$(PRECOMP_H) /Fp$(PRECOMP_PCH) /Yu$(PRECOMP_H) -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp

$(FREEZE): ..\common\win32\$(@B).cpp $(HEADERS)
    @$(CC) $(CFLAGS) $(DEFINES) /Yu$(PRECOMP_H) /Fp$(PRECOMP_PCH) -c -Fo$(OBJDIR)/$(@B).obj ..\common\win32\$(@B).cpp

$(LDLANGOBJ): $(@B).cpp
    @echo $(CC) $(CFLAGS) $(DEFINES) -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp
    @$(CC) $(CFLAGS) $(DEFINES) -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp

$(HELPOBJ): $(OBJDIR)/helptext.cpp
    @$(CC) $(CFLAGS) $(DEFINES) -c -Fo$(OBJDIR)/helptext.obj $(OBJDIR)/helptext.cpp


