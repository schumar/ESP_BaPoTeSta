
enum sensorType {
    TEMP,
    BATTERY,
    HUMIDITY
};

enum unitType {
    CENT_DEGC,
    PERCENT,
    RAW
};

struct sensorMeasurement {
    unsigned int sensorId;
    enum sensorType type;
    float value;
    enum unitType unit;
};

struct allMeasurements {
    unsigned long int chipId;
    unsigned int timestep;
    byte nrMeasurements;
    struct sensorMeasurement * sensorMeasurements;
};

void sendTemp(float temp);
float calcTemp(unsigned int raw);
void bubbleSort(float * analogValues);
void gotoSleep(unsigned int seconds);
void collectData();
void getNTC();
void addData(unsigned int sensorId, enum sensorType type,
        float value, enum unitType unit);
void sendData();
void powerSensors(bool on);
void powerNTC(bool on);
