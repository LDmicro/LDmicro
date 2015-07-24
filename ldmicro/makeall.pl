#!/usr/bin/perl

sub SYS { system($_[0]); }

SYS("rm -rf build");
SYS("mkdir build");

for $f qw(DE ES FR IT PT TR) {
    SYS("nmake clean");
    SYS("nmake D=LDLANG_$f");
    $fl = lc($f);
    SYS("copy ldmicro.exe build\\ldmicro-$fl.exe");
}

SYS("nmake clean");
SYS("nmake D=LDLANG_EN");
SYS("copy ldmicro.exe build\\ldmicro.exe");
