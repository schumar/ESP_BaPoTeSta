
enum sensorType {
    TEMP,
    BATTERY,
    HUMIDITY,
    TIME,
    TEMPHI
};

const char* sensorTypeName[] = {
    "temp",
    "battery",
    "humidity",
    "time",
    "tempHI"
};

enum unitType {
    CENT_DEGC,
    PERCENT,
    RAW,
    MVOLT,
    USEC,
    CENT_PERC
};

const char* unitTypeName[] = {
    "centdegc",
    "percent",
    "raw",
    "millivolt",
    "microsec",
    "centpercent"
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
    byte cfgversion = 1;

    char ssid[32] = "tabr.org";
    char password[32] = "";
    IPAddress ip = {10, 1, 0, 38};
    byte netmask = 24;
    IPAddress gw = {10, 1, 0, 1};
    IPAddress mqttip = {10, 1, 0, 9};
    unsigned int mqttport = 1883;

    bool usedallas = true;
    byte dallasres = 12;
    bool dallaswait = false;
    bool usedht = true;
    byte dhttype = DHT22;
    bool dhthi = true;
    bool battery = true;
    bool battraw = false;
    bool doperf = true;
    bool perfraw = false;

    unsigned int deltat = 300;

    int8_t pinblue = 1;
    bool invblue = true;
    byte pinconfig = 5;
    byte pinpwrsens = 14;
    byte pindallas = 13;
    byte pindhtdata = 12;

    byte adcmeas = 5;
    float battdiv = 10.0/66;
    float ntcrfix = 4.7e3;
    float ntc_b = 3950;
    float ntc_r0 = 20e3;

};

void sendTemp(float temp);
float calcNTCTemp(unsigned int raw);
int readADC();
void getBattery();
float calcBattery(int raw);
void bubbleSort(float * analogValues, int nr);
void bubbleSort(int * analogValues, int nr);
void gotoSleep(unsigned int seconds);
void collectData();
void getNTC();
void getDallas();
void getDHT();
void getPerf();
void addData(unsigned int sensorId, enum sensorType type,
        int value, enum unitType unit);
void sendData();
void powerSensors(bool on);
void powerNTC(bool on);
void powerSensorss(bool on);

void setupNormal();
void setupWebserver();
String ipToString (IPAddress ip);
void webForm();

void getConfig();

void debugPrint(const char * msg);
