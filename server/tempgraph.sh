#!/bin/bash

out='tempgraphs/temp'
rrddir='sensors/chip-12345678/';
wunder='wunder.rrd'


rrdtool graph ${out}-daily.png \
    --end now --start end-24h \
    --title 'Temperatur Hasengehege (Tag)' --vertical-label Celsius \
    --width 800 --height 500 --upper-limit 20 --lower-limit -4 \
    --slope-mode --imgformat PNG \
    DEF:cent="$sensor1":value:AVERAGE:start=end-49h CDEF:temp=cent,100,/ \
    DEF:exttemp="$wunder":temp:AVERAGE:start=end-49h \
    DEF:sunny="$wunder":sunup:MAX:start=end-49h \
    VDEF:avg=temp,AVERAGE VDEF:min=temp,MINIMUM VDEF:max=temp,MAXIMUM VDEF:lst=temp,LAST \
    CDEF:smooth=temp,3600,TRENDNAN \
    CDEF:vortag=84600,1,3600,temp,PREDICT \
    CDEF:sunnybar=sunny,100,* \
    AREA:sunnybar#ffff8060::skipscale \
    AREA:temp#aaaaaa80 \
    COMMENT:"     " LINE2:temp#008800:"Temperatur    " \
    COMMENT:"Min\:" GPRINT:min:"%2.1lf  " \
    COMMENT:"Avg\:" GPRINT:avg:"%2.1lf  " \
    COMMENT:"Max\:" GPRINT:max:"%2.1lf  " \
    COMMENT:"Now\:" GPRINT:lst:"%2.1lf\l" \
    COMMENT:"     " LINE1:vortag#0000ff88:"Vortag\l" \
    SHIFT:smooth:-1800 \
    COMMENT:"     " LINE2:smooth#ff000088:"Stundenschnitt\l" \
    COMMENT:"     " LINE1:exttemp#000000:"Temperatur (extern)\l"

rrdtool graph ${out}-weekly.png \
    --end now --start end-7d \
    --title 'Temperatur Hasengehege (Woche)' --vertical-label Celsius \
    --width 800 --height 300 --upper-limit 20 --lower-limit -4 \
    --slope-mode --imgformat PNG \
    DEF:cent="$sensor1":value:AVERAGE:start=end-15d CDEF:temp=cent,100,/ \
    DEF:exttemp="$wunder":temp:AVERAGE:start=end-8d \
    DEF:sunny="$wunder":sunup:MAX:start=end-8d \
    VDEF:avg=temp,AVERAGE VDEF:min=temp,MINIMUM VDEF:max=temp,MAXIMUM VDEF:lst=temp,LAST \
    CDEF:smooth=temp,86400,TRENDNAN \
    CDEF:vortag=603000,1,3600,temp,PREDICT \
    CDEF:sunnybar=sunny,100,* \
    AREA:sunnybar#ffff8060::skipscale \
    AREA:temp\#00000020 \
    COMMENT:"     " LINE2:temp#008800:"Temperatur    " \
    COMMENT:"Min\:" GPRINT:min:"%2.1lf  " \
    COMMENT:"Avg\:" GPRINT:avg:"%2.1lf  " \
    COMMENT:"Max\:" GPRINT:max:"%2.1lf  " \
    COMMENT:"Now\:" GPRINT:lst:"%2.1lf\l" \
    COMMENT:"     " LINE1:vortag#0000ff88:"Vorwoche\l" \
    SHIFT:smooth:-43200 \
    COMMENT:"     " LINE2:smooth#ff000088:"Tagesschnitt\l" \
    COMMENT:"     " LINE1:exttemp#000000:"Temperatur (extern)\l"

