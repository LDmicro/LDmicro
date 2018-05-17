#!/usr/bin/perl

sub SYS { system($_[0]); }

SYS("rmdir build_");
SYS("rm -rf build_");
SYS("mkdir build_");
SYS("mkdir obj");

foreach $f (qw(DE ES FR IT PT TR RU JA EN)) {
    SYS("nmake clean");
    print $f;
    SYS("nmake D=LDLANG_$f");
    $fl = lc($f);
    SYS("copy ldmicro.exe build_\\ldmicro-$fl.exe");
}
SYS("copy ldmicro.exe build_\\ldmicro.exe");

SYS("copy COPYING.txt     build_");
SYS("copy CHANGES.txt     build_");
SYS("copy manual*.txt     build_");
SYS("copy clear.bat       build_");
SYS("copy notepad.bat     build_");
SYS("copy readmcu.bat     build_");
SYS("copy flashmcu.bat    build_");
SYS("copy postCompile.bat build_");

SYS("mkdir build_\\LIBRARIES_FOR");
SYS("mkdir build_\\LIBRARIES_FOR\\ARDUINO");
SYS("mkdir build_\\LIBRARIES_FOR\\ARDUINO\\PwmFrequency");
SYS("copy  LIBRARIES_FOR\\ARDUINO\\PwmFrequency\\PwmFrequency.h   build_\\LIBRARIES_FOR\\ARDUINO\\PwmFrequency");
SYS("copy  LIBRARIES_FOR\\ARDUINO\\PwmFrequency\\PwmFrequency.cpp build_\\LIBRARIES_FOR\\ARDUINO\\PwmFrequency");
