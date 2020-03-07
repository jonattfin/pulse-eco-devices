#include <EEPROM.h>
#include <SoftwareSerial.h>

#include <helpers.h>

// #include <sensorsFacade.h>
using namespace facade;

#include <constants.h>

// No Connection counter
int noConnectionLoopCount = 0;

ESP8266WebServer server(80);

//EEPROM Data
String dbName = "";
String dbPassword = "";
String location = "";

String ssid = "";
String password = "";

int status = -1;

// TCP + TLS
IPAddress apIP(192, 168, 1, 1);

helpers::CustomClient customClient;

// facade::NullSensors sensors;
facade::Sensors sensors;

void discoverAndSetStatus()
{
  String data = "";
  bool validData = false;

  char readValue = (char)EEPROM.read(0);
  SH_DEBUG_PRINTLN("data is EPROM is " + readValue);
  if ((char)readValue == '[')
  {
    //we're good
    //read till you get to the end
    SH_DEBUG_PRINTLN("Found data in EEPROM");
    int nextIndex = 1;
    while (nextIndex < facade::EEPROM_SIZE && (readValue = (char)EEPROM.read(nextIndex++)) != ']')
    {
      data += readValue;
    }
  }

  if ((char)readValue == ']')
  {
    validData = true;
#ifdef DEBUG_PROFILE
    SH_DEBUG_PRINTLN("Read data:");
    SH_DEBUG_PRINTLN(data);
#endif
  }
  else
  {
    SH_DEBUG_PRINTLN("No data found in EEPROM");
  }

  String readFields[4];
  if (validData)
  {
    //Try to properly split the string
    //String format: SSID:password:mode

    int count = helpers::Util::splitCommand(&data, ':', readFields, 5);
    if (count != 5)
    {
      validData = false;
      SH_DEBUG_PRINTLN("Incorrect data format.");
    }
    else
    {
#ifdef DEBUG_PROFILE
      SH_DEBUG_PRINTLN("Read data parts:");
      SH_DEBUG_PRINTLN(readFields[0]);
      SH_DEBUG_PRINTLN(readFields[1]);
      SH_DEBUG_PRINTLN(readFields[2]);
      SH_DEBUG_PRINTLN(readFields[3]);
      SH_DEBUG_PRINTLN(readFields[4]);
#endif
      dbName = readFields[0];
      dbPassword = readFields[1];
      location = readFields[2];

      ssid = readFields[3];
      password = readFields[4];
      validData = true;
    }
  }

  if (ssid == NULL || ssid.equals(""))
  {
    SH_DEBUG_PRINTLN("No WiFi settings found.");
    //no network set yet
    validData = false;
  }

  if (!validData)
  {
    //It's still not connected to anything
    //broadcast net and display form
    SH_DEBUG_PRINTLN("Setting status code to 0: dipslay config options.");
    digitalWrite(STATUS_LED_PIN, LOW);
    status = 0;
  }
  else
  {
    status = 1;
    digitalWrite(STATUS_LED_PIN, HIGH);
    SH_DEBUG_PRINTLN("Initially setting status to 1: try to connect to the network.");
  }
}

//Web server params below
void handleRootGet()
{

  String output = "<!DOCTYPE html> \
    <html><head><title>Configure WiFi</title>";
  output += "<style>b ";
  output += "body { font-family: 'Verdana'} ";
  output += "div { font-size: 2em;} ";
  output += "input[type='text'] { font-size: 1.5em; width: 100%;} ";
  output += "input[type='submit'] { font-size: 1.5em; width: 100%;} ";
  output += "input[type='radio'] { display: none;} ";
  output += "input[type='radio'] { ";
  output += "   height: 2.5em; width: 2.5em; display: inline-block;cursor: pointer; ";
  output += "   vertical-align: middle; background: #FFF; border: 1px solid #d2d2d2; border-radius: 100%;} ";
  output += "input[type='radio'] { border-color: #c2c2c2;} ";
  output += "input[type='radio']:checked { background:gray;} ";
  output += "</style>";
  output += "</head> \
    <body> \
    <div style='text-align: center; font-weight: bold'>SkopjePulse node config</div> \
    <form method='post' action='/post'> \
    <div>Db name:</div> \
    <div><input type='text' name='dbName' /><br/><br/></div> \
    <div>Db password:</div> \
    <div><input type='text' name='dbPassword' /><br/><br/></div> \
    <div>Location:</div> \
    <div><input type='text' name='location' /><br/><br/></div> \
    <div>SSID:</div> \
    <div><input type='text' name='ssid' /><br/><br/></div> \
    <div>Password:</div> \
    <div><input type='text' name='password' /><br/><br/></div> \
    <div><input type='submit' /><div> \
    </form></body></html>";

  server.send(200, "text/html", output);
}

void handleRootPost()
{
  SH_DEBUG_PRINT("Number of args:");
  SH_DEBUG_PRINTLN(server.args());
  for (int i = 0; i < server.args(); i++)
  {
    SH_DEBUG_PRINT("Argument no.");
    SH_DEBUG_PRINT_DEC(i, DEC);
    SH_DEBUG_PRINT(": name: ");
    SH_DEBUG_PRINT(server.argName(i));
    SH_DEBUG_PRINT(" value: ");
    SH_DEBUG_PRINTLN(server.arg(i));
  }

  if (server.args() == 6 
    && server.argName(0).equals("dbName") 
    && server.argName(1).equals("dbPassword") 
    && server.argName(2).equals("location") 
    && server.argName(3).equals("ssid") 
    && server.argName(4).equals("password"))
  {
    //it's ok

    String data = "[" + server.arg(0) + ":" + server.arg(1) + ":" + server.arg(2) + ":" + server.arg(3) + ":" + server.arg(4) + "]";
    data.replace("+", " ");

    if (data.length() < facade::EEPROM_SIZE)
    {
      server.send(200, "text/html", "<h1>The device will restart now.</h1>");
      //It's ok

      SH_DEBUG_PRINTLN("Storing data in EEPROM:");
#ifdef DEBUG_PROFILE
      SH_DEBUG_PRINTLN(data);
#endif

      for (int i = 0; i < data.length(); i++)
      {
        EEPROM.write(i, (byte)data[i]);
      }

      EEPROM.commit();
      delay(500);

      SH_DEBUG_PRINTLN("Stored to EEPROM. Restarting.");
      ESP.restart();
    }
    else
    {
      server.send(200, "text/html", "<h1>The parameter string is too long.</h1>");
    }
  }
  else
  {
    server.send(200, "text/html", "<h1>Incorrect input. Please try again.</h1>");
  }
}

// the setup routine runs once when you press reset:
void setup()
{

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  SH_DEBUG_PRINTLN("Startup...");

  EEPROM.begin(facade::EEPROM_SIZE);

#ifndef NO_CONNECTION_PROFILE
  discoverAndSetStatus();

  if (status == 1)
  {
    //Try to connect to the network
    SH_DEBUG_PRINTLN("Trying to connect...");
    char ssidBuf[ssid.length() + 1];
    ssid.toCharArray(ssidBuf, ssid.length() + 1);
    char passBuf[password.length() + 1];
    password.toCharArray(passBuf, password.length() + 1);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidBuf, passBuf);
    SH_DEBUG_PRINT("SSID: ");
    SH_DEBUG_PRINTLN(ssidBuf);
#ifdef DEBUG_PROFILE
    SH_DEBUG_PRINT("Password: ");
    SH_DEBUG_PRINTLN(passBuf);
#endif

    // Wait for connection
    boolean toggleLed = false;
    int numTries = 200;
    while (WiFi.status() != WL_CONNECTED && --numTries > 0)
    {
      delay(250);
      SH_DEBUG_PRINT(".");
      toggleLed = !toggleLed;
      digitalWrite(STATUS_LED_PIN, toggleLed);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      SH_DEBUG_PRINT("Unable to connect to the network: ");
      SH_DEBUG_PRINTLN(ssid);
      status = 0;
      digitalWrite(STATUS_LED_PIN, LOW);
    }
    else
    {

      // Connected to the network

      SH_DEBUG_PRINT("Connected to:");
      SH_DEBUG_PRINTLN(ssid);
      SH_DEBUG_PRINT("IP address: ");
      SH_DEBUG_PRINTLN(WiFi.localIP());
      digitalWrite(STATUS_LED_PIN, HIGH);

      // init the sensors
      sensors.init();
      customClient.init(dbName, dbPassword, "room1"); // TODO
    }
  }

  if (status != 1)
  {
    //Input params
    //Start up the web server

    SH_DEBUG_PRINTLN("Setting up configuration web server");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    String AP_NameString = "PulseEcoSensor-" + helpers::Util::getMacID();

    char AP_NameChar[AP_NameString.length() + 1];
    memset(AP_NameChar, 0, AP_NameString.length() + 1);

    for (int i = 0; i < AP_NameString.length(); i++)
      AP_NameChar[i] = AP_NameString.charAt(i);

    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_NameChar);
    delay(500);
    server.on("/", HTTP_GET, handleRootGet);
    server.on("/post", HTTP_POST, handleRootPost);
    server.onNotFound(handleRootGet);
    server.begin();
    SH_DEBUG_PRINTLN("HTTP server started");
    SH_DEBUG_PRINT("AP IP address: ");
    SH_DEBUG_PRINTLN(apIP);
  }
#else
  status = 1;
#endif

  //wait a bit before your start
  delay(2000);
}

void sendMeasurements(facade::SensorData data)
{
  Serial.println("Send measurements");
  customClient.publish(data);
}

void loop()
{
  if (status == 1)
  {
    // wait
    delay(CYCLE_DELAY);

    // read & send sensor data
    facade::SensorData measurements = sensors.readMeasurements();
    if (measurements.hasAnyData())
    {
      sendMeasurements(measurements);
    }
    else
    {
      Serial.println("No data...");
    }
  }
  else
  {
    server.handleClient();
    delay(50);

    noConnectionLoopCount++;

    // second 20 cycles
    // 1 minute 60 * 20 = 1200 cycles
    // 1 minute 60 * 20 = 1200 cycles
    // 10 minutes 10 * 1200 = 12000  cycles
    if (noConnectionLoopCount >= 12000)
    {
      //Reboot after 10 minutes in setup mode. Might be a temp failure in the network
      ESP.restart();
    }
  }
}
