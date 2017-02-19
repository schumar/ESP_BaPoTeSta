
enum sensorType {
    TEMP,
    BATTERY,
    HUMIDITY,
    TIME,
    TEMPHI,
    PRESSURE,
    PRESSUREASL
};

const char* sensorTypeName[] = {
    "temp",
    "battery",
    "humidity",
    "time",
    "tempHI",
    "pressure",
    "pressureASL"
};

enum unitType {
    CENT_DEGC,
    PERCENT,
    RAW,
    MVOLT,
    USEC,
    CENT_PERC,
    PASCAL
};

const char* unitTypeName[] = {
    "centdegc",
    "percent",
    "raw",
    "millivolt",
    "microsec",
    "centpercent",
    "pascal"
};

struct sensorMeasurement {
    unsigned int sensorId;
    enum sensorType type;
    int value;
    enum unitType unit;
};

struct allMeasurements {
    unsigned long int chipId;
    unsigned int timestep;
    byte nrMeasurements;
    struct sensorMeasurement * sensorMeasurements;
};

struct config {
    byte cfgversion = 4;

    char ssid[32] = "tabr.org";
    char password[32] = "";
    IPAddress ip = {10, 1, 0, 38};
    byte netmask = 24;
    IPAddress gw = {10, 1, 0, 1};
    IPAddress mqttip = {10, 1, 0, 9};
    unsigned int mqttport = 1883;

    bool usedallas = true;
    byte dallasres = 12;
    float biasDallasTemp = 0.0;
    bool dallaswait = false;

    bool usedht = true;
    byte dhttype = DHT22;
    float biasDHTTemp = 0.0;
    float biasDHTHumid = 0.0;
    bool dhthi = true;

    bool battery = true;
    bool battraw = false;

    bool doperf = true;
    bool perfraw = false;

    bool usebmp280 = true;
    byte bmp280addr = 0x76;     // 0 or 0x77 for SDO=HIGH, 0x76 for SDO=LOW
    bool bmppress = true;       // calculate pressure at current height?
    bool bmpslp = true;         // calculate pressure at sea level?
    int16_t heightASL = 450;    // height above sea level (meter)

    unsigned int deltat = 300;

    int8_t pinblue = -1;
    bool invblue = true;
    byte pinconfig = 4;
    byte pinpwrsens = 13;
    byte pindallas = 5;
    byte pindhtdata = 2;
    byte pini2cscl = 14;
    byte pini2csda = 12;

    byte adcmeas = 5;
    float battdiv = 10.0/66;

};

void sendTemp(float temp);
int readADC();
void getBattery();
float calcBattery(int raw);
void bubbleSort(float * analogValues, int nr);
void bubbleSort(int * analogValues, int nr);
void gotoSleep(unsigned int seconds);
void collectData();
void getDallas();
void getDHT();
void getPerf();
void addData(unsigned int sensorId, enum sensorType type,
        int value, enum unitType unit);
void sendData();
void powerSensors(bool on);

void setupNormal();
void setupWebserver();
String ipToString (IPAddress ip);
IPAddress stringToIP (String text);
void webForm();
void webCSS();

void getConfig();
void storeConfig();

void debugPrint(String msg);
