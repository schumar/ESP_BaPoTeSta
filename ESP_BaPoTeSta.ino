/*
   Battery-Powered WiFi Temperature Station
   Send temperature data via UDP

   Martin Schuster 2015

ToDo:
    look for other power-saving measures
        switch off UART
        put all unused pins into INPUT/OUTPUT/?
    add "timestamp" to sent packet
        read time + counter from eprom
        if 0, or counter > 100(?)
            retrieve time via NTP
            reset counter
            write new time to eprom
        when sending data, use time + counter * (SLEEPSEC + c)
            c needs to be calibrated (few secs)
        write counter to eprom
    CoAP
        use Discovery to find URI?
            store URI and expiration-time in eprom
            if URI not yet known, or expired
                send out Discovery paket to multicast
                on reply, store URI and time+maxvalid to eprom
        Multicast: 224.0.1.187

Pins:
    CH_PD PullUp  GPIO 15 PullDn  GPIO 2 PullUp
    Normal: GPIO 0 PullUp
    Upload: GPIO 0 GND
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


/*
    CONFIGURATION
 */
// network
const char ssid[] = "tabr.org";
const char pass[] = "";
const IPAddress IPLocal(10, 1, 0, 35);
const IPAddress IPGateway(10, 1, 0, 1);
const IPAddress IPSubnet(255, 255, 255, 0);
const IPAddress IPServer(10, 1, 0, 9);
const unsigned int portServer = 9988;
const byte maxConnRetry = 200;   // in 50ms units!
const unsigned int noConnSleepSec = 120;
// hardware
const byte PIN_BLUELED = 1;
const byte PIN_PTC = 5;
// behaviour
const byte MEASURES = 5;
const byte SLEEPSEC = 20;
// temp calculation
const float Vdd = 3.3;      // voltage of PIN_PTC when HIGH
const float Voff = 0.05;    // highest V where ADC still reports "0"
const float Rfix = 4.7e3;   // pulldown
/*
    END OF CONFIGURATION
 */


WiFiUDP Udp;
float sensorValue[MEASURES];
unsigned long int chipId;


void setup() {
    // activate (active low) blue LED to show that we are "on"
    pinMode(PIN_BLUELED, OUTPUT);
    digitalWrite(PIN_BLUELED, LOW);

    // start WiFi
    WiFi.mode(WIFI_STA);
    WiFi.config(IPLocal, IPGateway, IPSubnet);
    if (pass == NULL || pass[0] == 0) {
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, pass);
    }

    // wait until WiFi is connected, but max maxConnRetry
    for (byte i = 0;
            i < maxConnRetry && WiFi.status() != WL_CONNECTED;
            i++)
        delay(50);
    // if this didn't work, go back to sleep
    if (WiFi.status() != WL_CONNECTED)
        ESP.deepSleep(1e6 * noConnSleepSec, WAKE_NO_RFCAL);

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
        sensorValue[c] = calcTemp(analogRead(A0));
    }

    // switch off PTC
    digitalWrite(PIN_PTC, LOW);

    sendTemp(sensorValue);

    // switch off (active low) blue LED to show that we are "off"
    digitalWrite(PIN_BLUELED, HIGH);

    // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED
    ESP.deepSleep(1e6 * SLEEPSEC, WAKE_NO_RFCAL);
}


void sendTemp(float* temp) {
    const byte PACKET_SIZE = 11 + 6*MEASURES + 1;
    static char packetBuffer[PACKET_SIZE];

    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, PACKET_SIZE);

    // start data with ChipID (so multiple stations can use the same server)
    sprintf(packetBuffer, "0x%08x ", chipId);

    // add measured temperature values
    for (byte c=0; c<MEASURES; c++) {
        sprintf(11+packetBuffer+6*c, "%+04.1f ", temp[c]);
    }
    // the buffer now ends with SPC NUL -- change SPC to Newline
    packetBuffer[PACKET_SIZE - 2] = '\n';

    // send packet (twice, to make sure)
    for (byte i=0; i<2; i++) {
        Udp.beginPacket(IPServer, portServer);
        Udp.write(packetBuffer, PACKET_SIZE);
        Udp.endPacket();
    }
}

float calcTemp(unsigned int raw) {
    /*
       V = Vdd * Rfix / (Rfix + PTC)
       raw/1024 = V - Voff

       PTC = Vdd*Rfix / V - Rfix
           = Vdd*Rfix / (raw/1024 + Voff) - Rfix
     */
    float ptc = Vdd * Rfix / (raw/1024. + Voff) - Rfix;
    float temp = 0.001 * ptc - 10;  // [XXX] just a test!!
    return temp;
}


// vim: sw=4:expandtab:ts=4
