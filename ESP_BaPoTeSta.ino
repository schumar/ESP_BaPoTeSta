/*
   Battery-Powered WiFi Temperature Station
   Send temperature data via UDP

   Martin Schuster 2015

ToDo:
    power PTC only when needed
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
const byte MEASURES = 5
const byte SLEEPSEC = 2
const byte PIN_LED = 4;

WiFiUDP Udp;
unsigned int sensorValue[MEASURES];
unsigned long int chipId;


void setup() {
    // activate LED to show that we are "on"
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

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

    // measure temp multiple times
    for (byte c=0; c<MEASURES; c++) {
        delay(100);
        sensorValue[c] = analogRead(A0);
    }

    delay(1000);
    sendTemp(sensorValue);

    // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED
    ESP.deepSleep(10e6 * SLEEPSEC, WAKE_RF_DEFAULT);
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
