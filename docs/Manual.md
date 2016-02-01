# Manual

## Read This First!

If you have acquired a finished BaPoTesTa, you can skip ahead to
(Configuration)[#configuration] now!

The following instructions are for the board as I designed it -- if you are
using another PCB, or have modified the schematics, they might still be a good
rough guideline.

If you want to attach other sensors, actuators, MOSFETs or whatever: There is
one unused pin (GPIO 4) which even has a through-hole you can solder a wire
into, and on the bottom of the board there are 2 solder bridges for GPIO 12
and GPIO 13 which you can cut to use those pins (originally for the DS18B20 and
DHT22, respecticely) too.

## Parts List

### Basic parts

* 1 PCB
* 10 SMD (1206) 10k&Omega; resistors
* 1 SMD (1206) 56k&Omega; resistor
* 2 SMD (1206) 1&mu;F capacitors
* 1 MCP1700 voltage regulator
* 1 100&mu;F buffer capacitor
* 1 ESP-12/12E/12F module
* 9 pin headers

### Power

You also need either either

* 1 case for 4 AA batteries
* 1 3mm screw+nut

or, if you want to plug in some other power source:

* 2 additional pin headers

### Sensors

One or both of
* 1 DS18B20 temperature sensor
* 1 DHT22 temperature&amp;humidity sensor

## Soldering

If you haven't done any soldering before (or your knowledge is a little rusty),
you might want to check the Internet for tutorials :) e.g.
* https://learn.sparkfun.com/tutorials/how-to-solder---through-hole-soldering
* https://learn.sparkfun.com/tutorials/how-to-solder---castellated-mounting-holes

but if you have some time, and want to have a good understanding of soldering,
check out
[PACE: Basic Soldering Lesson 1](https://www.youtube.com/watch?v=vIT4ra6Mo0s)  
(it's American, so prepare to snicker whenever the presenter pronounces
soldering as "soddering" :)

### Label your Board

Take a non water-solvable pen and write an ID on the white boxes on the front
and back of the board. Feel free to choose whatever scheme you want (most
people will go with "1, 2, 3,...", but why not use Klingon letters instead? ;)

### SMD Parts

For an aesthetically pleasing result, I suggest to stick with the convention I
already used for the board itself: "down" for text is South or East, i.e. the
labels of the SMD parts should be left-to-right or bottom-to-top, not
right-to-left or top-to-bottom.

Start with R1 (the only 56k&Omega; resistor, labelled "5602").  
Then do the 10k&Omega; resistors.

If you have a multi-meter, measure the resistance of R2, and the resistance
between the two left pins of R1 and R2 (R1+R2, as their right pins are
connected). Divide the first measurement (around 10k) by the second (around
66k), and note down the number (4 significant digits, i.e. "0.1534", not "0.15"
or "0.1533782")  
This will be the "battery divider" you can configure in the web-interface.

The only SMD parts left are the 2 1&mu;F capacitors (C1 and C3) -- add those.

Now is a good time to take some isopropanol, maybe mixed 1:1 with acetone,
and, using a brush, clean flux residue from the board (the ugly brown stuff).

### MCP 1700

That should be a no-brainer. Just make sure to put it in correctly (flat side
of the case points left/outward).

After soldering, cut off the 3 wires, and try to keep those short pieces, we'll
need them in the next step!

### ESP-12

I've found this to be a good way to solder the ESP-12:

* Tin the RST and GND pin of the PCB
* Put the module on the PCB
* Put 2 of the wires from the previous step through the little holes of the
  module, next to the VCC and TX pins (i.e. the corner pins which you *didn't*
  tin)
* Now heat the RST pin a little while pressing (gently!) down on the module.  
  This should fix that corner
* Do the same for GND
* Now the module is perfectly aligned and can't move anymore -- remove the 2
  wires.
* Solder all pins (do RST/GND last!)
* If you aren't an experiences solderer/ess(?), shortly heat each pin again
  (until the solder melts again), to ensure that all connections are flawless

But I'm just an amateur, so feel free to do it better! (and fork this project,
and improve this documentation ;)

This is again a good time to clean your board.

### Buffer Capacitor

This one should be easy again, just make sure that you don't swap the pins --
this is an electrolytic cap, so it has a plus- and a minus-pin, and swapping
those two can lead to Rapid Unplanned Disassembly.

### Pin Headers

Speaking from experience:
* Make sure that you count correctly! You need 3+2+2+2, and possibly 2 again
  for the power connection
* Those little bastards are really hard to separate! Using a good knife is the
  best way, but make sure not to cut yourself, and be aware that both parts will
  jump away the moment you separate them
* After soldering, those things are *hot* (d'uh). Touching them is bad
* Soldering might take a little more patience than usual, because the pins have
  a high thermal mass.

### Sensors

I've decided to use a "wide" footprint for the DS18B20, to make it easier to
solder in a cable instead of the sensor. But this means that if you want to put
the DS18B20 right on the board, you need to bend its pins a little bit. Tweezers
are quite useful for that.

While it's usually a good thing to keep pins as short as possible, I recommend
to keep the pins of the DS18B20 as *long* as possible, so the temperature
reading will be rather influenced by the air temperature than by the temperature
of the PCB.

When inserting the DHT22, make absolutely sure that the grill points outwards!

### Mounting Screw

Now is a good time to prepare a hole where the mounting screw will go through,
e.g. if you plan to put the board directly on a battery case, drill a hole
in that.

The antenna of the ESP-12 module (at the right border of the board) shouldn't
be covered by metal, if possible!

### Power

If you want to plug the power supply onto the board, solder in 2 pin headers.

Otherwise, solder the cables directly in the two pins. There are 2 small
mounting holes just left of the pins, which you can put the cables through (to
protect the solder joints from tension).

### Finish

Make one last visual inspection of all soldering joints.

If you want, you can switch on your board now -- it should briefly flash the
blue LED on the module. If the LED doesn't flash, switch off the board, take
your multimeter and test all connections :(  
If it does flash, switch the board off again; no use in wasting energy.

Tin, clean, re-tin you soldering iron, switch it off, clean the board, put back
any unused parts.

Congratulations :)  
You are now ready to flash the firmware!

## Flashing

Right now, your ESP-12 has the stock firmware, which is useless for what we want
to do.

### Firmware

If you want to compile the firmware yourself, see the [Hacking doc](Hacking.md)
on how to do this.

But there are only a few reason to do so:

* Your ESP8266-module has a flash size other than 4MiB
* You have modified the board (or using a completely different one), and can't
  use GPIO 5 for entering the Maintenance mode
* While you can change the WiFi-password for the Maintenance mode directly in
  the Maintenance web-interface, the board will still start with the default
  password the first time. If you are worried about that, you are even more
  paranoid than me :)
* You don't trust binaries you haven't compiled yourself -- good for you!

Otherwise, just take the [precompiled firmware](../precompiled/).

### Connect a Serial-to-USB adapter to your board

You need a Serial-to-USB adapter with support for 3.3V levels instead
of 5V, e.g. [FTL232](XXX)

* Switch off your board.
* Ensure that your adapter is set to 3.3V!
* Connect the "GND" pin of the adapter to the "GND" pin of the ESP.
* Connect the "TX" pin of the adapter to the "RX" pin of the ESP, and vice versa.
  Do *not* connect VCC, unless you know what you are doing!

### Connect the adapter to your PC

I guess you can figure out how to do this :)

Check dmesg for the device name of the adapter (e.g. /dev/ttyUSB0)

### Transfer firmware

You will also need the [esptool](XXX), and of course 
* Put a jumper bridge on the 2 "Upload" pins.
* Type the command
  esptool [XXX]
  but don't press enter yet
* Switch on the board
* Press Enter

If it doesn't work the first time, make sure that you have specified the correct
device name, and that you haven't swapped TX and RX. Try again.



