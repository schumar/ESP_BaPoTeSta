/*
   Battery-Powered WiFi Temperature Station
   Send temperature data via UDP

   Martin Schuster 2015

ToDo:
    use static IP
    check for WiFi status instead of blindly waiting
    figure out best sleep mode

 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// configuration
const char ssid[] = "tabr.org";
IPAddress myAddr(10, 1, 0, 35);
IPAddress tempServer(10, 1, 0, 9);
const unsigned int tempPort = 9988;
const byte MEASURES = 5;
const byte SLEEPSEC = 20;
const byte PIN_BLUELED = 1;
const byte PIN_PTC = 5;

WiFiUDP Udp;
unsigned int sensorValue[MEASURES];
unsigned long int chipId;


void setup() {
    // activate (active low) blue LED to show that we are "on"
    pinMode(PIN_BLUELED, OUTPUT);
    digitalWrite(PIN_BLUELED, LOW);

    // start WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid);

    // wait a while for WiFi to associate and get IP
    delay(2000);

    // get ChipID, will be used as unique ID when sending data
    chipId = ESP.getChipId();
}


// this won't actually "loop", as the last command leads to a reset
void loop() {

    // activate power to PTC
    pinMode(PIN_PTC, OUTPUT);
    digitalWrite(PIN_PTC, HIGH);

    // measure temp multiple times
    for (byte c=0; c<MEASURES; c++) {
        delay(100);
        sensorValue[c] = analogRead(A0);
    }

    // switch off PTC
    digitalWrite(PIN_PTC, LOW);

    delay(1000);
    sendTemp(sensorValue);

    // switch off (active low) blue LED to show that we are "off"
    digitalWrite(PIN_BLUELED, HIGH);

    // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED
    ESP.deepSleep(1e6 * SLEEPSEC, WAKE_RF_DEFAULT);
}


void sendTemp(unsigned int* temp) {
    const byte PACKET_SIZE = 11 + 5*MEASURES + 1;
    static char packetBuffer[PACKET_SIZE];

    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, PACKET_SIZE);

    // start data with ChipID (so multiple stations can use the same server)
    sprintf(packetBuffer, "0x%08x ", chipId);

    // add measured temperature values
    for (byte c=0; c<MEASURES; c++) {
        sprintf(11+packetBuffer+5*c, "%04d ", temp[c]);
    }
    // the buffer now ends with SPC NUL -- change SPC to Newline
    packetBuffer[PACKET_SIZE - 2] = '\n';

    Udp.beginPacket(tempServer, tempPort);
    Udp.write(packetBuffer, PACKET_SIZE);
    Udp.endPacket();
}

// vim: sw=4:expandtab:ts=4
