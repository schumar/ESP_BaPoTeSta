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
    output decimal
        "23.45" instead of "2345", by simple string manipulation

Pins:
    CH_PD PullUp  GPIO 15 PullDn  GPIO 2 PullUp
    Normal: GPIO 0 PullUp
    Upload: GPIO 0 GND
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <math.h>


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
const byte BLUELED_ON = LOW;  // onboard-LED is active-LOW
const byte PIN_NTC = 5;
// behaviour
const byte MEASURES = 5;
const byte SLEEPSEC = 20;
// temp calculation
const float Vdd = 3.3;      // voltage of PIN_NTC when HIGH
const float Voff = -0.01;   // highest V where ADC still reports "0"
const float Rfix = 4.7e3;   // pulldown
const float NTC_B = 3950;
const float NTC_R0 = 20e3;
/*
    END OF CONFIGURATION
 */

void sendTemp(float temp);
float calcTemp(unsigned int raw);
void bubbleSort(float * analogValues);
void gotoSleep(unsigned int seconds);

WiFiUDP Udp;
float sensorValue[MEASURES];
// unsigned long int chipId;

const float Rinf = NTC_R0*exp(-NTC_B/298.15);  // (T0 = 25 + 273.15 = 298.15)

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

    // wait until WiFi is connected, but max maxConnRetry
    for (byte i = 0;
            i < maxConnRetry && WiFi.status() != WL_CONNECTED;
            i++)
        delay(50);
    // if this didn't work, go back to sleep
    if (WiFi.status() != WL_CONNECTED)
        gotoSleep(noConnSleepSec);

    // get ChipID, will be used as unique ID when sending data
    // chipId = ESP.getChipId();
}


// this won't actually "loop", as the last command leads to a reset
void loop() {

    // activate power to NTC
    pinMode(PIN_NTC, OUTPUT);
    digitalWrite(PIN_NTC, HIGH);

    // measure temp multiple times
    for (byte c=0; c<MEASURES; c++) {
        delay(100);
        sensorValue[c] = calcTemp(analogRead(A0));
    }

    // switch off NTC
    digitalWrite(PIN_NTC, LOW);

    bubbleSort(sensorValue);

    sendTemp(sensorValue[MEASURES/2]);

    // wait a little bit, to ensure that everything is sent
    delay(100);

    gotoSleep(SLEEPSEC);
}


void sendTemp(float temp) {
    const byte PACKET_SIZE = 6 + 1;
    static char packetBuffer[PACKET_SIZE];

    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, PACKET_SIZE);

    // start data with ChipID (so multiple stations can use the same server)
    // sprintf(packetBuffer, "0x%08x ", chipId);

    // add measured temperature values
    sprintf(packetBuffer, "%+05d ", (int)(temp*100));

    // the buffer now ends with SPC NUL -- change SPC to Newline
    packetBuffer[PACKET_SIZE - 2] = '\n';

    // send packet (twice, to make sure)
    for (byte i=0; i<2; i++) {
        Udp.beginPacket(IPServer, portServer);
        Udp.write(packetBuffer, PACKET_SIZE);
        Udp.endPacket();
        delay(50);
    }
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

void bubbleSort(float * analogValues) {
    int out, in, swapper;
    for(out=0 ; out < MEASURES; out++) {  // outer loop
        for(in=out; in<(MEASURES-1); in++)  {  // inner loop
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
