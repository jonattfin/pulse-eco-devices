#include "Arduino.h"
#include <SoftwareSerial.h>

namespace facade {
    const int EEPROM_SIZE = 256;

    struct SensorData
    {
        public:
            int pm10 = -1;
            int pm25 = -1;
            int noise = -1;
            int temperature = -1;
            int humidity = -1;

            int pressure = -1;
            int altitude = -1;
            int gasResistance = -1;

            bool hasAnyData();
            void printMeasurements();
    };

    class Sensors
    {
        public:
            void init();
            SensorData readMeasurements();
    };

    class NullSensors
    {
        public:
            void init();
            SensorData readMeasurements();
    };

}
