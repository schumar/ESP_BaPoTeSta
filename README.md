#  ESP8266 Battery Powered Temperature Station

## Intro

Using only an ESP8266 (no Atmel AVR) and a sensor, it's possible to build a very cheap and power-efficient WiFi-enabled weather station.

### Pictures of v0.3

![BaPoTeSta Board v0.3 Rendering](https://github.com/schumar/ESP_BaPoTeSta/raw/master/docs/ESP_BaPoTeSta.jpg)
![BaPoTeSta Board v0.3 real](https://github.com/schumar/ESP_BaPoTeSta/raw/master/docs/board_v0.3_real.jpg)

## Current state

I've already built 3 sensors of v0.3, have now improved the board massively, and am in the process
of ordering the parts for the second run (ca. 30 pieces).

The sensors are reporting their data via MQTT, so the data can be received with OpenHAB,
or a small Perl script.

"Release early, release often" -- so don't expect to be able to clone this repo and have a working IoThingy 10 minutes later :)

## Screenshot

![RRD Graph](https://github.com/schumar/ESP_BaPoTeSta/raw/master/docs/temp-daily.png)

## Technical details

-> Read the source, stare at my KiCAD schematic :)

## BOM

Quick calculation: 10 boards would cost 6 USD each (excl. batteries)

Part|need/board|URL|pcs|total Price|price/board|
----|---------:|---|--:|----------:|----------:|
PCB|1|http://dev.dangerousprototypes.com/store/pcbs|10|13.00|1.30
PinHdr (angular)|2|http://www.aliexpress.com/item/x/32241048789.html|80|0.50|0.01
DS18B20|1|http://www.aliexpress.com/item/x/32530262851.html|10|5.40|0.54
R 10k|5|http://www.aliexpress.com/item/x/32479305609.html|100|0.82|0.04
R 68k|1|http://www.aliexpress.com/item/x/32505283538.html|100|0.82|0.01
R 4k7|1|http://www.aliexpress.com/item/x/32270050733.html|100|0.96|0.01
MCP1700-330|1|http://www.aliexpress.com/item/x/32523953207.html|10|3.61|0.36
10uF 10V|2|http://www.aliexpress.com/item/x/32260294384.html|50|3.59|0.14
ESP-12E|1|http://www.aliexpress.com/item/x/32519328481.html|10|21.19|2.12
Button|2|http://www.aliexpress.com/item/x/1582090411.html|100|1.38|0.03
PinHdr (straight)|3|http://www.aliexpress.com/item/x/32537378399.html|40|0.32|0.02
Battery Case|1|http://www.aliexpress.com/item/x/2022527017.html|10|11.68|1.17
Battery Case|1|http://www.aliexpress.com/item/x/32522870331.html|10|8.30|0.83
**TOTAL**||||**59.89**|**5.42**

## To Do

see https://github.com/schumar/ESP_BaPoTeSta/issues

## Thanks

* Many thanks to all the people working on the [Arduino core for ESP8266](https://github.com/esp8266/Arduino)
* Cheers to [jdunmire](https://github.com/jdunmire/kicad-ESP8266) for his ESP8266 KiCAD library!
