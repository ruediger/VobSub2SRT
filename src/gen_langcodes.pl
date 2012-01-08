#!/usr/bin/perl
#
#  This file is part of vobsub2srt
#
#  Copyright (C) 2012 Rüdiger Sonderfeld <ruediger@c-plusplus.de>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
## Comment:
#
# This script was used to generate parts of langcodes.c++. Get the list of codes
# from http://www.sil.org/iso639-3/download.asp
# e.g. wget http://www.sil.org/iso639-3/iso-639-3_20110525.tab
#
# I'm not a big Perl hacker and this implementation probably sucks. Last time I
# used emacs keyboard macros. But this is no good if you have to repeat the
# procedure a while later. So I wanted to write a script. I hadn't done anything
# in Perl in a long time (~10 years?) so I figured I use Perl to do the job
# (instead of Bash or Python).
#
use warnings;
use strict;

my $filename = 'iso-639-3_20110525.tab';
my @iso639;

open FH, $filename or die $!;
<FH>; # skip first line
while(<FH>) {
  my @fields = split /\t/, $_;
  if($fields[3] ne '') {
    push(@iso639, [$fields[3], $fields[0]] );
  }
}

# keep sorted for binary search
my @iso639_sorted = sort {@{$a}[0] cmp @{$b}[0] } @iso639;

#foreach (@iso639_sorted) {
#  print @{$_}[0],' ',@{$_}[1],"\n";
#}

print 'static char const *const iso639_1[] = {',"\n";
foreach (@iso639_sorted) {
  print '  "',@{$_}[0],"\",\n";
}
print "};\n\n";

print 'static char const *const iso639_3[] = {',"\n";
foreach (@iso639_sorted) {
  print '  "',@{$_}[1],"\",\n";
}
print "};\n";
