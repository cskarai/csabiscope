#!/usr/bin/perl

use constant MAX_LEN => 7;

my %pids = (
# TOKENS
  1 => 'OUT',
  9 => 'IN',
  5 => 'SOF',
  13 => 'SETUP',
# DATA
  3 => 'DATA0',
  11 => 'DATA1',
  7 => 'DATA2',
  15 => 'MDATA',
# HANDSHAKE
  2 => 'ACK',
  10 => 'NAK',
  14 => 'STALL',
  6 => 'NYET',
# SPECIAL
  12 => 'ERR',
  8 => 'SPLIT',
  4 => 'PING',
);

use strict;
use Data::Dumper;

my $file = shift @ARGV;

my $state = "SYNC";
my @bits;
my @bytes;
my $paddingBit = 0;
my $prevState = undef;

my $offset = 0;

open FH, "<", $file or die "Can't open file for read! $!";

my $header = <FH>;

my ($stamp, $dplus, $dminus) = decode_row( scalar(<FH>) );

my $oldstamp;

do
{
  $oldstamp = $stamp;
  my ($olddplus, $olddminus) = ($dplus, $dminus);
  ($stamp, $dplus, $dminus) = decode_row( scalar(<FH>) );
  
  if ( defined $stamp )
  {
    my ($signal, $length) = decode_signal( $stamp - $oldstamp, $olddplus, $olddminus, $dplus, $dminus);
    if( defined $signal )
    {
      my $hasFrame = decode_bits( $signal, $length );

      if( $hasFrame )
      {
        decode_frame();
      }
    }
  }
}while( defined $stamp );

close FH;

exit 0;

sub decode_row
{
  my ($row) = @_;
  
  my @arr = split /,/, $row;
  s/^\s+|\s+$//g for (@arr);
  
  return @arr;
}

sub decode_signal
{
  if( ! defined $prevState )
  {
    $prevState = [@_];
    return;
  }
  
   
  my ($len, $olddplus, $olddminus, $dplus, $dminus, $plus, $lenAdd) = @$prevState;
  $prevState = [@_];

  $prevState->[0] += $lenAdd;
  
  my $oldlen = $len;
  $len /= 8.33333329999884e-08;
  $len = int($len + 0.5);

  my $signal;

  
  if( !$olddplus && $olddminus && !$dminus )
  {
    $signal = "K";
  }
  elsif( $olddplus && !$olddminus && !$dplus )
  {
    $signal = "J";
    $signal = "J+" if $plus == 1;
    $signal = "J-" if $plus == -1;
    $signal = "Empty" if( $len > MAX_LEN );
  }
  elsif( ( $len == 2 ) && !$olddplus && !$olddminus && $dplus && !$dminus )
  {
    $signal = "SE0";
  }
  elsif( ( $len > MAX_LEN ) && ( $olddplus && !$olddminus ) )
  {
    $signal = "Empty";
    $offset = 0;
  }
  else
  {
    if((( $offset == 0 ) || ( $offset == 1 )) && 
       ($olddplus && !$olddminus && $dplus && $dminus) &&
       (!$prevState->[3], $prevState->[4]) )
    {
      my @ps = ($oldlen, $olddplus, $olddminus, 0, $dminus, 1, $prevState->[0]);
      
      $prevState = \@ps;
      $offset = 1;
      return;
    }
    elsif((( $offset == 0 ) || ( $offset == -1 )) && 
       ($olddplus && $olddminus && !$dplus && $dminus) &&
       ($prevState->[3], !$prevState->[4]) )
    {
      my @ps = ($oldlen, $olddplus, 0, $dplus, $dminus, -1, -$prevState->[0]);
      print Dumper("Faxom");
      $prevState = \@ps;
      $offset = -1;
      return;
    }
    else
    {
      $signal = "?";
    }
  }
  
  my $padding = substr("      ", length($signal));
  
  print "SIGNAL: $signal,$padding$len\n";
  return ($signal, $len);
}

sub decode_bits
{
  my ($signal, $length) = @_;

  if( $signal eq "SE0" )
  {
    if( $state eq "READ_BYTE" )
    {
      if( @bits == 0 )
      {
        $state = "SYNC";
        return 1;
      }
      print "BIT:    ERROR, UNEXPECTED EOP\n";
    }
    elsif( $state eq "ERROR_WAIT_EOP" )
    {
      print "PACKET: ERROR\n";
      $state = "SYNC";
      return 0;
    }
    else
    {
      print "BIT:    ERROR\n";
      $state = "ERROR_WAIT_EOP";
      return 0;
    }
  }
  elsif( $state eq "SYNC" )
  {
    @bytes = ();
    @bits = ();
	$paddingBit = 0;
    if( $signal =~ /Empty|J/ )
    {
      $state = "SYNC_FIRST";
      return 0;
    }
    else
    {
      $state = "ERROR_WAIT_EOP";
      return 0;
    }
    die "TODO"; # TODO
  }
  elsif( $state eq "SYNC_FIRST" )
  {
    if(( $signal eq "K" ) && ($length == 1) )
    {
      print "BIT:    0\n";
      push @bits, 0;
      $state = "READ_BYTE";
      return 0;
    }
    else
    {
      $state = "ERROR_WAIT_EOP";
      return 0;
    }
  }
  elsif( $state eq "READ_BYTE" )
  {
    if( $signal =~ /J|K/ )
    {
      if( ! $paddingBit )
      {
        add_bit(0); 
      }
  
      $paddingBit = $length >= MAX_LEN;
  
      for(my $i=1; $i < $length; $i++ )
      {
        add_bit(1); 
      }
    }
    elsif( $signal eq "?" )
    {
      print "BIT:    ERROR, UNEXPECTED COMBINATION\n";
      $state = "ERROR_WAIT_EOP";
      return 0;
    }
    else
    {
    print Dumper($signal);
      die "TODO"; # TODO
    }
  }
  elsif( $state eq "ERROR_WAIT_EOP" )
  {
    if( $signal eq "Empty" )
    {
      $state = "SYNC_FIRST";
      return 0;
    }
    # do nothing
  }
  else
  {
    die "TODO"; # TODO
  }
  
  return 0;
}

sub add_bit
{
  my ($bit) = @_;
  
  print "BIT:    " . ( $bit ? "1" : "0" ) . "\n";
  push @bits, $bit;
  
  if( @bits == 8 )
  {
    my $byte = 0;
    my $msk = 128;
    while( $msk )
    {
      if( pop @bits )
      {
        $byte |= $msk;
      }
      $msk >>= 1;
    }
    
    push @bytes, $byte;
    
    my $hex = sprintf("0x%02X", $byte);
    print "BYTE:   $hex\n";
  }
}

sub compute_crc_5
{
  my ($crcarr, $bit) = @_;
  
  my $doInvert = $crcarr->[4] ^ $bit;
  
  $crcarr->[4] = $crcarr->[3];
  $crcarr->[3] = $crcarr->[2];
  $crcarr->[2] = $crcarr->[1] ^ $doInvert;
  $crcarr->[1] = $crcarr->[0];
  $crcarr->[0] = $doInvert;
}

sub compute_crc_5_int
{
  my ($crcarr, $val, $bits) = @_;
  
  for( my $i = 0; $i < $bits; $i++ )
  {
    compute_crc_5( $crcarr, ($val & (1 << $i)) ? 1 : 0 );
  }
}

sub get_crc_5
{
  my ($crcarr) = @_;
  my $res = 0;
  
  for( my $i = 0; $i < 5; $i++ )
  {
    $res |= ( 1 << (4-$i) ) if ! $crcarr->[$i];
  }

  return $res;
}

sub compute_crc_16
{
  my ($crcarr, $bit) = @_;
  
  my $doInvert = $crcarr->[15] ^ $bit;
  
  unshift @{$crcarr}, $doInvert;
  pop @{$crcarr};
  $crcarr->[2] = $crcarr->[2] ^ $doInvert;
  $crcarr->[15] = $crcarr->[15] ^ $doInvert;
}

sub compute_crc_16_int
{
  my ($crcarr, $val, $bits) = @_;
  
  for( my $i = 0; $i < $bits; $i++ )
  {
    compute_crc_16( $crcarr, ($val & (1 << $i)) ? 1 : 0 );
  }
}

sub get_crc_16
{
  my ($crcarr) = @_;
  my $res = 0;
  
  for( my $i = 0; $i < 16; $i++ )
  {
    $res |= ( 1 << (15-$i) ) if ! $crcarr->[$i];
  }

  return $res;
}

sub decode_frame
{
  my $frame = 'PACKET:';
  
  if( $bytes[0] == 128 )
  {
    $frame .= " SYNC";
  }
  else
  {
    die "TODO: invalid packet start";
  }
  
  my $pid = $bytes[1];
  my $pid1 = $pid & 15;
  my $pid2 = ((~$pid) >> 4 ) & 15;
  
  if( $pid1 != $pid2 )
  {
    die "TODO: invalid PID value";
  }
  $pid = $pid1;
  
  if( ! exists $pids{$pid} )
  {
    die "TODO: unknown PID";
  }
  
  $frame .= " PID(" . $pids{$pid} . ")";
  
  if( $pid == 5 ) # SOF
  {
    my $frameNum = $bytes[2] + (($bytes[3] & 7) << 8);

    $frame .= " FRAME(" . $frameNum . ")";

    my $crc5 = $bytes[3] >> 3;
    $frame .= " CRC(" . $crc5 . ")";

    my @crcarr = ( 1, 1, 1, 1, 1 );
    compute_crc_5_int( \@crcarr, $frameNum, 11);

    my $num = get_crc_5( \@crcarr );

    if( $num == $crc5 )
    {
      $frame .= " OK";
    }
    else
    {
      die "TODO: crc error";
    }
  }
  elsif( ($pid == 9) || ($pid == 1) || ($pid == 13) ) # IN, OUT, SETUP
  {
    my $addr = $bytes[2] & 0x7F;

    $frame .= " ADDR(" . $addr . ")";

    my $endp = (($bytes[2] &0x80) >> 7 ) + (($bytes[3] & 7) << 1);
    $frame .= " ENDP(" . $endp . ")";

    my $crc5 = $bytes[3] >> 3;
    $frame .= " CRC(" . $crc5 . ")";

    my @crcarr = ( 1, 1, 1, 1, 1 );
    compute_crc_5_int( \@crcarr, $addr, 7);
    compute_crc_5_int( \@crcarr, $endp, 4);

    my $num = get_crc_5( \@crcarr );

    if( $num == $crc5 )
    {
      $frame .= " OK";
    }
    else
    {
      die "TODO: crc error";
    }
  }
  elsif(( $pid == 10 ) || ( $pid == 2 ) || ( $pid == 14 )) # NAK, ACK, STALL
  {
    # done
  }
  elsif(( $pid == 3 ) || ( $pid == 11 )) # DATA0, DATA1
  {
    my $crc16 = $bytes[-1] << 8;
    $crc16 |= $bytes[-2];

    $frame .= " CRC(" . $crc16 . ")";

    my @crcarr = ( (1) x 16 );

    for(my $i=2; $i < @bytes-2; $i++)
    {
      compute_crc_16_int( \@crcarr, $bytes[$i], 8 );
    }

    my $result = get_crc_16( \@crcarr );

    if( $result == $crc16 )
    {
      $frame .= " OK";
    }
    else
    {
      die "TODO: crc error";
    }
 
    for(my $i=2; $i < @bytes-2; $i++)
    {
      if(( $i % 32 ) == 2 )
      {
        $frame .= "\nPACKET => ";
      }
      else
      {
        $frame .= " ";
      }
      my $hex = sprintf("%02X ", $bytes[$i]);
      $frame .= $hex;
    }
  }
  else
  {
    print Dumper(\@bytes);
    die "TODO: implement this: " . $pids{$pid};
  }
  
  print "$frame\n";
}