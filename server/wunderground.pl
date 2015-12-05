#!/usr/bin/perl -w
#

use warnings;
use strict;

use RRDs;
use IO::Socket::INET;
use POSIX qw(strftime);
use JSON;
use LWP::Simple;
use WWW::Wunderground::API;
use Data::Dumper;
use Cache::FileCache;

my $file = 'wunder.rrd';
my $loc = '12.3456,12.3456';                     # [XXX] CHANGE THIS
my $key = 'xxxx';                                # [XXX] AND THIS
die 'You need to configure this script first.';  # [XXX] and then this :)

if ( ! -e $file ) {
    RRDs::create(
        $file,
        '--step=1800',
        '--start=now-1h',
        'DS:temp:GAUGE:3600:-25:50',
        'DS:precip:GAUGE:3600:0:100',
        'DS:humid:GAUGE:3600:1:100',
        'DS:wind:GAUGE:3600:0:200',
        'DS:sunup:GAUGE:3600:0:1',
        'RRA:AVERAGE:0.5:1:3100',
        'RRA:AVERAGE:0.5:8:1100',
        'RRA:AVERAGE:0.5:48:1100',
        'RRA:AVERAGE:0.5:336:530',
        'RRA:MIN:0.8:1:3100',
        'RRA:MIN:0.8:8:1100',
        'RRA:MIN:0.8:48:1100',
        'RRA:MIN:0.8:336:530',
        'RRA:MAX:0.8:1:3100',
        'RRA:MAX:0.8:8:1100',
        'RRA:MAX:0.8:48:1100',
        'RRA:MAX:0.8:336:530',
    );
}
my $ERR=RRDs::error;
die "ERROR while creating $file: $ERR." if $ERR;

my $wun = new WWW::Wunderground::API(
    location => $loc,
    api_key => $key,
);

my $conditions = $wun->api_call('conditions');
my $astronomy  = $wun->api_call('astronomy');

my $ts = $$conditions{'observation_epoch'};

my (undef, $min, $hour, undef) = localtime($ts);
my $curmin = $hour*60 + $min;
my $sunrise = $$astronomy{'sunrise'}{'hour'}*60 + $$astronomy{'sunrise'}{'minute'};
my $sunset = $$astronomy{'sunset'}{'hour'}*60 + $$astronomy{'sunset'}{'minute'};

my $sunny = 1;
$sunny = 0 if ($curmin < $sunrise or $curmin > $sunset);


# some simple fixes
$$conditions{'relative_humidity'} =~ s/%$//;
$$conditions{'precip_1hr_metric'} =~ s/^\s+//;
$$conditions{'precip_1hr_metric'} = 'U' if $$conditions{'precip_1hr_metric'} eq '--';

my $update = "$ts:" . join(':', map {$$conditions{$_}} qw( temp_c precip_1hr_metric relative_humidity wind_kph )) . ":$sunny";
print "$update\n";

RRDs::update(
    $file,
    '--template=temp:precip:humid:wind:sunup',
    $update,
);
$ERR=RRDs::error;
die "ERROR while creating $file: $ERR." if $ERR;

