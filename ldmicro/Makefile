PERL=perl
LDLANG=LDLANG_EN

DEFINES = /D_WIN32_WINNT=0x500 /DISOLATION_AWARE_ENABLED /D_WIN32_IE=0x400 /DWIN32_LEAN_AND_MEAN /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE /D$(LDLANG)

INCLUDES = -I. -I..\common\win32 -I..\common\gsl-lite\include -I..\common\gsl-lite\include\gsl -I..\common\stringarray -I..\common\utils -I.\ldlog\include
#CFLAGS  =  /P /Fi preprocessor output
#CFLAGS  =  /MDd /Ob0 /Od /Zi /W3 /nologo /EHsc /J
CFLAGS  =  /MD /O2 /Zi /W3 /nologo /EHsc /J

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
           $(OBJDIR)\compilercommon.obj \
           $(OBJDIR)\ansic.obj \
           $(OBJDIR)\netzer.obj \
           $(OBJDIR)\interpreted.obj \
           $(OBJDIR)\xinterpreted.obj \
           $(OBJDIR)\pic16.obj \
           $(OBJDIR)\avr.obj \
           $(OBJDIR)\accel.obj \
           $(OBJDIR)\pascal.obj \
           $(OBJDIR)\pcports.obj \
           $(OBJDIR)\plcprogram.obj\
           $(OBJDIR)\translit.obj \
           $(OBJDIR)\mcutable.obj \
           $(OBJDIR)\display.obj

LDLANGOBJ=$(OBJDIR)\lang.obj

HELPOBJ  = $(OBJDIR)\helptext.obj

LDLOGSRC =  $(OBJDIR)\..\ldlog\src\logger.cpp \
            $(OBJDIR)\..\ldlog\src\winerror.cpp \
            $(OBJDIR)\..\ldlog\src\sinks\consolesink.cpp \
            $(OBJDIR)\..\ldlog\src\sinks\windowsdebugstringsink.cpp \
            $(OBJDIR)\..\ldlog\src\sinks\windowsmessagesink.cpp \
            $(OBJDIR)\..\ldlog\src\fmt\format.cc \
            $(OBJDIR)\..\ldlog\src\fmt\ostream.cc \
            $(OBJDIR)\..\ldlog\src\fmt\posix.cc \
            $(OBJDIR)\..\ldlog\src\fmt\printf.cc

LDLOGOBJ =  $(OBJDIR)\logger.cpp.obj \
            $(OBJDIR)\winerror.cpp.obj \
            $(OBJDIR)\consolesink.cpp.obj \
            $(OBJDIR)\windowsdebugstringsink.cpp.obj \
            $(OBJDIR)\windowsmessagesink.cpp.obj \
            $(OBJDIR)\format.cc.obj \
            $(OBJDIR)\ostream.cc.obj \
            $(OBJDIR)\posix.cc.obj \
            $(OBJDIR)\printf.cc.obj


PRECOMP_H=stdafx.h
PRECOMP_CPP=stdafx.cpp
PRECOMP_OBJ=$(OBJDIR)\$(PRECOMP_CPP).obj
PRECOMP_PCH=$(OBJDIR)\stdafx.pch

LIBS = user32.lib gdi32.lib comctl32.lib advapi32.lib shell32.lib kernel32.lib

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
    @echo $(DEFINES) > $(OBJDIR)/defs.rsp
    @echo $(CFLAGS)  > $(OBJDIR)/flags.rsp
    @echo $(INCLUDES) > $(OBJDIR)/inc.rsp
    @echo $(LIBS)     > $(OBJDIR)/libs.rsp

$(OBJDIR)/lang-tables.h: lang*.txt
    $(PERL) lang-tables.pl > $(OBJDIR)/lang-tables.h

$(OBJDIR)/helptext.cpp: manual.txt manual-*.txt
    $(PERL) txt2c.pl > $(OBJDIR)/helptext.cpp

$(OBJDIR)/ldinterpret.exe: ldinterpret.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/ldinterpret.cpp.obj ldinterpret.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp -Fe$(OBJDIR)/ldinterpret.exe $(LIBS) $(OBJDIR)/ldinterpret.cpp.obj

$(OBJDIR)/ldxinterpret.exe: ldxinterpret.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/ldxinterpret.cpp.obj ldxinterpret.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp -Fe$(OBJDIR)/ldxinterpret.exe $(LIBS) $(OBJDIR)/ldxinterpret.cpp.obj

$(OBJDIR)/ldmicro.exe: $(PRECOMP_OBJ) $(LDOBJS) $(LDLOGOBJ) $(LDLANGOBJ) $(FREEZE) $(HELPOBJ) $(OBJDIR)/ldmicro.res
    @echo @$(CC) @$(OBJDIR)/defs.rsp $(CFLAGS) @$(OBJDIR)/inc.rsp -Fe$(OBJDIR)/ldmicro.exe $(LDOBJS) $(LDLOGOBJ) $(FREEZE) $(HELPOBJ) $(OBJDIR)/ldmicro.res $(LIBS)
    @$(CC) @$(OBJDIR)/defs.rsp $(CFLAGS) @$(OBJDIR)/inc.rsp -Fe$(OBJDIR)/ldmicro.exe $(LDOBJS) $(LDLANGOBJ) $(LDLOGOBJ) $(PRECOMP_OBJ) $(FREEZE) $(HELPOBJ) $(OBJDIR)/ldmicro.res $(LIBS)

##########################################################
$(OBJDIR)\logger.cpp.obj : $(OBJDIR)\..\ldlog\src\logger.cpp
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\logger.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\logger.cpp

$(OBJDIR)\winerror.cpp.obj : $(OBJDIR)\..\ldlog\src\winerror.cpp
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\winerror.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\winerror.cpp

$(OBJDIR)\consolesink.cpp.obj : $(OBJDIR)\..\ldlog\src\sinks\consolesink.cpp
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\sinks\consolesink.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\sinks\consolesink.cpp

$(OBJDIR)\windowsdebugstringsink.cpp.obj : $(OBJDIR)\..\ldlog\src\sinks\windowsdebugstringsink.cpp
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\sinks\windowsdebugstringsink.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\sinks\windowsdebugstringsink.cpp

$(OBJDIR)\windowsmessagesink.cpp.obj : $(OBJDIR)\..\ldlog\src\sinks\windowsmessagesink.cpp
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\sinks\windowsmessagesink.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\sinks\windowsmessagesink.cpp

$(OBJDIR)\format.cc.obj : $(OBJDIR)\..\ldlog\src\fmt\format.cc
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\format.cc
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\format.cc

$(OBJDIR)\ostream.cc.obj : $(OBJDIR)\..\ldlog\src\fmt\ostream.cc
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\ostream.cc
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\ostream.cc

$(OBJDIR)\posix.cc.obj : $(OBJDIR)\..\ldlog\src\fmt\posix.cc
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\posix.cc
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\posix.cc

$(OBJDIR)\printf.cc.obj : $(OBJDIR)\..\ldlog\src\fmt\printf.cc
    @echo @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\printf.cc
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@F) $(OBJDIR)\..\ldlog\src\fmt\printf.cc

############################################################

$(OBJDIR)/ldmicro.res: ldmicro.rc ldmicro.ico
    @rc ldmicro.rc
    @copy ldmicro.res $(OBJDIR)

$(PRECOMP_OBJ):  stdafx.h
    @echo $(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp /Fp$(PRECOMP_PCH) /Yc$(PRECOMP_H) -c -Fo$(PRECOMP_OBJ) $(PRECOMP_CPP)
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp  @$(OBJDIR)/defs.rsp /Fp$(PRECOMP_PCH) /Yc$(PRECOMP_H) -c -Fo$(PRECOMP_OBJ) $(PRECOMP_CPP)

$(LDOBJS): $(@B).cpp $(HEADERS)
    @echo $(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp /Yustdafx.h /Fp$(PRECOMP_PCH) -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp /FI$(PRECOMP_H) /Fp$(PRECOMP_PCH) /Yu$(PRECOMP_H) -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp

$(FREEZE): ..\common\win32\$(@B).cpp $(HEADERS)
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp /Yu$(PRECOMP_H) /Fp$(PRECOMP_PCH) -c -Fo$(OBJDIR)/$(@B).obj ..\common\win32\$(@B).cpp

$(LDLANGOBJ): $(@B).cpp
    @echo $(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/$(@B).obj $(@B).cpp

$(HELPOBJ): $(OBJDIR)/helptext.cpp
    @$(CC) $(CFLAGS) @$(OBJDIR)/inc.rsp @$(OBJDIR)/defs.rsp -c -Fo$(OBJDIR)/helptext.obj $(OBJDIR)/helptext.cpp
