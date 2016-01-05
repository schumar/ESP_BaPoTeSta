
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
    VOLT,
    USEC,
    CENT_PERC
};

const char* unitTypeName[] = {
    "centdegc",
    "percent",
    "raw",
    "volt",
    "usec",
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
void powerDallas(bool on);
void powerDHT(bool on);
