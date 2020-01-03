#include "helpers.h"
#include <SoftwareSerial.h>

using namespace helpers;

// Initializes the espClient.
WiFiClient espClient;
PubSubClient client(espClient);


int  Util::countSplitCharacters(String* text, char splitChar) {
    int returnValue = 0;
    int index = -1;

    while (true) {
        index = text->indexOf(splitChar, index + 1);

        if(index > -1) {
            returnValue+=1;
        } else {
            break;
        }
    }

    return returnValue;
}


int  Util::splitCommand(String* text, char splitChar, String returnValue[], int maxLen) {
    int splitCount = Util::countSplitCharacters(text, splitChar);
    if (splitCount + 1 > maxLen) {
        return -1;
    }

    int index = -1;
    int index2;

    for(int i = 0; i <= splitCount; i++) {
    //    index = text->indexOf(splitChar, index + 1);
        index2 = text->indexOf(splitChar, index + 1);

        if(index2 < 0) index2 = text->length();
        returnValue[i] = text->substring(index+1, index2);
        index = index2;
    }

    return splitCount + 1;
}

String Util::getMacID() {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();

  return macID;
}


void MQTTClient::init(String channel, String tkn) {
  // set the MQTT server
  client.setServer(BBT, 1883);

  channelName = channel;
  token = tkn;

  Serial.println("channelName, token=" + channelName + " " + token);
}

void MQTTClient::connect() {
    if (!client.connected()) {
        reconnect();
    }

    if(!client.loop())
        client.connect(helpers::Util::getMacID().c_str());
}


void MQTTClient::reconnect() {
     // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection... with token " + token);
    
    if (client.connect(helpers::Util::getMacID().c_str(), token.c_str(), "")) {
      Serial.println("connected");  

      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics
    
    } else {
      Serial.println("failed, rc=");
      Serial.println(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void MQTTClient::publish(const char* resource, float data) {
    if (data == INT_MIN) {
    return;
  }

  DynamicJsonDocument doc(128);
  doc["channel"] = channelName;
  doc["resource"] = resource;
 
  if (PERSIST) {
    doc["write"] = true;
  }
  doc["data"] = data;

  // Now print the JSON into a char buffer
  char buffer[128];
  serializeJson(doc, buffer);

  Serial.println("channel name is" + channelName);

  // Create the topic to publish to
  char topic[64];
  sprintf(topic, "%s/%s", channelName.c_str(), resource);

  // Now publish the char buffer to Beebotte
  client.publish(topic, buffer);
}