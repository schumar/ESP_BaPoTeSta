/*
   Battery-Powered WiFi Temperature Station
   Send temperature data via UDP

   Martin Schuster 2015

Pins:
    CH_PD PullUp  GPIO 15 PullDn  GPIO 2 PullUp
    Normal: GPIO 0 PullUp
    Upload: GPIO 0 GND
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "config.h"
#include "ESP_BaPoTeSta.h"

// globals
struct sensorMeasurement sensorMeasurements[maxSensors];
struct allMeasurements data;
WiFiUDP Udp;
OneWire oneWire(2);
DallasTemperature dallasSensors(&oneWire);

void setup() {
    // activate (active low) blue LED to show that we are "on"
    pinMode(PIN_BLUELED, OUTPUT);
    digitalWrite(PIN_BLUELED, BLUELED_ON);

    // start WiFi
    WiFi.mode(WIFI_STA);
    WiFi.config(IPLocal, IPGateway, IPSubnet);
    if (pass == NULL || pass[0] == 0) {
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, pass);
    }

    // now is a perfect time for "other" stuff, as WiFi will need some time
    // to associate

    // setup Dallas sensors
    dallasSensors.begin();
    dallasSensors.setResolution(DALLAS_RESOLUTION);

    // get ChipID, will be used as unique ID when sending data
    data.chipId = ESP.getChipId();
    data.sensorMeasurements = sensorMeasurements;

    // wait until WiFi is connected, but max maxConnRetry
    for (byte i = 0;
            i < maxConnRetry && WiFi.status() != WL_CONNECTED;
            i++)
        delay(50);
    // if this didn't work, go back to sleep
    if (WiFi.status() != WL_CONNECTED)
        gotoSleep(noConnSleepSec);

}


// this won't actually "loop", as the last command leads to a reset
void loop() {
    data.timestep = 0;      // [XXX] this needs to be read from eprom and ++
    data.nrMeasurements = 0;

    powerSensors(true);     // activate power to sensors
    collectData();          // make mesurements
    powerSensors(false);    // deactivate power to sensors again

    sendData();

    // wait a little bit, to ensure that everything is sent
    delay(100);
    gotoSleep(SLEEPSEC);
}

void collectData() {
    if (doNTC) getNTC();
    if (doDallas) getDallas();
}

void getNTC() {
    int sensorValue[NTC_MEASURES];

    // measure temp multiple times
    for (byte c=0; c<NTC_MEASURES; c++) {
        delay(100);

        unsigned int adc = analogRead(A0);
        if (adc == 0 || adc >= 1023) return; // don't report wrong values

        sensorValue[c] = adc;
    }

    // calculate median
    bubbleSort(sensorValue, NTC_MEASURES);
    // as NTC_MEASURES is odd, we can just take the middle sample
    addData(NTC_ID, TEMP, calcTemp(sensorValue[NTC_MEASURES/2]), CENT_DEGC);
    if (doNTCraw)
        addData(NTC_ID, TEMP, sensorValue[NTC_MEASURES/2], RAW);
}

void getDallas() {
    uint8_t addr[8];
    float temp;

    // try to find address of first sensor
    if (dallasSensors.getAddress(addr, 0) == 0) return;

    dallasSensors.requestTemperaturesByAddress(addr);
    temp = dallasSensors.getTempC(addr);

    // use last two byte of serial as ID (addr[0] is "family code")
    addData(addr[2]<<8 + addr[1], TEMP, temp * 100.0, CENT_DEGC);
}

void addData(unsigned int sensorId, enum sensorType type,
        float value, enum unitType unit) {
    byte idx = data.nrMeasurements++;
    if (idx >= maxSensors) return;

    sensorMeasurements[idx].sensorId = sensorId;
    sensorMeasurements[idx].type = type;
    sensorMeasurements[idx].value = value;
    sensorMeasurements[idx].unit = unit;
}

void sendData() {
    static char packetBuffer[maxPacketSize];
    unsigned int pos = 0;           // cursor

    pos +=
        snprintf(packetBuffer, maxPacketSize - pos,
                "{\n"
                "  \"chipId\": 0x%08x,\n"
                "  \"timestep\": %d,\n",
                data.chipId, data.timestep);

    pos +=
        snprintf(packetBuffer, maxPacketSize - pos,
                "  \"measurements\": [\n");

    for (byte i = 0; i < data.nrMeasurements; i++) {
        pos +=
            snprintf(packetBuffer, maxPacketSize - pos,
                    "    {\n"
                    "      \"sensorId\": 0x%02x,\n"
                    "      \"sensorType\": %d,\n"
                    "      \"value\": %d,\n"
                    "      \"unitType\": %d\n"
                    "    },\n"
                    , data.sensorMeasurements[i].sensorId,
                    data.sensorMeasurements[i].type,
                    data.sensorMeasurements[i].value,
                    data.sensorMeasurements[i].unit
                    );
    }

    pos +=
        snprintf(packetBuffer, maxPacketSize - pos,
                "  ]\n}\n");

    // check that we aren't out-of-bounds
    if (pos >= maxPacketSize-2) return;
    packetBuffer[pos++] = '\000';

    // send packet (twice, to make sure)
    for (byte i=0; i<2; i++) {
        Udp.beginPacket(IPServer, portServer);
        Udp.write(packetBuffer, pos);
        Udp.endPacket();
        delay(50);
    }
}

void powerSensors(bool on) {
    if (doNTC) powerNTC(on);
    if (doDallas) powerDallas(on);
}

void powerNTC(bool on) {
    pinMode(PIN_NTC, OUTPUT);
    digitalWrite(PIN_NTC, on ? HIGH : LOW);
}

void powerDallas(bool on) {
    pinMode(PIN_DALLAS, OUTPUT);
    digitalWrite(PIN_DALLAS, on ? HIGH : LOW);
}


float calcTemp(unsigned int raw) {
    /*
       V = Vdd * Rfix / (Rfix + NTC)
       raw/1024 = V - Voff

       NTC = Vdd*Rfix / V - Rfix
           = Vdd*Rfix / (raw/1024 + Voff) - Rfix

       T = B / ln(R/Rinf)
     */
    float ntc = Vdd * Rfix / (raw/1024. + Voff) - Rfix;
    float temp = NTC_B / log(ntc/Rinf) - 273.15;
    // float temp = raw * 0.01;  // use this to test ADC
    // float temp = ntc * 1e-3;  // use this to test the NTC
    return temp;
}

void bubbleSort(float * analogValues, int nr) {
    int out, in, swapper;
    for(out=0 ; out < nr; out++) {  // outer loop
        for(in=out; in<(nr-1); in++)  {  // inner loop
            if( analogValues[in] > analogValues[in+1] ) {   // out of order?
                // swap them:
                swapper = analogValues[in];
                analogValues [in] = analogValues[in+1];
                analogValues[in+1] = swapper;
            }
        }
    }
}

void bubbleSort(int * analogValues, int nr) {
    int out, in, swapper;
    for(out=0 ; out < nr; out++) {  // outer loop
        for(in=out; in<(nr-1); in++)  {  // inner loop
            if( analogValues[in] > analogValues[in+1] ) {   // out of order?
                // swap them:
                swapper = analogValues[in];
                analogValues [in] = analogValues[in+1];
                analogValues[in+1] = swapper;
            }
        }
    }
}

void gotoSleep(unsigned int seconds) {
    // switch off (active low) blue LED to show that we are "off"
    digitalWrite(PIN_BLUELED, 1 - BLUELED_ON);

    // go to sleep, reboot after 'seconds' seconds
    // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED
    ESP.deepSleep(1e6 * seconds, WAKE_NO_RFCAL);
}


// vim: sw=4:expandtab:ts=4
