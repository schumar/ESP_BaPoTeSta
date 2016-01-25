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
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>

#ifdef SERIALDEBUG
#include <Serial.h>
#endif

#include "config.h"
#include "ESP_BaPoTeSta.h"

// globals
struct sensorMeasurement sensorMeasurements[maxSensors];
struct allMeasurements data;
struct config config;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
OneWire oneWire(0xff);
DallasTemperature dallasSensors(&oneWire);
DHT dhtSensor(0,0);
bool configmode = false;
ESP8266WebServer httpServer(80);
IPAddress IPSubnet(255, 255, 255, 0);
float ntc_rinf;

#ifdef SERIALDEBUG
ESP8266HTTPUpdateServer httpUpdater(true);
#else
ESP8266HTTPUpdateServer httpUpdater;
#endif


void setup() {
    // activate (active low) blue LED to show that we are "on"
    if (config.pinblue >= 0) {
        pinMode(config.pinblue, OUTPUT);
        digitalWrite(config.pinblue, config.invblue ? LOW : HIGH);
    }

    #ifdef SERIALDEBUG
    Serial.begin(115200);
    #endif

    EEPROM.begin(1024);
    getConfig();

    // check if "config mode" jumper is set
    if (digitalRead(config.pinconfig)) {
        debugPrint("\nStarting webserver");

        configmode = true;
        setupWebserver();

    } else {
        debugPrint("\nStarting normally");

        configmode = false;
        setupNormal();
    }
}

void setupNormal() {
    char idBuffer[32];

    // start WiFi
    WiFi.mode(WIFI_STA);
    WiFi.config(config.ip, config.gw, IPSubnet);
    if (config.password == NULL || config.password[0] == 0) {
        WiFi.begin(config.ssid);
    } else {
        WiFi.begin(config.ssid, config.password);
    }

    // now is a perfect time for "other" stuff, as WiFi will need some time
    // to associate

    powerSensors(true);     // activate power to sensors

    // setup Dallas sensors
    if (config.usedallas) {
        oneWire = OneWire(config.pindallas);
        dallasSensors.begin();
        dallasSensors.setResolution(config.dallasres);
        dallasSensors.setCheckForConversion(config.dallaswait);
    }

    // setup DHT sensor
    if (config.usedht)
        dhtSensor = DHT(config.pindhtdata, config.dhttype);
        dhtSensor.begin();

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

    // connect to MQTT server
    mqttClient.setServer(config.mqttip, config.mqttport);
    sprintf(idBuffer, "esp8266-%08lx", data.chipId);
    mqttClient.connect(idBuffer);

}


void loop() {
    if (configmode) {
        httpServer.handleClient();
        delay(1);
    } else {
        // this won't actually "loop", as the last command leads to a reset
        data.timestep = 0;      // [XXX] this needs to be read from eprom and ++
        data.nrMeasurements = 0;

        mqttClient.loop();

        collectData();          // make mesurements
        powerSensors(false);    // deactivate power to sensors again

        sendData();
        mqttClient.loop();

        // wait a little bit, to ensure that everything is sent
        delay(sleepEnd);
        mqttClient.loop();
        gotoSleep(config.deltat);
    }
}

void collectData() {
    if (config.usentc) getNTC();
    if (config.usedallas) getDallas();
    if (config.usedht) getDHT();
    if (config.battery) getBattery();
    if (config.doperf) getPerf();
}

void getNTC() {
    int raw;

    raw = readADC();

    addData(NTC_ID, TEMP, (int) (100.0*calcNTCTemp(raw)), CENT_DEGC);
    if (config.ntcraw)
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
    addData((addr[2]<<8) + addr[1], TEMP, (int) (temp * 100.0), CENT_DEGC);
}

void getDHT() {
    delay(sleepDHT);    // [XXX] should substract the time since power-up

    // store values, so they can be used for calculating the heat index later
    float temp = dhtSensor.readTemperature(false, true);
    float hum = dhtSensor.readHumidity();

    // use the DHT_TYPE as sensor ID, as the DHT doesn't have a real ID
    addData(config.dhttype, TEMP, (int) (temp * 100.0), CENT_DEGC);
    addData(config.dhttype, HUMIDITY, (int) (hum * 100.0), CENT_PERC);

    if (config.dhthi)
        addData(config.dhttype, TEMPHI,
                (int) (dhtSensor.computeHeatIndex(temp, hum, false) * 100.0),
                CENT_DEGC);
}

int readADC() {
    int sensorValue[config.adcmeas];

    // measure multiple times
    for (byte c=0; c<config.adcmeas; c++) {
        delay(sleepADCmeasure);
        sensorValue[c] = analogRead(A0);
    }

    // calculate median
    bubbleSort(sensorValue, config.adcmeas);

    // as config.adcmeas is odd, we can just take the middle sample
    return(sensorValue[config.adcmeas/2]);
}

void getBattery() {
    int raw;

    // Sanity check: We only have 1 ADC, so we can't measure battery AND NTC
    if (config.usentc) return;

    raw = readADC();
    addData(0, BATTERY, (int) (calcBattery(raw) * 1000.0), MVOLT);
    if (config.battraw)
        addData(0, BATTERY, raw, RAW);
}

void getPerf() {
    // simple "measure" the number of CPU cycles since bootup
    unsigned long int cycles;
    cycles = ESP.getCycleCount();
    addData(0, TIME, cycles/(F_CPU/1e6), USEC);
    if (config.perfraw)
        addData(0, TIME, cycles, RAW);
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
    char payloadBuffer[16];
    char topicBuffer[128];

    for (byte i = 0; i < data.nrMeasurements; i++) {
        snprintf(topicBuffer, 128,
                "chip-%08lx/sensor-%d/%s-%s",
                data.chipId, data.sensorMeasurements[i].sensorId,
                sensorTypeName[data.sensorMeasurements[i].type],
                unitTypeName[data.sensorMeasurements[i].unit]);

        snprintf(payloadBuffer, 16,
                "%d", data.sensorMeasurements[i].value);

        mqttClient.publish(topicBuffer, payloadBuffer);
    }

}

void powerSensors(bool on) {
    // only power the pin if actually needed
    if (config.usentc || config.usedallas || config.usedht) {
        pinMode(config.pinpwrsens, OUTPUT);
        digitalWrite(config.pinpwrsens, on ? HIGH : LOW);
    }
}


float calcNTCTemp(unsigned int raw) {
    /*
       V = Vdd * Rfix / (Rfix + NTC)
       raw/1024 = V - Voff

       NTC = Vdd*Rfix / V - Rfix
           = Vdd*Rfix / (raw/1024 + Voff) - Rfix

       T = B / ln(R/Rinf)
     */
    float ntc = Vdd * config.ntcrfix / (raw/1024. + Voff) - config.ntcrfix;
    float temp = config.ntc_b / log(ntc/ntc_rinf) - 273.15;
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
    return ((raw/1024. + Voff) / config.battdiv);
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
    if (config.pinblue >= 0) {
        digitalWrite(config.pinblue, config.invblue ? HIGH : LOW);
    }

    // go to sleep, reboot after 'seconds' seconds
    // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED
    ESP.deepSleep(1e6 * seconds, WAKE_NO_RFCAL);
}

void setupWebserver() {
    char host[32];

    unsigned long int chipId = ESP.getChipId();
    sprintf(host, "chip-%08lx", chipId);

    WiFi.mode(WIFI_AP);
    char passwd[] = "bapotesta";
    WiFi.softAP(host, passwd);

    MDNS.begin(host);

    httpUpdater.setup(&httpServer);
    httpServer.on("/",  HTTP_GET, &webForm);
    httpServer.on("/config",  HTTP_POST, &storeConfig);
    httpServer.begin();

    MDNS.addService("http", "tcp", 80);

    #ifdef SERIALDEBUG
    Serial.printf("Ready! Connect to SSID %s with password '%s'"
            " and visit http://%s.local/update\n", host, passwd, host);
    #endif
}

String ipToString (IPAddress ip) {
    const String delim = ".";
    return String(
            ip[0] + delim + ip[1] + delim + ip[2] + delim + ip[3]
            );
}

void webForm() {
    // called when client does a  GET /
    String buf;

    buf = indexPage;
    buf.replace("${flashsize}", String(ESP.getFlashChipRealSize()/1024));
    buf.replace("${buildtime}", String(__DATE__ + String(" at ") + __TIME__));

    buf.replace("${ssid}", String(config.ssid));
    buf.replace("${password}", String(config.password));
    buf.replace("${ssid}", String(config.ssid));
    buf.replace("${password}", String(config.password));
    buf.replace("${ip}", ipToString(config.ip));
    buf.replace("${netmask}", String(config.netmask));
    buf.replace("${gw}", ipToString(config.gw));
    buf.replace("${mqttip}", ipToString(config.mqttip));
    buf.replace("${mqttport}", String(config.mqttport));
    buf.replace("${usedallas}", config.usedallas ? "checked" : "");
    buf.replace("${dallasres}", String(config.dallasres));
    buf.replace("${dallaswait}", config.dallaswait ? "checked" : "");
    buf.replace("${usedht}", config.usedht ? "checked" : "");
    buf.replace("${dhttype}", String(config.dhttype));
    buf.replace("${dhthi}", config.dhthi ? "checked" : "");
    buf.replace("${usentc}", config.usentc ? "checked" : "");
    buf.replace("${ntcraw}", config.ntcraw ? "checked" : "");
    buf.replace("${battery}", config.battery ? "checked" : "");
    buf.replace("${battraw}", config.battraw ? "checked" : "");
    buf.replace("${doperf}", config.doperf ? "checked" : "");
    buf.replace("${perfraw}", config.perfraw ? "checked" : "");
    buf.replace("${deltat}", String(config.deltat));
    buf.replace("${pinblue}", String(config.pinblue));
    buf.replace("${invblue}", config.invblue ? "checked" : "");
    buf.replace("${pinconfig}", String(config.pinconfig));
    buf.replace("${pinpwrsens}", String(config.pinpwrsens));
    buf.replace("${pindallas}", String(config.pindallas));
    buf.replace("${pindhtdata}", String(config.pindhtdata));
    buf.replace("${adcmeas}", String(config.adcmeas));
    buf.replace("${battdiv}", String(config.battdiv));
    buf.replace("${ntcrfix}", String(config.ntcrfix));
    buf.replace("${ntc_b}", String(config.ntc_b));
    buf.replace("${ntc_r0}", String(config.ntc_r0));

    httpServer.send(200, "text/html", buf);
}

void getConfig() {
    // read config from EEPROM

    // check if the first byte is "magic" (i.e. EEPROM has been written before)
    if (EEPROM.read(0) == 0x42) {
        // check version of config storage
        if (EEPROM.read(1) == 1) {
            // read config
            EEPROM.get(1, config);

            // conversions

            // convert netmask "/25" -> "255.255.255.128"
            // [XXX] there must be an easier way to do this
            if (config.netmask < 8+1) {
                IPSubnet = { (uint8_t)(0xff - (0xff >> config.netmask)),
                    0, 0, 0};
            } else if (config.netmask < 16+1) {
                IPSubnet = { 0xff,
                    (uint8_t)(0xff - (0xff >> (config.netmask-8))), 0, 0};
            } else if (config.netmask < 24+1) {
                IPSubnet = { 0xff, 0xff,
                    (uint8_t)(0xff - ((uint8_t)0xff >> (config.netmask-16))), 0};
            } else {
                IPSubnet = { 0xff, 0xff, 0xff,
                    (uint8_t)(0xff - ((uint8_t)0xff >> (config.netmask-24)))};
            }

            // (T0 = 25 + 273.15 = 298.15)
            ntc_rinf = config.ntc_r0*exp(-config.ntc_b/298.15);

        } else {
            debugPrint("Ignoring EEPROM, cfg version >1");
        }
    }
    // otherwise we rely on the defaults
}

void storeConfig() {
    String buf;
    buf = "Number of received args: " + httpServer.args();
    httpServer.send(200, "text/plain", buf);
}


void debugPrint(const char * msg) {
    #ifdef SERIALDEBUG
    Serial.println(msg);
    #endif
}

// vim: sw=4:expandtab:ts=4:tw=80
