/*
   Battery-Powered WiFi Temperature Station
   Send temperature data via UDP

   Martin Schuster 2015
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char ssid[] = "tabr.org";
WiFiUDP Udp;
IPAddress myAddr(10, 1, 0, 35);
IPAddress tempserver(10, 1, 0, 9);
#define MEASURES 5
#define PACKET_SIZE 5*MEASURES+2+11
unsigned int sensorValue[MEASURES];
#define SLEEPSEC 2

unsigned long int chipId;

// the setup function runs once when you press reset or power the board
void setup() {
    // initialize digital pin 13 as an output.
    // pinMode(4, OUTPUT);
    // Serial.begin(115200);
    // Serial.setDebugOutput(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid);
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);   // turn the LED on (HIGH is the voltage level)

    delay(2000);
    // long rssi = WiFi.RSSI();
    // Serial.print("RSSI: ");
    // Serial.println(rssi);
    chipId = ESP.getChipId();
}

// the loop function runs over and over again forever
void loop() {
    static byte led = 1;


    for (byte c=0; c<MEASURES; c++) {
        delay(100);
        sensorValue[c] = analogRead(A0);
    }
    delay(1000);
    sendTemp(sensorValue);

    // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED
    ESP.deepSleep(10e6 * SLEEPSEC, WAKE_RF_DEFAULT);
    //delay(1000);              // wait for a second
}

void sendTemp(unsigned int* temp) {
    static char packetBuffer[PACKET_SIZE];

    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, PACKET_SIZE);

    sprintf(packetBuffer, "0x%08x ", chipId);

    for (byte c=0; c<MEASURES; c++) {
        sprintf(11+packetBuffer+5*c, "%04d ", temp[c]);
    }
    packetBuffer[11+5*MEASURES-1] = '\n';

    Udp.beginPacket(tempserver, 9988);
    Udp.write(packetBuffer, PACKET_SIZE);
    Udp.endPacket();
}

// vim: sw=4:expandtab:ts=4
