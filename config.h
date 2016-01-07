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
const unsigned int portServer = 1883;
const unsigned int maxPacketSize = 1400;

// behaviour/timing
const byte sleepWifiCheck = 50;     // ms to sleep between checks if associated
const byte maxConnRetry = 200;      // how many sleepWifiCheck sleeps
const unsigned int noConnSleepSec = 600;
const unsigned int SLEEPSEC = 307; // not very accurate
const byte sleepEnd = 100;          // ms to sleep before going to deepSleep
const byte sleepADCmeasure = 100;   // ms to sleep before each ADC read
const byte sleepUDP = 50;           // ms to sleep after each UDP packet
const unsigned int sleepDHT = 2000; // ms to sleep before querying DHT
const byte maxSensors = 8;

// hardware
const byte PIN_BLUELED = 1;
const byte BLUELED_ON = LOW;  // onboard-LED is active-LOW
const byte ADC_MEASURES = 5;
const float Vdd = 3.3;
const float Voff = -0.01;   // highest V where ADC still reports "0"

// Sensors
const bool doNTC = true;
const bool doNTCraw = true;
const bool doDallas = true;
const bool doDHT = true;
const bool doDHTHI = true;
const bool doBattery = true;
const bool doBattraw = true;
const bool doPerf = true;
const bool doPerfraw = true;

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
const float battDivider = 10.0/78;    // 68k + 10k

/*
    END OF CONFIGURATION
 */

