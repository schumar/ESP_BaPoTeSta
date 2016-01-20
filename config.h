/*
    CONFIGURATION
 */

// behaviour/timing
const byte sleepWifiCheck = 50;     // ms to sleep between checks if associated
const byte maxConnRetry = 200;      // how many sleepWifiCheck sleeps
const unsigned int noConnSleepSec = 600;
const byte sleepEnd = 100;          // ms to sleep before going to deepSleep
const byte sleepADCmeasure = 100;   // ms to sleep before each ADC read
const byte sleepUDP = 50;           // ms to sleep after each UDP packet
const unsigned int sleepDHT = 2000; // ms to sleep before querying DHT
const byte maxSensors = 8;

#define SERIALDEBUG

// hardware
const float Vdd = 3.3;
const float Voff = 0.00;        // highest V where ADC still reports "0"

// Sensors
const bool doNTC = false;
const bool doNTCraw = true;

// NTC
const byte PIN_NTC = 5;
const unsigned int NTC_ID = 42;

// Dallas
const byte DALLAS_RESOLUTION = 12;
const bool DALLAS_CHECKCONVERSION = false;  // set to false if sensor returns 85

// DHT
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
        <form method="POST" action="/config" enctype="multipart/form-data">
        <h2>Network</h2>
        <ul>
            <li><label>ssid: <input type="text" name="ssid" value="${ssid}" /></label></li>
            <li><label>password: <input type="text" name="password" placeholder="empty or at least 8 chars" value="${password}" /></label></li>
            <li><label>IP: <input type="text" name="ip" value="${ip}" pattern="\d\d?\d?.\d\d?\d?.\d\d?\d?.\d\d?\d?" /></label></li>
            <li><label>Netmask: <input type="number" min="0" max="31" name="netmask" value="${netmask}" /></label></li>
            <li><label>Gateway: <input type="text" pattern="\d\d?\d?.\d\d?\d?.\d\d?\d?.\d\d?\d?" name="gw" value="${gw}" /></label></li>
            <li><label>MQTT IP: <input type="text" pattern="\d\d?\d?.\d\d?\d?.\d\d?\d?.\d\d?\d?" name="mqttip" value="${mqttip}" /></label></li>
            <li><label>MQTT Port: <input type="number" min="1" max="65535" name="mqttport" value="${mqttport}" /></label></li>
        </ul>
        <h2>Measuring</h2>
        <ul>
            <li><label>Use DS18B20: <input type="checkbox" name="usedallas" ${usedallas} /></label></li>
            <li><label>&nbsp; resolution: <input type="number" min="9" max="12" name="dallasres" value="${dallasres}" /></label></li>
            <li><label>&nbsp; check for result: <input type="checkbox" name="dallaswait" ${dallaswait} /></label></li>
            <li><label>Use DHT: <input type="checkbox" name="usedht" ${usedht} /></label></li>
            <li><label>&nbsp; type: <input type="number" min="11" max="33" name="dhttype" value="${dhttype}" /></label></li>
            <li><label>&nbsp; report HI: <input type="checkbox" name="dhthi" ${dhthi} /></label></li>
            <li><label>Report battery: <input type="checkbox" name="battery" ${battery} /></label></li>
            <li><label>&nbsp; incl. raw value: <input type="checkbox" name="battraw" ${battraw} /></label></li>
            <li><label>Report performance: <input type="checkbox" name="doperf" ${doperf} /></label></li>
            <li><label>&nbsp; incl. raw value: <input type="checkbox" name="perfraw" ${perfraw} /></label></li>
            <li><label>Measure every <input type="number" min="1" max="9999" name="deltat" value="${deltat}" /> seconds</label></li>
        </ul>
        <h2>Hardware</h2>
        <ul>
            <li><label>Pin for blue LED: <input type="number" min="0" max="16" name="pinblue" value="${pinblue}" /></label></li>
            <li><label>&nbsp; invert: <input type="checkbox" name="invblue" ${invblue} /></label></li>
            <li><label>Pin for config-mode: <input type="number" min="0" max="16" name="pinconfig" value="${pinconfig}" /></label></li>
            <li><label>Pin for sensor power: <input type="number" min="0" max="16" name="pinpwrsens" value="${pinpwrsens}" /></label></li>
            <li><label>Pin for Dallas data: <input type="number" min="0" max="16" name="pindallas" value="${pindallas}" /></label></li>
            <li><label>Pin for DHT data: <input type="number" min="0" max="16" name="pindhtdata" value="${pindhtdata}" /></label></li>
            <li><label>Number of ADC measurements: <input type="number" min="1" max="13" step="2" name="adcmeas" value="${adcmeas}" /></label></li>
            <li><label>Battery divider: <input type="number" min="0" max="1" step="0.0001" name="battdiv" value="${battdiv}" /></label></li>
            <li><label>NTC fixed R: <input type="number" name="ntcrfix" value="${ntcrfix}" /> Ohm</label></li>
            <li><label>NTC B: <input type="number" name="ntc_b" value="${ntc_b}" /></label></li>
            <li><label>NTC R0: <input type="number" name="ntc_r0" value="${ntc_r0}" /></label> Ohm</li>
        </ul>
        <input type="reset" value="Revert">
        <input type="submit" value="Apply">
        </form>
        <hr>
        <h2>Firmware Update</h2>
        Firmware needs to be compiled for ${flashsize} KiB flash size!<br />
        <form method="POST" action="/update" enctype="multipart/form-data">
            Firmware: <input type="file" name="update">
            <input type="submit" value="Update">
        </form>
        <hr>
        Current firmware was built on ${buildtime}
    </body>
</html>)";

/*
    END OF CONFIGURATION
 */

