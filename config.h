/*
    CONFIGURATION
 */

// behaviour/timing
const byte sleepWifiCheck = 50;     // ms to sleep between checks if associated
const byte maxConnRetry = 200;      // how many sleepWifiCheck sleeps
const unsigned int noConnSleepSec = 600;
const unsigned int lowBattSleepSec = 600;
const byte sleepEnd = 100;          // ms to sleep before going to deepSleep
const byte sleepADCmeasure = 100;   // ms to sleep before each ADC read
const unsigned int sleepDHT = 2000; // ms to sleep before querying DHT
const byte maxSensors = 8;

// #define SERIALDEBUG

// hardware
const float Vdd = 3.3;
const float Voff = 0.00;        // highest V where ADC still reports "0"
const float VLowBat = 3.0;      // don't take/send measurements if Vbat < this

// NTC
const unsigned int NTC_ID = 42; // NTC doesn't have a special "id" -> use 42

// Dallas
/* Each DS18B20 has its own ID, but as the rest of the software can't handle
 * more than 1 Dallas (yet), we can just stick to a hardcoded Id. Set to 0
 * to use the actual ID */
const unsigned int DALLAS_ID = 0x1820;

// Webserver
const char indexPage[] =
R"(<!DOCTYPE html>
<html>
    <head>
        <title>ESP_BaPoTeSta Maintenance</title>
        <link href="style.css" rel="stylesheet" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
    </head>
    <body>
        <h1>ESP_BaPoTeSta Maintenance</h1>
        <h2>Configuration</h2>
        <form method="POST" action="/config" enctype="multipart/form-data"  class="pure-form pure-form-aligned">
            <fieldset>
                <legend>Network</legend>
                <div class="inputgroup">
                    <label for="ssid">ssid</label><input type="text" id="ssid" name="ssid" value="${ssid}" />
                    <label for="password">password</label><input type="text" id="password" name="password" placeholder="0 or 8+ chars" value="${password}" />
                    <label for="ip">IP</label><input type="text" id="ip" name="ip" value="${ip}" pattern="\d\d?\d?.\d\d?\d?.\d\d?\d?.\d\d?\d?" />
                    <label for="netmask">Netmask</label><input type="number" min="0" max="31" id="netmask" name="netmask" value="${netmask}" />
                    <label for="gw">Gateway</label><input type="text" pattern="\d\d?\d?.\d\d?\d?.\d\d?\d?.\d\d?\d?" id="gw" name="gw" value="${gw}" />
                </div>
                <div class="inputgroup">
                    <label for="mqttip">MQTT IP</label><input type="text" pattern="\d\d?\d?.\d\d?\d?.\d\d?\d?.\d\d?\d?" id="mqttip" name="mqttip" value="${mqttip}" />
                    <label for="mqttport">MQTT Port</label><input type="number" min="1" max="65535" id="mqttport" name="mqttport" value="${mqttport}" />
                </div>
            </fieldset>
            <fieldset>
                <legend>Measuring</legend>
                <div class="inputgroup">
                    <label for="usedallas">Use DS18B20</label><input type="checkbox" id="usedallas" name="usedallas" ${usedallas} />
                    <label for="dallasres"> resolution</label><input type="number" min="9" max="12" id="dallasres" name="dallasres" value="${dallasres}" />
                    <label for="biasdallastemp"> bias</label><input type="number" min="-1.0" max="1.0" step="0.01" id="biasdallastemp" name="biasdallastemp" value="${biasdallastemp}" />
                    <label for="dallaswait"> check for result</label><input type="checkbox" id="dallaswait" name="dallaswait" ${dallaswait} />
                </div>
                <div class="inputgroup">
                    <label for="usedht">Use DHT</label><input type="checkbox" id="usedht" name="usedht" ${usedht} />
                    <label for="dhttype"> type</label><input type="number" min="11" max="33" id="dhttype" name="dhttype" value="${dhttype}" />
                    <label for="biasdhttemp"> temp bias</label><input type="number" min="-1.0" max="1.0" step="0.01" id="biasdhttemp" name="biasdhttemp" value="${biasdhttemp}" />
                    <label for="biasdhthumid"> hum bias</label><input type="number" min="-10" max="10" step="0.01" id="biasdhthumid" name="biasdhthumid" value="${biasdhthumid}" />
                    <label for="dhthi"> report HI</label><input type="checkbox" id="dhthi" name="dhthi" ${dhthi} />
                </div>
                <div class="inputgroup">
                    <label for="usentc">Use NTC</label><input type="checkbox" id="usentc" name="usentc" ${usentc} />
                    <label for="ntcraw"> incl. raw value</label><input type="checkbox" id="ntcraw" name="ntcraw" ${ntcraw} />
                </div>
                <div class="inputgroup">
                    <label for="battery">Report battery</label><input type="checkbox" id="battery" name="battery" ${battery} />
                    <label for="battraw"> incl. raw value</label><input type="checkbox" id="battraw" name="battraw" ${battraw} />
                </div>
                <div class="inputgroup">
                    <label for="doperf">Report performance</label><input type="checkbox" id="doperf" name="doperf" ${doperf} />
                    <label for="perfraw"> incl. raw value</label><input type="checkbox" id="perfraw" name="perfraw" ${perfraw} />
                </div>
                <div class="inputgroup">
                    <label for="deltat">Period (secs)</label><input type="number" min="1" max="9999" id="deltat" name="deltat" value="${deltat}" />
                </div>
            </fieldset>
            <fieldset>
                <legend>Hardware</legend>
                <div class="inputgroup">
                    <label for="pinblue">Pin for blue LED</label><input type="number" min="-1" max="16" id="pinblue" name="pinblue" value="${pinblue}" />
                    <label for="invblue"> invert</label><input type="checkbox" id="invblue" name="invblue" ${invblue} />
                    <label for="pinconfig">Pin for config-mode</label><input type="number" min="0" max="16" id="pinconfig" name="pinconfig" value="${pinconfig}" />
                    <label for="pinpwrsens">Pin for sensor power</label><input type="number" min="0" max="16" id="pinpwrsens" name="pinpwrsens" value="${pinpwrsens}" />
                    <label for="pindallas">Pin for Dallas data</label><input type="number" min="0" max="16" id="pindallas" name="pindallas" value="${pindallas}" />
                    <label for="pindhtdata">Pin for DHT data</label><input type="number" min="0" max="16" id="pindhtdata" name="pindhtdata" value="${pindhtdata}" />
                </div>
                <div class="inputgroup">
                    <label for="adcmeas">Number of ADC measurements</label><input type="number" min="1" max="13" step="2" id="adcmeas" name="adcmeas" value="${adcmeas}" />
                    <label for="battdiv">Battery divider</label><input type="number" min="0" max="1" step="0.0001" id="battdiv" name="battdiv" value="${battdiv}" />
                    <label for="ntcrfix">NTC fixed R (&Omega;)</label><input type="number" id="ntcrfix" name="ntcrfix" value="${ntcrfix}" />
                    <label for="ntc_b">NTC B</label><input type="number" id="ntc_b" name="ntc_b" value="${ntc_b}" />
                    <label for="ntc_r0">NTC R0 (&Omega;)</label><input type="number" id="ntc_r0" name="ntc_r0" value="${ntc_r0}" />
                </div>
            </fieldset>

            <div style="clear:left">
                <button type="reset">Revert</button>
                <button type="submit" class="primary">Apply</button>
            </div>
        </form>

        <h2>Firmware Update</h2>
        <form method="POST" action="/update" enctype="multipart/form-data">
            <label for="update">Firmware:</label><input type="file" id="update" name="update" />
            <button type="submit" class="primary">Update</button>
        </form>
        <p>Firmware needs to be compiled for ${flashsize} KiB flash size!</p>

        <h2>Info</h2>
        <p>Current firmware was built on ${buildtime}</p>
    </body>
</html>)";

const PROGMEM char css[] =
R"(
/*! Pure v0.6.0 || Copyright 2014 Yahoo! Inc. All rights reserved. || Licensed under the BSD License. || https://github.com/yahoo/pure/blob/master/LICENSE.md */
html{font-family:sans-serif;-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}body{margin:0}article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary{display:block}audio,canvas,progress,video{display:inline-block;vertical-align:baseline}audio:not([controls]){display:none;height:0}[hidden],template{display:none}a{background-color:transparent}a:active,a:hover{outline:0}abbr[title]{border-bottom:1px dotted}b,strong{font-weight:700}dfn{font-style:italic}h1{font-size:2em;margin:.67em 0}mark{background:#ff0;color:#000}small{font-size:80%}sub,sup{font-size:75%;line-height:0;position:relative;vertical-align:baseline}sup{top:-.5em}sub{bottom:-.25em}img{border:0}svg:not(:root){overflow:hidden}figure{margin:1em 40px}hr{-moz-box-sizing:content-box;box-sizing:content-box;height:0}pre{overflow:auto}code,kbd,pre,samp{font-family:monospace,monospace;font-size:1em}button,input,optgroup,select,textarea{color:inherit;font:inherit;margin:0}button{overflow:visible}button,select{text-transform:none}button,html input[type=button],input[type=reset],input[type=submit]{-webkit-appearance:button;cursor:pointer}button[disabled],html input[disabled]{cursor:default}button::-moz-focus-inner,input::-moz-focus-inner{border:0;padding:0}input{line-height:normal}input[type=checkbox],input[type=radio]{box-sizing:border-box;padding:0}input[type=number]::-webkit-inner-spin-button,input[type=number]::-webkit-outer-spin-button{height:auto}input[type=search]{-webkit-appearance:textfield;-moz-box-sizing:content-box;-webkit-box-sizing:content-box;box-sizing:content-box}input[type=search]::-webkit-search-cancel-button,input[type=search]::-webkit-search-decoration{-webkit-appearance:none}fieldset{border:1px solid silver;margin:0 2px;padding:.35em .625em .75em}legend{border:0;padding:0}textarea{overflow:auto}optgroup{font-weight:700}table{border-collapse:collapse;border-spacing:0}td,th{padding:0}.hidden,[hidden]{display:none!important}.pure-img{max-width:100%;height:auto;display:block}

html {
    margin: 0em 1em;
}
h2 {
    border-top:1px solid;
    padding-top:0.5em;
}
fieldset {
    border:none;
    width: 23em;
    float: left;
    margin: 0em 1em;
}
legend {
    border-bottom: 1px solid #e5e5e5;
    color: #333;
    display: block;
    margin-bottom: 0.3em;
    padding: 0.3em 0;
    width: 100%;
}
label {
    display: inline-block;
    margin: 0 1em 0 0;
    text-align: right;
    vertical-align: middle;
    width: 12em;
}
input {
    border: 1px solid #ccc;
    border-radius: 4px;
    box-shadow: 0 1px 3px #ddd inset;
    box-sizing: border-box;
    display: inline-block;
    padding: 0.5em 0.6em;
    vertical-align: middle;
    width: 10em;
}
input[type="number"] {
    width:7em;
}
input[type="checkbox"] {
    margin: 0.7em 0.5em;
    width: 1em;
}
input[type="file"] {
    width: 30em;
}
button {
    -moz-user-select: none;
    background-color: #e6e6e6;
    border-radius: 2px;
    border: 0 none rgba(0, 0, 0, 0);
    box-sizing: border-box;
    color: rgba(0, 0, 0, 0.8);
    cursor: pointer;
    display: inline-block;
    padding: 0.5em 1em;
    text-align: center;
    text-decoration: none;
    vertical-align: middle;
    white-space: nowrap;
}
button.primary {
    background-color: #0078e7;
    color: #fff;
}
div.inputgroup {
    margin:0.5em 0em;
}
)"
;

/*
    END OF CONFIGURATION
 */

