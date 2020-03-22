#include "helpers.h"
#include <SoftwareSerial.h>
#include <ESP8266HTTPClient.h>

using namespace helpers;

HTTPClient http;

// Initializes the espClient.
WiFiClient espClient;
PubSubClient client(espClient);

int Util::countSplitCharacters(String *text, char splitChar)
{
  int returnValue = 0;
  int index = -1;

  while (true)
  {
    index = text->indexOf(splitChar, index + 1);

    if (index > -1)
    {
      returnValue += 1;
    }
    else
    {
      break;
    }
  }

  return returnValue;
}

int Util::splitCommand(String *text, char splitChar, String returnValue[], int maxLen)
{
  int splitCount = Util::countSplitCharacters(text, splitChar);
  if (splitCount + 1 > maxLen)
  {
    return -1;
  }

  int index = -1;
  int index2;

  for (int i = 0; i <= splitCount; i++)
  {
    //    index = text->indexOf(splitChar, index + 1);
    index2 = text->indexOf(splitChar, index + 1);

    if (index2 < 0)
      index2 = text->length();
    returnValue[i] = text->substring(index + 1, index2);
    index = index2;
  }

  return splitCount + 1;
}

String Util::getMacID()
{
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();

  return macID;
}

void helpers::CustomClient::init(String dbName, String dbPassword, String location)
{
  m_dbName = dbName;
  m_dbPassword = dbPassword;
  m_location = location;
}

void helpers::CustomClient::publish(facade::SensorData data)
{
  String payloadStr = "sensors_data,location=" + m_location + " temperature=" + data.temperature + ",pressure=" + data.pressure + ",humidity=" + data.humidity + ",noise=" + data.noise + ",gasResistance=" + data.gasResistance;

  if (data.pm10 > 0) {
    String s1 = ",pm10=" + String(data.pm10);
    payloadStr += s1;
  }

  if (data.pm25 > 0) {
    String s1 = ",pm25=" + String(data.pm25);
    payloadStr += s1;
  }

  Serial.println("payloadStr: " + payloadStr);

  String corlysisUrl = "http://corlysis.com:8087/write?db=" + m_dbName + "&u=token&p=" + m_dbPassword;
  Serial.println("corlysisUrl: " + corlysisUrl);

  http.begin(corlysisUrl);
  //HTTPS variant - check ssh public key fingerprint
  //sprintf(corlysisUrl, "https://corlysis.com:8086/write?db=%s&u=token&p=%s", dbName, dbPassword);

  //http.begin(corlysisUrl, "92:23:13:0D:59:68:58:83:E6:82:98:EB:18:D7:68:B5:C8:90:0D:03");

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(payloadStr);
  Serial.print("http result:");
  Serial.println(httpCode);
  http.writeToStream(&Serial);
  http.end();

  if (httpCode == 204)
  {
    Serial.println("Data successfully sent.");
  }
  else
  {
    Serial.println("Data were not sent. Check network connection.");
  }
}