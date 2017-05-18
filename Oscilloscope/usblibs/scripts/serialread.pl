#!/usr/bin/perl

use strict;
use warnings;
use IO::Handle;
use threads;
use threads::shared;

use Data::Dumper;

my $device = shift @ARGV;
my $file = shift @ARGV;

if( ! defined $file || ! defined $device )
{
  print STDERR "Usage: serialread.pl /dev/ttyACM0 outfile.txt\n";
  exit(0);
}

system("stty -F $device raw");

my @chunks : shared;
my $threadRunning : shared = 1;

open( DH, "<", $device ) or die "Can't open device for read: $!\n";
binmode DH;

my $thread = threads->create(
  sub {
    my $printed = 0;

    open( FH, ">", $file ) or die "Can't open file for write: $!\n";
    binmode FH;
    
    while($threadRunning)
    {
      my $data = shift @chunks;
      if( defined $data )
      {
        print FH "$data";
        $printed = 1;
      }
      elsif( $printed )
      {
        FH->flush();
        $printed = 0;
      }
    }
    
    close FH;
  }
);


my $bufsize = 20_000_000;
my $buffer;

while ( my $read = sysread(DH , $buffer , $bufsize) )
{
  push @chunks, $buffer;
}

close DH;

while( @chunks ) {}

$threadRunning = 0;
$thread->join();
