/*
   ESP8266 Battery Powered Temperature Station
   https://git.io/ESPTemp

   Martin Schuster 2015 - 2016
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
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

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

Adafruit_BMP280 bmp280;

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
    delay(10);
    debugPrint("\nSYNCSYNC\n");
    #endif

    EEPROM.begin(1024);
    getConfig();

    // check if "config mode" jumper is set
    if (digitalRead(config.pinconfig)) {
        debugPrint("Starting webserver");

        configmode = true;
        setupWebserver();

    } else {
        debugPrint("Starting normally");

        configmode = false;
        setupNormal();
    }
}

void setupNormal() {
    char idBuffer[32];

    // start WiFi
    debugPrint("Connecting to WiFi...");
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
    if (config.usedht) {
        dhtSensor = DHT(config.pindhtdata, config.dhttype);
        dhtSensor.begin();
    }

    // setup I2C
    if (config.usebmp280) {
        Wire.begin(config.pini2csda, config.pini2cscl);
    }

    // setup BMP280
    if (config.usebmp280) {
        // if bmp280addr is set, use it, otherwise stick to the default (0x77)
        if (! (config.bmp280addr > 0 ?
                    bmp280.begin(config.bmp280addr) : bmp280.begin())) {
            debugPrint("BMP280 not found.");
        }
    }

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

    debugPrint("Connected to WiFi.");

    // connect to MQTT server
    mqttClient.setServer(config.mqttip, config.mqttport);
    sprintf(idBuffer, "esp8266-%08lx", data.chipId);
    mqttClient.connect(idBuffer);
    debugPrint("Connected to MQTT server.");

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
    debugPrint("Collecting data...");
    if (config.battery) getBattery();
    if (config.usedallas) getDallas();
    if (config.usebmp280) getBMP280();
    if (config.usedht) getDHT();
    if (config.dowifi) getWiFi();
    if (config.doperf) getPerf();
}

void getDallas() {
    uint8_t addr[8];
    float temp;
    uint16_t id;

    debugPrint("Searching for DS18B20");

    // try to find address of first sensor
    if (dallasSensors.getAddress(addr, 0) == 0) return;

    dallasSensors.requestTemperaturesByAddress(addr);
    temp = dallasSensors.getTempC(addr);

    debugPrint("DSTmp=" + (String)temp);

    // sanity check; valid range taken from datasheet
    // "85" is the "reset value", ignore it too
    if (temp < -55.0 || temp > 125.0 || (temp > 84.95 && temp < 85.05))
        return;

    // add the bias after checking for "85" above
    temp += config.biasDallasTemp;

    if (DALLAS_ID > 0) {
        id = DALLAS_ID;
    } else {
        id = (addr[2]<<8) + addr[1];
    }

    // use last two byte of serial as ID (addr[0] is "family code")
    addData(id, TEMP, (int) (temp * 100.0), CENT_DEGC);
}

void getBMP280() {
    float temp;
    float pres;

    debugPrint("Querying BMP280...");

    temp = bmp280.readTemperature();
    pres = bmp280.readPressure();

    debugPrint("BMP280Temp=" + (String)temp + "\n" +
               "BMP280Pres=" + (String)pres);

    if (pres < 1.0)
        return;

    addData(0x0280, TEMP, (int) (temp * 100.0), CENT_DEGC);

    if (config.bmppress) {
        // report measured pressure (current height)
        addData(0x0280, PRESSURE, (int) (pres), PASCAL);
    }
    if (config.bmpslp && config.heightASL > 0) {
        // report pressure at sea level
        addData(0x0280, PRESSUREASL, (int) (sealevelPressure(pres)), PASCAL);
    }
}

void getDHT() {
    delay(sleepDHT);    // [XXX] should substract the time since power-up

    // store values, so they can be used for calculating the heat index later
    float temp = dhtSensor.readTemperature(false, true) + config.biasDHTTemp;
    float hum = dhtSensor.readHumidity() + config.biasDHTHumid;

    debugPrint("DHTHum=" + (String)hum);
    debugPrint("DHTTmp=" + (String)temp);
    // sanity check; valid range taken from datasheet
    if (temp < -40.0 || temp > 80.0 || hum < 0.0 || hum > 100.0 || isnan(temp) || isnan(hum))
        return;

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
    float volt;

    raw = readADC();
    volt = calcBattery(raw);

    // If battery voltage is below 3.0V, the results from most sensors will
    // be totally off -> don't even try anymore
    if (volt < VLowBat) {
        debugPrint("LOW BATTERY");
        gotoSleep(lowBattSleepSec);
    }

    addData(0, BATTERY, (int) (volt * 1000.0), MVOLT);
    if (config.battraw)
        addData(0, BATTERY, raw, RAW);
}

void getWiFi() {
    addData(0, WIFIRSSI, WiFi.RSSI(), DBM);
}

void getPerf() {
    // simply "measure" the number of CPU cycles since bootup
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
    if (config.usedallas || config.usedht || config.usebmp280) {
        pinMode(config.pinpwrsens, OUTPUT);
        digitalWrite(config.pinpwrsens, on ? HIGH : LOW);
        // when switching "off", ensure that the pin is not connected to GND or Vcc anymore
        // by changing it to INPUT
        if (! on) pinMode(config.pinpwrsens, INPUT);
    }
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

float sealevelPressure(float pressure) {
    // calculate the pressure at sea level, given the measured pressure
    // and the height it was measured at
    float slp;

    slp = pressure / (
            exp( log(1 - 1.0*config.heightASL/44330) / 0.1903 )
            );
    return slp;
}

void gotoSleep(unsigned int seconds) {

    debugPrint("Going to sleep.");

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
    httpServer.on("/style.css",  HTTP_GET, &webCSS);
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

IPAddress stringToIP (String text) {
    byte octet[4];
    byte pos = 0;

    for (byte i = 0; i < 4; i++) {
        byte dot = text.indexOf('.', pos);
        octet[i] = text.substring(pos, dot).toInt();
        pos = dot + 1;
    }

    return IPAddress(octet[0], octet[1], octet[2], octet[3]);
}

void webForm() {
    // called when client does a  GET /
    String buf;

    buf = indexPage;
    buf.replace("${flashsize}", String(ESP.getFlashChipRealSize()/1024));
    buf.replace("${buildtime}", String(__DATE__ + String(" at ") + __TIME__));

    buf.replace("${ssid}", String(config.ssid));
    buf.replace("${password}", String(config.password));
    buf.replace("${ip}", ipToString(config.ip));
    buf.replace("${netmask}", String(config.netmask));
    buf.replace("${gw}", ipToString(config.gw));
    buf.replace("${mqttip}", ipToString(config.mqttip));
    buf.replace("${mqttport}", String(config.mqttport));

    buf.replace("${usedallas}", config.usedallas ? "checked" : "");
    buf.replace("${dallasres}", String(config.dallasres));
    buf.replace("${biasdallastemp}", String(config.biasDallasTemp));
    buf.replace("${dallaswait}", config.dallaswait ? "checked" : "");

    buf.replace("${usedht}", config.usedht ? "checked" : "");
    buf.replace("${dhttype}", String(config.dhttype));
    buf.replace("${biasdhttemp}", String(config.biasDHTTemp));
    buf.replace("${biasdhthumid}", String(config.biasDHTHumid));
    buf.replace("${dhthi}", config.dhthi ? "checked" : "");

    buf.replace("${battery}", config.battery ? "checked" : "");
    buf.replace("${battraw}", config.battraw ? "checked" : "");

    buf.replace("${dowifi}", config.dowifi ? "checked" : "");

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

    httpServer.send(200, "text/html", buf);
}

void webCSS() {
    httpServer.send(200, "text/css", css);
}

void getConfig() {
    // read config from EEPROM

    // check if the first byte is "magic" (i.e. EEPROM has been written before)
    if (EEPROM.read(0) == 0x42) {
        debugPrint("EEPROM magic byte 0x42 found.");
        // check version of config storage
        if (EEPROM.read(1) == config.cfgversion) {
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

        } else {
            debugPrint("Ignoring EEPROM, wrong cfg version");
        }
    } else {
        // otherwise we rely on the defaults
        debugPrint("No config found, using hardcoded defaults.");
    }
}

void storeConfig() {
    String buf = "";

    debugPrint("Received form data");

    buf += (String)"Number of received args: " + httpServer.args() + "\n";
    for (int i = 0; i < httpServer.args(); i++) {
        buf += httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
    }

    if (httpServer.hasArg("ssid"))
        httpServer.arg("ssid").toCharArray(config.ssid, 32);

    if (httpServer.hasArg("password"))
        httpServer.arg("password").toCharArray(config.password, 32);

    if (httpServer.hasArg("ip"))
        config.ip = stringToIP(httpServer.arg("ip"));

    if (httpServer.hasArg("netmask"))
        config.netmask = httpServer.arg("netmask").toInt();

    if (httpServer.hasArg("gw"))
        config.gw = stringToIP(httpServer.arg("gw"));

    if (httpServer.hasArg("mqttip"))
        config.mqttip = stringToIP(httpServer.arg("mqttip"));

    if (httpServer.hasArg("mqttport"))
        config.mqttport = httpServer.arg("mqttport").toInt();

    if (httpServer.hasArg("usedallas"))
        config.usedallas = true;
    else
        config.usedallas = false;

    if (httpServer.hasArg("dallasres"))
        config.dallasres = httpServer.arg("dallasres").toInt();

    if (httpServer.hasArg("dallaswait"))
        config.dallaswait = true;
    else
        config.dallaswait = false;

    if (httpServer.hasArg("usedht"))
        config.usedht = true;
    else
        config.usedht = false;

    if (httpServer.hasArg("dhttype"))
        config.dhttype = httpServer.arg("dhttype").toInt();

    if (httpServer.hasArg("dhthi"))
        config.dhthi = true;
    else
        config.dhthi = false;

    if (httpServer.hasArg("battery"))
        config.battery = true;
    else
        config.battery = false;

    if (httpServer.hasArg("battraw"))
        config.battraw = true;
    else
        config.battraw = false;

    if (httpServer.hasArg("dowifi"))
        config.dowifi = true;
    else
        config.dowifi = false;

    if (httpServer.hasArg("doperf"))
        config.doperf = true;
    else
        config.doperf = false;

    if (httpServer.hasArg("perfraw"))
        config.perfraw = true;
    else
        config.perfraw = false;

    if (httpServer.hasArg("deltat"))
        config.deltat = httpServer.arg("deltat").toInt();

    if (httpServer.hasArg("pinblue"))
        config.pinblue = httpServer.arg("pinblue").toInt();

    if (httpServer.hasArg("invblue"))
        config.invblue = true;
    else
        config.invblue = false;

    if (httpServer.hasArg("pinconfig"))
        config.pinconfig = httpServer.arg("pinconfig").toInt();

    if (httpServer.hasArg("pinpwrsens"))
        config.pinpwrsens = httpServer.arg("pinpwrsens").toInt();

    if (httpServer.hasArg("pindallas"))
        config.pindallas = httpServer.arg("pindallas").toInt();

    if (httpServer.hasArg("pindhtdata"))
        config.pindhtdata = httpServer.arg("pindhtdata").toInt();

    if (httpServer.hasArg("adcmeas"))
        config.adcmeas = httpServer.arg("adcmeas").toInt();

    if (httpServer.hasArg("battdiv"))
        config.battdiv = httpServer.arg("battdiv").toFloat();

    debugPrint("Saving form data");
    EEPROM.put(0, 0x42);        // magic byte
    EEPROM.put(1, config);
    EEPROM.commit();            // ESP8266 EEPROM library needs this

    httpServer.send(200, "text/plain", buf);
}


void debugPrint(String msg) {
    #ifdef SERIALDEBUG
    Serial.println(msg);
    #endif
}

// vim: sw=4:expandtab:ts=4:tw=80
