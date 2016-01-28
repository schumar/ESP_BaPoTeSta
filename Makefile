# Simple Makefile, using the Arduino command-line client
# based on http://www.esp8266.com/viewtopic.php?f=33&t=3358
#
# Can't use the "standard" Arduino-Makefile, as it doesn't support esp8266, see
# https://github.com/sudar/Arduino-Makefile/issues/343

ARDUINO_DIR ?= /opt/arduino
ARDUINO_BIN ?= $(ARDUINO_DIR)/arduino
SOURCE = ESP_BaPoTeSta
BUILD_DIR ?= /tmp/build
OPTS = FlashSize=4M1M
SERIAL_PORT ?= /dev/ttyUSB0
SERIAL_BAUD ?= 115200

verify: $(SOURCE).ino clean
	$(ARDUINO_BIN) -v --verify --board esp8266:esp8266:generic:$(OPTS) \
		--pref build.path=$(BUILD_DIR) \
		--pref target_package=esp8266 \
		--pref target_platform=esp8266 \
		--pref board=generic \
		$(SOURCE).ino

upload: $(SOURCE).ino clean
	$(ARDUINO_BIN) -v --upload --board esp8266:esp8266:generic:$(OPTS) \
		--port $(SERIAL_PORT) \
		--pref build.path=$(BUILD_DIR) \
		--pref target_package=esp8266 \
		--pref target_platform=esp8266 \
		--pref board=generic \
		$(SOURCE).ino

clean:
	rm -rf build
