#!/usr/bin/perl -w
#
use warnings;
use strict;

use RRD::Simple;
use IO::Socket::INET;
use POSIX qw(strftime);

my $file = 'temp.rrd';

my $rrd = RRD::Simple->new(
    file => $file,
    cf => [ qw(MIN AVERAGE MAX) ],
);

if ( ! -e $file ) {
    $rrd->create(
        $file,
        'mrtg',
        temp => "GAUGE",
    );
}


my $socket = new IO::Socket::INET(
    LocalPort => '9988',
    Proto => 'udp',
) or die "ERROR in Socket Creation : $!.";

my $lastupdate = 0;
my $lastgraph = 0;

while(1)
{
    # read operation on the socket
    $socket->recv(my $data, 1024);
    my $recv_time = time();
    # $peer_address = $socket->peerhost();

    printf STDERR "%s %s", strftime("%F %T", gmtime($recv_time)), $data;

    if ($recv_time <= $lastupdate + 2) {
        printf STDERR "    skipped.\n";
        next;
    }

    if ($data =~ /([+-]?[\d.]+)/) {
        $rrd->update($file, $recv_time, temp => 100 * $1);
        $lastupdate = $recv_time;
    } else {
        printf STDERR "Parse error!";
        next;
    }

    # every 10 mins, regenerate graph
    if ($lastgraph <= $recv_time + 600) {
        system('./tempgraph.sh');
        $lastgraph = $recv_time;
        printf STDERR "    updated graph\n";
    }

}

