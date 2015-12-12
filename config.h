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
const unsigned int maxPacketSize = 1400;
// behaviour/timing
const byte maxConnRetry = 200;   // in 50ms units!
const unsigned int noConnSleepSec = 600;
const unsigned int SLEEPSEC = 307; // not very accurate
const byte maxSensors = 8;
// hardware
const byte PIN_BLUELED = 1;
const byte BLUELED_ON = LOW;  // onboard-LED is active-LOW

// Sensors
const bool doNTC = true;
const bool doNTCraw = true;
const bool doDallas = true;

// NTC
const byte PIN_NTC = 5;
const unsigned int NTC_ID = 42;
const byte NTC_MEASURES = 5;
// temp calculation
const float Vdd = 3.3;      // voltage of PIN_NTC when HIGH
const float Voff = -0.01;   // highest V where ADC still reports "0"
const float Rfix = 4.7e3;   // pulldown
const float NTC_B = 3950;
const float NTC_R0 = 20e3;
const float Rinf = NTC_R0*exp(-NTC_B/298.15);  // (T0 = 25 + 273.15 = 298.15)

// Dallas
const byte PIN_DALLAS_POWER = 14;
const byte PIN_1WIRE = 13;
const byte DALLAS_RESOLUTION = 12;

/*
    END OF CONFIGURATION
 */

