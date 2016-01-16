# Hacking

## Install Arduino

Get Arduino from https://www.arduino.cc/en/Main/Software

## Add ESP8266 to Arduino

Start the Arduino IDE, go to File/Preferences, and add the following line to "Additional Boards Manager URLs:"
 http://arduino.esp8266.com/staging/package_esp8266com_index.json

Go to Tools/Board: "something"/Boards Manager...
Search for "esp82", click on the "esp8266 by ESP Community", click Install

(for details, see https://github.com/esp8266/Arduino)

## Clone this repository

cd ~/Arduino
git clone https://github.com/schumar/ESP_BaPoTeSta.git

## Start coding

Open the ESP_BaPoTeSta project in Arduino, and hack away

## Upload

### Connect a Serial-to-USB adapter to your board

Switch off your board.
Ensure that your adapter is set to 3.3V!
Connect the "GND" pin of the adapter to the "GND" pin of the ESP.
Connect the "TX" pin of the adapter to the "RX" pin of the ESP, and vice versa.
Do *not* connect VCC, unless you know what you are doing!

### Connect to your PC

I guess you can figure out how to do this :)

### Transfer firmware

Put a jumper bridge on the 2 "Upload" pins.
Press Ctrl+U in Arduino (or click the "Play" button)
While the sketch is compiling, switch on the board.

If it doesn't work the first time, make sure that the correct serial port is
selected in Tools/Port, and try again.
