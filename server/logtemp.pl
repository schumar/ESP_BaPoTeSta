#!/usr/bin/perl -w
#
use warnings;
use strict;

use RRDs;
use IO::Socket::INET;
use POSIX qw(strftime);
use JSON;

my $basedir = 'sensors';
my $graphscript = './tempgraph.sh';

my @sensortype = qw( temp battery humidity );
my @unittype = qw( centdegc percent raw volt );

my $socket = new IO::Socket::INET(
    LocalPort => '9989',
    Proto => 'udp',
) or die "ERROR in Socket Creation : $!.";

my $lastupdate = 0;
my $lastgraph = 0;

while(1)
{
    # read operation on the socket
    $socket->recv(my $rawdata, 2048);
    my $recv_time = time();
    # $peer_address = $socket->peerhost();

    printf STDERR "%s: %s", strftime("%F %T", gmtime($recv_time)), $rawdata;

    # if we receive another package within 2 seconds, ignore it
    # (most likely caused by the client sending every packet 2 times)
    if ($recv_time <= $lastupdate + 2) {
        printf STDERR "    skipped.\n";
        next;
    }

    my $data = decode_json $rawdata;

    # Received JSON will look like this:
    # {
    #   "chipId": 3735931646,
    #   "timestep": 123,
    #   "measurements": [
    #     {
    #       "sensorId": 42,
    #       "sensorType": 0,
    #       "value": 1875,
    #       "unitType": 0
    #     },
    #   ]
    # }

    my $chipdir = sprintf '%s/chip-%08x', $basedir, $$data{'chipId'};
    mkdir $chipdir if ! -d $chipdir;

    foreach my $measurement (@{$$data{'measurements'}}) {
        my $file = sprintf('%s/sensor-%04x-%s-%s.rrd',
            $chipdir, $$measurement{'sensorId'},
            $sensortype[$$measurement{'sensorType'}],
            $unittype[$$measurement{'unitType'}]);

        my $err;
        if ( ! -e $file ) {
            RRDs::create(
                $file,
                '--step=300',
                '--start=now-1h',
                'DS:value:GAUGE:3600:-10000:10000',
                'RRA:AVERAGE:0.5:1:4100',  # 2 weeks 5min
                'RRA:AVERAGE:0.5:6:3000',  # 2 month 30min
                'RRA:AVERAGE:0.5:36:4300', # 2 years 3h
                'RRA:MIN:0.5:1:4100',
                'RRA:MIN:0.5:6:3000',
                'RRA:MIN:0.5:36:4300',
                'RRA:MAX:0.5:1:4100',
                'RRA:MAX:0.5:6:3000',
                'RRA:MAX:0.5:36:4300',
            );
            $err = RRDs::error;
            if ($err) {
                printf STDERR "ERROR while creating $file: $err.";
                next;
            }
        }

        RRDs::update(
            $file, '--template=value', $recv_time.':'.$$measurement{'value'}
        );
        $err = RRDs::error;
        if ($err) {
            printf STDERR "ERROR while updating $file: $err.";
            next;
        }
    }

    $lastupdate = $recv_time;

    system($graphscript);
}

