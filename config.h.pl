#!/usr/bin/perl -w
use strict;
use warnings;

my $debug = 0;

print("// Automatically generated. Do not edit.\n");
print("#ifndef __CONFIG_GEN_H__\n");
print("#define __CONFIG_GEN_H__\n");
do
{
    my $hash = getGithash();
    my $date = getDate();
    my $time = getTime();
    my $year = substr($date, 0, 4);

    while (my $line = <STDIN>)
    {
        $line =~ s{%HASH%}{$hash}g;
        $line =~ s{%DATE%}{$date}g;
        $line =~ s{%TIME%}{$time}g;
        $line =~ s{%YEAR%}{$year}g;
        print($line);
    }
};
print("#endif\n");

sub getDate
{
    my (undef, undef, undef, $mday, $mon, $year) = localtime();
    return sprintf('%04i-%02i-%02i', $year + 1900, $mon + 1, $mday);
}

sub getTime
{
    my ($sec, $min, $hour) = localtime();
    return sprintf('%02i:%02i:%02i', $hour, $min, $sec);
}

sub getGithash
{
    my ($hash, $dirty);
    if ($^O =~ m/Win/i)
    {
        $hash = qx{git log --pretty=format:%H -n1};
        $dirty = qx{git describe --always --dirty};
    }
    else
    {
        $hash = qx{LC_ALL=C git log --pretty=format:%H -n1 2>/dev/null};
        $dirty = qx{LC_ALL=C git describe --always --dirty 2>/dev/null};
    }
    $hash =~ s{\r?\n}{};
    $dirty =~ s{\r?\n}{};
    print(STDERR "hash=[$hash]\ndirty=[$dirty]\n") if ($debug);

    if ($hash && $dirty)
    {
        my $str = substr($hash, 0, 8);
        if ($dirty =~ m{-dirty$})
        {
            $str .= 'M' ;
        }
        return $str;
    }
    else
    {
        return "00000000";
    }
}

# eof
