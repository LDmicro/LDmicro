#!/usr/bin/perl

sub SYS { system($_[0]); }

SYS("rmdir buildXXXX");
SYS("rm -rf buildXXXX");
SYS("mkdir buildXXXX");

foreach $f (qw(DE ES FR IT PT TR RU JA EN)) {
    SYS("del ldmicro.exe");
    SYS("nmake clean");
    print $f;
    SYS("nmake LDLANG=LDLANG_$f");
    $fl = lc($f);
    SYS("copy ldmicro.exe buildXXXX\\ldmicro-$fl.exe");
    #SYS("pause")
}
SYS("copy ldmicro.exe buildXXXX\\ldmicro.exe");

SYS("copy COPYING.txt     buildXXXX");
SYS("copy CHANGES.txt     buildXXXX");
SYS("copy manual*.txt     buildXXXX");
SYS("copy clear.bat       buildXXXX");
SYS("copy notepad.bat     buildXXXX");
SYS("copy readmcu.bat     buildXXXX");
SYS("copy flashmcu.bat    buildXXXX");
SYS("copy postCompile.bat buildXXXX");
SYS("copy buildarm.bat    buildXXXX");
SYS("copy buildavr.bat    buildXXXX");
SYS("copy buildCCS.bat    buildXXXX");
SYS("copy buildPic16.bat  buildXXXX");
SYS("copy buildPic18.bat  buildXXXX");

SYS("mkdir buildXXXX\\LIBRARIES_FOR");
SYS("mkdir buildXXXX\\LIBRARIES_FOR\\ARDUINO");
SYS("mkdir buildXXXX\\LIBRARIES_FOR\\ARDUINO\\PwmFrequency");
#SYS("copy  LIBRARIES_FOR\\ARDUINO\\PwmFrequency\\PwmFrequency.h   buildXXXX\\LIBRARIES_FOR\\ARDUINO\\PwmFrequency");
#SYS("copy  LIBRARIES_FOR\\ARDUINO\\PwmFrequency\\PwmFrequency.cpp buildXXXX\\LIBRARIES_FOR\\ARDUINO\\PwmFrequency");

SYS("mkdir buildXXXX\\LIBRARIES_FOR\\ARM");
SYS("mkdir buildXXXX\\LIBRARIES_FOR\\AVR");
SYS("mkdir buildXXXX\\LIBRARIES_FOR\\PIC16");

SYS("xcopy /s LIBRARIES_FOR\\*.* buildXXXX\\LIBRARIES_FOR");

SYS("python.exe 7zip.py");
