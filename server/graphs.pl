#!/usr/bin/perl -w
#
# Make pretty graphs out of the data in the RRD files created
# by mqttlog.pl
#

use warnings;
use strict;

use RRDs;
use POSIX qw(strftime);
use Data::Dumper;

my $basedir = '/home/martin/sensors';
my $outdir = '/srv/http/sensors/graphs';

my %chipname = (
    'chip-009d557a' => 'Sensor 0 Hasengehege',
    'chip-009baf92' => 'Sensor 1 Wohnzimmer',
    'chip-001199a1' => 'Sensor 2 Schlafzimmer',
    'chip-00119998' => 'Sensor 3 Hasengehege',
);

my %sensorname = (
    '0000' => 'intern',
    '002a' => 'NTC',
    '0f00' => 'Dallas',
    'b8ff' => 'Dallas',
    '0016' => 'DHT22',
);


my %typename = (
    'temp' => 'Temperatur',
    'humidity' => 'Feuchtigkeit',
    'tempHI' => 'Hitzeindex',
    'battery' => 'Batteriespannung',
    'time' => 'Messdauer',
);

my %unit = (
    'temp' => {name => 'Celsius', div => 100},
    'humidity' => {name => 'Prozent', div => 100},
    'battery' => {name => 'Volt', div => 1000},
    'time' => {name => 'Sekunden', div => 1e6},
);

my %range = (
    'daily' => {
        'duration' => '24h',
        'retrieve' => '49h',
        'previous' => 86400,
        'smooth'   => 3600,
        'name'     => 'Tag',
    },
    'weekly' => {
        'duration' => '7d',
        'retrieve' => '15d',
        'previous' => 86400*7,
        'smooth'   => 3600*6,
        'name'     => 'Woche',
    },
    'monthly' => {
        'duration' => '30d',
        'retrieve' => '65d',
        'previous' => 86400*30,
        'smooth'   => 86400,
        'name'     => 'Monat',
    },
);

my %consfunc = (
    'min' => 'MINIMUM',
    'avg' => 'AVERAGE',
    'max' => 'MAXIMUM',
    'lst' => 'LAST',
);

# colors taken from http://colorbrewer2.org/
my @colors = qw( 1b9e77 d95f02 7570b3 e7298a 666666 e6ab02 66a61e a6761d );


# Collect a list of RRD files, e.g.
# chip-009d557a/sensor-002a-temp-centdegc.rrd
# chip-001199a1/sensor-0016-humidity-centpercent.rrd

opendir (my $dh, $basedir) || die "can't opendir $basedir: $!.";

my %rrdfiles;
my %prettyname;

while (readdir $dh) {
    if (/(chip-.*)/) {
        my $chipid = $1;
        my $chipdir = "$basedir/$1";
        opendir (my $dhc, $chipdir) || die "can't opendir $chipdir: $!.";
        while (readdir $dhc) {
            next if /^\./;
            if (/(^sensor-(\w+)-(\w+)-(\w+).rrd)$/) {
                my $rrdfile = "$chipdir/$1";
                my $sensorid = $2;
                my $type = $3;
                my $unit = $4;

                next if $unit eq 'raw';

                # printf "Found $rrdfile\n";

                $prettyname{$rrdfile} = sprintf '%-22s %-7s %s',
                    $chipname{$chipid},
                    (exists $sensorname{$sensorid} ? $sensorname{$sensorid} : ''),
                    $typename{$type};

                $type = 'temp' if $type eq 'tempHI';
                push @{$rrdfiles{$type}}, $rrdfile;

            } else {
                printf "WARN: Found extraenous file $chipdir/$1\n";
            }
        }
    }
}

#print Dumper(\%rrdfiles);
#print Dumper(\%prettyname);

foreach my $type (keys %rrdfiles) {

    # prepare arrays
    my $idx = 0;
    my @file;
    my @name;
    my @def;
    foreach my $file (@{$rrdfiles{$type}}) {
        $file[$idx] = $file;
        $name[$idx] = $prettyname{$file};
        $def[$idx] = "${type}${idx}";
        $idx++;
    }

    foreach my $range (keys %range) {

        my @opts = (
            "$outdir/$type-$range.png",
            '--end', 'now', '--start', 'end-'.$range{$range}{'duration'},
            '--title', $typename{$type}.' ('.$range{$range}{'name'}.')',
            '--vertical-label', $unit{$type}{'name'},
            '--width', '700', '--height', '350',
            '--slope-mode', '--imgformat', 'PNG',
        );

        # header for legend
        push @opts,
            #         Sensor 0 Schlafzimmer Dallas Batteriespannung
            sprintf 'COMMENT:  %45s  Minimum Schnitt Maximum   Jetzt\l', '';

        # build value definitions (DEF, VDEF, CDEF)
        for my $i (0..$#file) {
            # raw value from file
            push @opts, sprintf 'DEF:raw%s=%s:value:AVERAGE:start=end-%s',
                $def[$i], $file[$i], $range{$range}{'retrieve'};
            # scale (e.g. centidegree -> degree)
            push @opts, sprintf 'CDEF:%s=raw%s,%d,/',
                $def[$i], $def[$i], $unit{$type}{'div'};
            # get min/avg/max/last for legend
            foreach my $cons ('min', 'avg', 'max', 'lst') {
                push @opts, sprintf 'VDEF:%s%s=%s,%s',
                    $cons, $def[$i], $def[$i], $consfunc{$cons};
            }
            # smooth curves
            push @opts, sprintf 'CDEF:smo%s=%s,%d,TRENDNAN',
                $def[$i], $def[$i], $range{$range}{'smooth'};
            push @opts, sprintf 'SHIFT:smo%s:-%d',
                $def[$i], $range{$range}{'smooth'} / 2;
            # previous day/week/whatever
            push @opts, sprintf 'CDEF:prv%s=%d,1,%d,%s,PREDICT',
                $def[$i], $range{$range}{'previous'} - $range{$range}{'smooth'}/2,
                $range{$range}{'smooth'}, $def[$i];
        }

        # Now draw the lines

        # line at 0 for temp-graphs
        push @opts, sprintf 'HRULE:0#00000080' if $type eq 'temp';
        # line at 3.75V for battery graphs
        push @opts, sprintf 'LINE1:3.75#C00000' if $type eq 'battery';

        for my $i (0..$#file) {
            # the previous period
            push @opts, sprintf 'LINE1:prv%s#%sc0', $def[$i], $colors[$i];
            # current line
            push @opts, sprintf 'LINE2:%s#%s80', $def[$i], $colors[$i];
            # the smoothed line
            push @opts, sprintf 'LINE2:smo%s#%s:%-45s', $def[$i], $colors[$i], $name[$i];

            # and add the values to the legend
            foreach my $cons ('min', 'avg', 'max', 'lst') {
                push @opts, sprintf 'GPRINT:%s%s:%%6.2lf', $cons, $def[$i];
            }
            push @opts, 'COMMENT:\l';

        }

        RRDs::graph(@opts);
        my $err = RRDs::error;
        if ($err) {
            printf STDERR "ERROR while creating $type-$range.png: $err.";
            next;
        }
        #print Dumper(\@opts);
        #print "\n";
    }

}

