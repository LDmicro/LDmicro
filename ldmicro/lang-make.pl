#!/usr/bin/perl

$engl = 1;
while(<>) {
    chomp;

    if(/^\s*$/) {
        if($engl) {
            next;
        } else {
            die "blank line mid-translation at $file, $.\n";
        }
    }

    if($engl) {
        $toTranslate = $_;
        $engl = 0;
    } else {
        $translated = $_;
        
        $toTranslate =~ s/\0/\\"/g;
        $toTranslate =~ s#"##g;
        $Already{$toTranslate} = 1;
        $engl = 1;
    }
}

# A tool to generate a list of strings to internationalize

for $file (<*.cpp>) {
    open(IN, $file) or die;
    $source = join("", <IN>);
    close IN;

    $source =~ s/\\"/\0/g;

    while($source =~ m#
        _\(  \s*
            ((
                \s* "
                    [^"]+
                " \s*
            )+)
        \s*\)
    #xsg) {
        $str = $1;
        $strIn = '';
        while($str =~ m#\s*"([^"]+)"\s*#g) {
            $strIn .= $1;
        }
        $Occurs{$strIn} = $c++;
        $String{$strIn}++;
    }
}

sub by_occurrence {
    $Occurs{$a} <=> $Occurs{$b};
}

for (sort by_occurrence (keys %String)) {
    if (not $Already{$_}) {
        $_ =~ s/\0/\\"/g;
        print qq{"$_"\n\n\n};
    }
}

