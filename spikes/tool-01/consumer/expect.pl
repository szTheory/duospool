#!/usr/bin/perl
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# expect.pl - independent computation of the W-07 digest for the W-11 golden line (TOOL-01 spike).
# usage: perl expect.pl <fixture-path>
#   Reads a 65536-byte fixture, folds its bytes into FNV-1a 64, then applies the steps 100, 200, 300
#   (each adds to the machine time and folds the 8 little-endian bytes of the new time).
# stdout: abi=1 load=OK time=600 digest=<16 hex>
# This is a second implementation of the spec written in Perl, not DuoSpool output. Core Perl only.
# 64-bit state is held as two 32-bit halves so that no multiplication leaves exact integer range:
# multiplying by the FNV prime 0x100000001b3 = 2^40 + 0x1b3 mod 2^64 is (h << 40) + h * 0x1b3.
use strict;
use warnings;

my $M32 = 0xFFFFFFFF;
my ($hi, $lo) = (0xcbf29ce4, 0x84222325);

sub fold_byte {
    my ($b) = @_;
    $lo ^= $b;
    my $lo_prod = $lo * 0x1b3;                       # below 2^41
    my $carry   = int($lo_prod / 4294967296);
    my $new_lo  = $lo_prod & $M32;
    my $new_hi  = ($hi * 0x1b3 + $carry + (($lo << 8) % 1099511627776)) & $M32;
    ($hi, $lo) = ($new_hi, $new_lo);
}

my $path = shift(@ARGV);
die "usage: perl expect.pl <fixture-path>\n" unless defined $path;
open(my $fh, '<:raw', $path) or die "cannot open $path: $!\n";
my $buf = '';
{
    local $/ = undef;
    $buf = <$fh>;
}
close($fh);
die "fixture is not 65536 bytes\n" unless length($buf) == 65536;

fold_byte($_) for unpack('C*', $buf);

my $time = 0;
for my $cycles (100, 200, 300) {
    $time += $cycles;
    my $t = $time;
    for (1 .. 8) {
        fold_byte($t & 0xFF);
        $t = int($t / 256);
    }
}
printf("abi=1 load=OK time=%d digest=%08x%08x\n", $time, $hi, $lo);
