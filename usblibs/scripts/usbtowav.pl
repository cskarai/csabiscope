#!/usr/bin/perl

use strict;
use warnings;

use Data::Dumper;

my $infile = shift @ARGV;
my $outfile = shift @ARGV;

die "Usage: usbtowav.pl" if ! $infile || ! $outfile;

my $filesize = -s $infile;

open( FH, "<", $infile ) or die "Can't open file to read: $!";

my $line = <FH>;
die "Invalid file format!" if $line !~ /Bits: (\d+)/;
my $bits = $1;

$line = <FH>;
die "Invalid file format!" if $line !~ /Channels: (\d+)/;
my $channels = $1;

$line = <FH>;
die "Invalid file format!" if $line !~ /Period: (\d+)/;
my $rate = $1;

$line = <FH>;
die "Invalid file format!" if ! $line;

my $pos = tell(FH);

my $samples = $filesize - $pos;

my $samplesize = 2;
$samplesize = 3 if $bits == 12;
$samplesize *= 2 if $channels == 2;

$samples *= 2;
$samples /= $samplesize;
$samples = int($samples);

binmode FH;

open( WH, ">", $outfile ) or die "Can't open file to write: $!";
binmode WH;

print WH "RIFF";

my $wavfilesize = $samples;
$wavfilesize *= 2 if $bits == 12;
$wavfilesize *= 2 if $channels == 2;

print WH pack("V", $wavfilesize + 44 - 8);
print WH "WAVEfmt ";
print WH pack("V", 16); # chunk size
print WH pack("v", 1);  # PCM
print WH pack("v", $channels); # number of channels

my @validfreqlist = (384000, 352800, 192000, 176400, 96000, 88200, 48000, 44100, 22050, 16000, 11025, 8000);
my $freq = 1000000000 / $rate;

my @dist = map { [ $_, abs($_ - $freq) ] } @validfreqlist;
@dist = sort { $a->[1] <=> $b->[1] } @dist;

$freq = $dist[0][0];

print WH pack("V", $freq); # chunk size

my $bytesPerSample = (($bits == 12) ? 2 : 1);
my $byteRate = $freq * $channels * $bytesPerSample;

print WH pack("V", $byteRate); # chunk size
print WH pack("v", $bytesPerSample); # bytes for a sample
print WH pack("v", 8 * $bytesPerSample); # bytes for a sample
print WH 'data';
print WH pack("V", $wavfilesize);

my $remaining = '';
my $buffer;
while( sysread(FH , $buffer , 4096) )
{
  process_buffer( $buffer);
}

close FH;
close WH;



sub process_buffer
{
  my ($buffer) = @_;
  
  $buffer = $remaining . $buffer;

  if( $bits == 12 )
  {
    while( length($buffer) >= 3 )
    {
      my $byte1 = ord( substr($buffer, 0, 1));
      my $byte2 = ord( substr($buffer, 1, 1));
      my $byte3 = ord( substr($buffer, 2, 1));
        
      my $ch1 = 16 * (($byte1 << 4) + ($byte2 >> 4) - 0x800);
      my $ch2 = 16 * ((($byte2 & 15) << 8) + $byte3 - 0x800);
      print WH pack("v", $ch1);
      print WH pack("v", $ch2);
      substr($buffer, 0, 3) = '';
    }
  }
  else
  {
    if( $channels == 1 )
    {
      while( length($buffer) >= 1 )
      {
        print WH substr($buffer, 0, 1);
        substr($buffer, 0, 1) = '';
      }
    }
    else
    {
      while( length($buffer) >= 2 )
      {
        print WH substr($buffer, 0, 2);
        substr($buffer, 0, 2) = '';
      }
    }
  }

  $remaining = $buffer;
 }
