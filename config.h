/*
    CONFIGURATION
 */
// network
const char ssid[] = "tabr.org";
const char pass[] = "";
const IPAddress IPLocal(10, 1, 0, 38);  // 35 + SensorID
const IPAddress IPGateway(10, 1, 0, 1);
const IPAddress IPSubnet(255, 255, 255, 0);
const IPAddress IPServer(10, 1, 0, 9);
const unsigned int portServer = 1883;
const unsigned int maxPacketSize = 1400;

// behaviour/timing
const byte sleepWifiCheck = 50;     // ms to sleep between checks if associated
const byte maxConnRetry = 200;      // how many sleepWifiCheck sleeps
const unsigned int noConnSleepSec = 600;
const unsigned int SLEEPSEC = 310; // not very accurate, use 59/310 for 60/300
const byte sleepEnd = 100;          // ms to sleep before going to deepSleep
const byte sleepADCmeasure = 100;   // ms to sleep before each ADC read
const byte sleepUDP = 50;           // ms to sleep after each UDP packet
const unsigned int sleepDHT = 2000; // ms to sleep before querying DHT
const byte maxSensors = 8;

#define SERIALDEBUG

// hardware
const byte PIN_BLUELED = 1;         // set to -1 to disable feature
const byte BLUELED_ON = LOW;  // onboard-LED is active-LOW
const byte PIN_CONFIG = 5;          // pin that enables "config mode"
const byte ADC_MEASURES = 5;
const float Vdd = 3.3;
const float Voff = 0.00;        // highest V where ADC still reports "0"

// Sensors
const bool doNTC = false;
const bool doNTCraw = true;
const bool doDallas = true;
const bool doDHT = true;
const bool doDHTHI = true;
const bool doBattery = true;
const bool doBattraw = false;
const bool doPerf = true;
const bool doPerfraw = false;

// NTC
const byte PIN_NTC = 5;
const unsigned int NTC_ID = 42;
// temp calculation
const float Rfix = 4.7e3;   // pulldown
const float NTC_B = 3950;
const float NTC_R0 = 20e3;
const float Rinf = NTC_R0*exp(-NTC_B/298.15);  // (T0 = 25 + 273.15 = 298.15)

// Dallas
const byte PIN_DALLAS_POWER = 14;
const byte PIN_1WIRE = 13;
const byte DALLAS_RESOLUTION = 12;
const bool DALLAS_CHECKCONVERSION = false;  // set to false if sensor returns 85

// DHT
const byte PIN_DHT_POWER = 14;
const byte PIN_DHT = 13;
const byte DHT_TYPE = DHT22;        // DHT11 / DHT21 / DHT22

// Battery
const float battDivider = 10.0/66;    // 56k + 10k

// Webserver
const char indexPage[] =
R"(
<html>
    <head>
        <title>ESP_BaPoTeSta Maintenance</title>
        <style type="text/css">
            body {
                background-color: #eee;
                color: black;
            }
            h1, h2 {
                color: #220;
            }
            h2 {
                border-left:1px black solid;
                border-top:1px black solid;
                margin-left:4px;
                margin-top:4px;
            }
        </style>
    </head>
    <body>
        <h1>ESP_BaPoTeSta Maintenance</h1>
        <form method='POST' action='/config' enctype='multipart/form-data'>
        <h2>Network</h2>
        <ul>
            <li>ssid: <input type="text" name="ssid" /></li>
            <li>password: <input type="text" name="passwd" /></li>
            <li>IP: <input type="text" name="ip" /></li>
            <li>Subnet: <input type="text" name="subnet" maxlen=2 /></li>
            <li>Gateway: <input type="text" name="gw" /></li>
            <li>MQTT IP: <input type="text" name="mqttip" /></li>
            <li>MQTT Port: <input type="text" name="mqttport" /></li>
        </ul>
        <h2>Measuring</h2>
        <ul>
            <li>Use DS18B20: <input type="checkbox" name="usedallas" /></li>
            <li>&nbsp; resolution: <input type="text" name="dallasres" /></li>
            <li>&nbsp; wait for result: <input type="checkbox" name="dallaswait" /></li>
            <li>Use DHT: <input type="checkbox" name="usedht" /></li>
            <li>&nbsp; type: <input type="text" name="dhttype" />/li>
            <li>&nbsp; report HI: <input type="checkbox" name="dhthi" /></li>
            <li>Report battery: <input type="checkbox" name="battery" /></li>
            <li>&nbsp; divider: <input type="text" name="battdiv" /></li>
            <li>&nbsp; incl. raw value: <input type="checkbox" name="battraw" /></li>
            <li>Report performance: <input type="checkbox" name="doperf" /></li>
            <li>&nbsp; incl. raw value: <input type="checkbox" name="perfraw" /></li>
            <li>Measure every <input type="text" maxlen="4" name="deltat" /> seconds</li>
        </ul>
        <h2>Hardware</h2>
        <ul>
            <li>Pin for blue LED: <input type="text" maxlen="4" name="pinblue" /></li>
            <li>&nbsp; invert: <input type="checkbox" name="invblue" /></li>
            <li>Pin for config-mode: <input type="text" maxlen="4" name="pinconfig" /></li>
            <li>Pin for sensor power: <input type="text" maxlen="4" name="pinpwrsens" /></li>
            <li>Pin for Dallas data: <input type="text" maxlen="4" name="pindallas" /></li>
            <li>Pin for DHT data: <input type="text" maxlen="4" name="pindhtdata" /></li>
            <li>Number of ADC measurements: <input type="text" maxlen="4" name="adcmeas" /></li>
        </ul>
        <input type='submit' value='Apply'>
        </form>
        <hr>
        <h2>Firmware Update</h2>
        Firmware needs to be compiled for %d KiB flash size!<br />
        <form method='POST' action='/update' enctype='multipart/form-data'>
            Firmware: <input type='file' name='update'>
            <input type='submit' value='Update'>
        </form>
        <hr>
        Current firmware was built on )" __DATE__ " at " __TIME__ R"(
    </body>
</html>)";

/*
    END OF CONFIGURATION
 */

