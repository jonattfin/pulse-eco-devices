#include "Arduino.h"
#include <ESP8266WebServer.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "sensorsFacade.h"

// Documentation -> https://randomnerdtutorials.com/esp8266-and-node-red-with-mqtt/

#define PERSIST true

namespace helpers {
    class Util
    {
        public:
            static int countSplitCharacters(String* text, char splitChar);
            static int splitCommand(String* text, char splitChar, String returnValue[], int maxLen);
            static String getMacID();
    };

    class CustomClient
    {
        public:
            void init(String dbName, String dbPassword, String location);
            void publish(facade::SensorData data);

        private:
            String m_dbName;
            String m_dbPassword;
            String m_location;
    };
}
