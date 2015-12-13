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
OneWire oneWire(PIN_1WIRE);
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
        delay(sleepWifiCheck);
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
    delay(sleepEnd);
    gotoSleep(SLEEPSEC);
}

void collectData() {
    if (doNTC) getNTC();
    if (doDallas) getDallas();
    if (doBattery) getBattery();
}

void getNTC() {
    int raw;

    raw = readADC();

    addData(NTC_ID, TEMP, (int) (100.0*calcNTCTemp(raw)), CENT_DEGC);
    if (doNTCraw)
        addData(NTC_ID, TEMP, raw, RAW);
}

void getDallas() {
    uint8_t addr[8];
    float temp;

    // try to find address of first sensor
    if (dallasSensors.getAddress(addr, 0) == 0) return;

    dallasSensors.requestTemperaturesByAddress(addr);
    temp = dallasSensors.getTempC(addr);

    // use last two byte of serial as ID (addr[0] is "family code")
    addData(addr[2]<<8 + addr[1], TEMP, (int) (temp * 100.0), CENT_DEGC);
}

int readADC() {
    int sensorValue[ADC_MEASURES];

    // measure multiple times
    for (byte c=0; c<ADC_MEASURES; c++) {
        delay(sleepADCmeasure);
        sensorValue[c] = analogRead(A0);
    }

    // calculate median
    bubbleSort(sensorValue, ADC_MEASURES);

    // as ADC_MEASURES is odd, we can just take the middle sample
    return(sensorValue[ADC_MEASURES/2]);
}

void getBattery() {
    int raw;

    // Sanity check: We only have 1 ADC, so we can't measure battery AND NTC
    if (doNTC) return;

    addData(0, BATTERY, calcBattery(raw), VOLT);
    if (doBattraw)
        addData(0, BATTERY, raw, RAW);
}

void addData(unsigned int sensorId, enum sensorType type,
        int value, enum unitType unit) {
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
        snprintf(packetBuffer+pos, maxPacketSize - pos,
                "{\n"
                "  \"chipId\": %d,\n"
                "  \"timestep\": %d,\n",
                data.chipId, data.timestep);

    pos +=
        snprintf(packetBuffer+pos, maxPacketSize - pos,
                "  \"measurements\": [\n");

    for (byte i = 0; i < data.nrMeasurements; i++) {
        // if this isn't the first measurement, put a comma between the previous
        // and this one
        if (i>0)
            pos += snprintf(packetBuffer+pos, maxPacketSize - pos, ",\n");

        pos +=
            snprintf(packetBuffer+pos, maxPacketSize - pos,
                    "    {\n"
                    "      \"sensorId\": %d,\n"
                    "      \"sensorType\": %d,\n"
                    "      \"value\": %d,\n"
                    "      \"unitType\": %d\n"
                    "    }"
                    , data.sensorMeasurements[i].sensorId,
                    data.sensorMeasurements[i].type,
                    data.sensorMeasurements[i].value,
                    data.sensorMeasurements[i].unit
                    );
    }

    pos +=
        snprintf(packetBuffer+pos, maxPacketSize - pos,
                "\n  ]\n}\n");

    // check that we aren't out-of-bounds
    if (pos >= maxPacketSize-2) return;
    packetBuffer[pos++] = '\000';

    // send packet (twice, to make sure)
    for (byte i=0; i<2; i++) {
        Udp.beginPacket(IPServer, portServer);
        Udp.write(packetBuffer, pos);
        Udp.endPacket();
        delay(sleepUDP);
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
    pinMode(PIN_DALLAS_POWER, OUTPUT);
    digitalWrite(PIN_DALLAS_POWER, on ? HIGH : LOW);
}


float calcNTCTemp(unsigned int raw) {
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

float calcBattery(int raw) {
    /*
       V = Vbatt * battDivider
       raw/1024 = V - Voff

       Vbatt = V / battDivider

     */
    return ((raw/1024. + Voff) / battDivider);
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
