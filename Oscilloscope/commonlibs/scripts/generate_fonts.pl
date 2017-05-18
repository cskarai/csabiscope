#!/usr/bin/perl

use strict;
use Data::Dumper;
use utf8;

my @chars = 32 .. 126;
my $hungarian = "ÁÉÍÓÖŐÚÜŰáéíóöőúüűµ";

my @intl = split //, $hungarian;
push @chars, map {ord($_)} @intl;
push @chars, 32 while @chars < 128;

my @files = `find . -iname '*.bdf'`;
$_ =~ s/^\s+|\s+$//gs for (@files);

my @names;

my $file = "#include \"graphics/Fonts.h\"\n";

for my $bdfFile (@files)
{
  print "Processing: $bdfFile...\n";
  my $name = $bdfFile;
  $name =~ s/^.*\///g;
  $name =~ s/\.BDF$//gi;
  
  my $name = "font_$name";
  
  open FH, "<", $bdfFile or die "Can't open file for read: $!";
  
  my @bdf = <FH>;
  $_ =~ s/^\s+|\s+$//gs for (@bdf);
  
  my $state = 0;
  my %chrs;
  my $width;
  my $height;
  
  
  my $cwidth;
  my @filtered;
  my @def;
  
  for my $line (@bdf )
  {
    if( $state == 0 )
    {
      if( $line =~ /^STARTFONT/ )
      {
        $state ++;
      }
    }
    if( $state == 1 )
    {
      if( $line =~ /^FONTBOUNDINGBOX (\d+) (\d+)/ )
      {
        $width = $1;
        $height = $2;
        $state++;
      }
    }
    if( $state == 2 )
    {
      if( $line =~ /^STARTCHAR/ )
      {
        $state++;
      }
    }
    if( $state == 3 )
    {
      if( $line =~ /^ENCODING (\d+)/ )
      {
        my $unicode = $1;
        
        my $i = 0;
        @filtered = grep { $_->[0] == $unicode } map { [ $_, $i++ ] }  @chars;
        
        if( @filtered )
        {
          $cwidth = $width;
          $state ++;
        }
        else
        {
          $state = 2;
        }
      }
    }
    if( $state == 4 )
    {
      if( $line =~ /^BBX (\d+)/ )
      {
        $cwidth = $1;
      }
      if( $line =~ /^ENDCHAR$/ )
      {
        $state = 2;
      }
      if( $line =~ /^BITMAP/ )
      {
        @def = ();
        $state++;
      }
    }
    if( $state == 5 )
    {
      if( $line =~ /^([0-9a-fA-F]+)$/ )
      {
        push @def, $1;
      }
      if( $line =~ /^ENDCHAR$/ )
      {
        for my $desc ( @filtered )
        {
          my $ndx = $desc->[1];
          my @dfn = @def;
          $chrs{$ndx} = { width => $cwidth, def => \@dfn };
        }
        $state = 2;
      }
    }
  }
  
  die "Not every font defined" if values %chrs != @chars;
  
  $file .= "const uint8_t $name [] = {\n";
  $file .= "  $width, $height,\n";
  
  my $ndx = 0;
  for my $chr (@chars)
  {
    my $def = $chrs{$ndx++};
    $file .= "  " . $def->{width};
    $file .= ",   ";
    
    my $data = $def->{def};
    my @hex;
    
    for my $line (@$data)
    {
      my @arr =  ( $line =~ m/../g );
      push @hex, @arr;
    }
    $_ = "0x$_" for (@hex);
    $file .= join(",", @hex);
    $file .= ", // ";
    if(chr($chr) eq '\\')
    {
      $file .= "back slash";
    }
    else
    {
      $file .= chr($chr);
    }
    $file .= "\n";
  }
  
  $file .= "};\n\n";
  
  close FH;
  
  push @names, $name;
}

# generate the header files
my $header = "#ifndef FONTS_H\n#define FONTS_H\n\n#include <inttypes.h>\n\n" . join("", map { "extern const uint8_t $_ [];\n" } @names) . "\n#endif /* FONTS_H */\n\n";

open HDR, ">", "../include/graphics/Fonts.h" or die "Can't open file for write: $!";
print HDR $header;
close(HDR);

open FCNT, ">", "../src/graphics/Fonts.cpp" or die "Can't open file for write: $!";
print FCNT $file;
close(FCNT);
