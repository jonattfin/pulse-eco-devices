#include "Arduino.h"
#include <ESP8266WebServer.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>

// Documentation -> https://randomnerdtutorials.com/esp8266-and-node-red-with-mqtt/

#define BBT "mqtt.beebotte.com"     // Domain name of Beebotte MQTT service

#define PERSIST true

namespace helpers {
    class Util
    {
        public:
            static int countSplitCharacters(String* text, char splitChar);
            static int splitCommand(String* text, char splitChar, String returnValue[], int maxLen);
            static String getMacID();
    };

    class MQTTClient
    {
        private:
            String channelName;
            String token;

        public:
            void init(String channelName, String token);
            void connect();
            void reconnect();
            void publish(const char* resource, float data);
    };
}
