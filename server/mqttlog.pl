#!/usr/bin/perl -w
#
# Connect to mqtt broker, collect the sensor data it receives, and
# put it into RRD files
#

use warnings;
use strict;

use RRDs;
use POSIX qw(strftime);
use Net::MQTT::Simple;

my $basedir = '/home/martin/sensors';

my $mqtt = Net::MQTT::Simple->new('localhost');
$mqtt->run(
    '#' => sub {
        my ($topic, $message) = @_;
        parse_sensor($topic, $message) if $topic =~ /^chip-/;
        print "[$topic] $message\n";
    }
);


sub parse_sensor {
    my ($topic, $message) = @_;
    # chip-001199a1/sensor-22/temp-centdegc  =>  2130
    my $recv_time = time();

    my $chipid;
    my $sensorid;
    my $typeunit;
    if ($topic =~ m!chip-(\w+)/sensor-(\w+)/(.+)!) {
        $chipid = $1;
        $sensorid = $2;
        $typeunit = $3;
    } else {
        printf STDERR "warn: can't parse topic $topic.\n";
        return;
    }

    my $chipdir = sprintf '%s/chip-%s', $basedir, $chipid;
    mkdir $chipdir if ! -d $chipdir;

    my $file = sprintf('%s/sensor-%04x-%s.rrd',
        $chipdir, $sensorid, $typeunit);

    my $err;
    if ( ! -e $file ) {
        RRDs::create(
            $file,
            '--step=300',
            '--start=now-1h',
            'DS:value:GAUGE:3600:-10000:100000000',
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
            return;
        }
    }

    RRDs::update(
        $file, '--template=value', $recv_time.':'.$message
    );
    $err = RRDs::error;
    if ($err) {
        printf STDERR "ERROR while updating $file: $err.";
    }

    return;
}

