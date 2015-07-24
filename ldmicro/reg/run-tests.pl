#!/usr/bin/perl

if (not -d 'results/') {
    mkdir 'results';
}

$c = 0;
for $test (<tests/*.ld>) {
    $output = $test;
    $output =~ s/^tests/results/;
    $output =~ s/\.ld$/.hex/;

    unlink $output;

    $cmd = "../ldmicro.exe /c $test $output";
    system $cmd;
    $c++;
}

print "\ndifferences follow:\n";
@diff = `diff -q results expected`;
for(@diff) {
    print "    $_";
}
$fc = scalar @diff;
print "($fc difference(s)/$c)\n";
if($fc == 0) {
    print "pass!\n";
    exit(0);
} else {
    print "FAIL\n";
    exit(-1);
}
