#include "Arduino.h"
#include <SoftwareSerial.h>

namespace facade {
    const int EEPROM_SIZE = 256;

    struct SensorData
    {
        public:
            int pm10 = INT_MIN;
            int pm25 = INT_MIN;
            int noise = INT_MIN;
            int temperature = INT_MIN;
            int humidity = INT_MIN;

            int pressure = INT_MIN;
            int altitude = INT_MIN;
            int gasResistance = INT_MIN;

            bool hasAnyData();
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
