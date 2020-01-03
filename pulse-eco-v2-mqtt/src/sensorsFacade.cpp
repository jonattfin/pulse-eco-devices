#include <SoftwareSerial.h>
#include <EEPROM.h>

// Nova SDS definitions
#include "Sds011.h"

// Adafruit General definitions
#include <Adafruit_Sensor.h>

// Adafruit Sensor definitions
#include <bme680.h>
#include <Adafruit_BME680.h>
#include <bme680_defs.h>
#include <Adafruit_BME280.h>

#include "constants.h"

#include "sensorsFacade.h"
using namespace facade;

#define SEALEVELPRESSURE_HPA (1013.25)

//Sharp Dust Sensor Pins
#define SDS_RX_PIN D5
#define SDS_TX_PIN D6

//Noise sensor pins
#define NOISE_MEASURE_PIN A0
#define NUM_NOISE_SAMPLES 1200
 
//Init global objects
Adafruit_BME680 bme680; // I2C
Adafruit_BME280 bme280; // I2C

SoftwareSerial sdsSerial(SDS_RX_PIN, SDS_TX_PIN); // RX, TX
sds011::Sds011 sdsSensor(sdsSerial);

//Flags

bool hasBME680 = false;
bool hasBME280 = false;

int loopCycleCount = 0;
int noiseTotal = 0;
int pm10 = 0;
int pm25 = 0;

void Sensors::init() {

    // Init the temp/hum sensor
    // Set up oversampling and filter initialization

    hasBME680 = true;
    if (!bme680.begin(0x76)) {
        if (!bme680.begin(0x77)) {
        SH_DEBUG_PRINTLN("Could not find a valid BME680 sensor, check wiring!");
        hasBME680 = false;
        }
    }
    if (hasBME680) {
        bme680.setTemperatureOversampling(BME680_OS_8X);
        bme680.setHumidityOversampling(BME680_OS_2X);
        bme680.setPressureOversampling(BME680_OS_4X);
        bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme680.setGasHeater(320, 150); // 320*C for 150 ms
    }

    if (!hasBME680) {
        //No BME680 found, trying out BME280 instead
        hasBME280 = true;
        if (!bme280.begin(0x76)) {
        if (!bme280.begin(0x77)) {
            SH_DEBUG_PRINTLN("Could not find a valid BME280 sensor, check wiring!");
            hasBME280 = false;
        }
        }
    }

    if (hasBME680) {
        SH_DEBUG_PRINTLN("Found a BME680 sensor attached");
    } else if (hasBME280) {
        SH_DEBUG_PRINTLN("Found a BME280 sensor attached");
    }

    //Init the pm SENSOR
    sdsSerial.begin(9600);
    delay(2000);
    sdsSensor.set_sleep(true);

    delay(2000);
}

facade::SensorData Sensors::readMeasurements() {
    facade::SensorData measurements;

    //increase counter
    loopCycleCount++;
    
    //measure noise
    int noiseSessionMax = 0;
    int noiseSessionMin = 1024; 
    int currentSample = 0;
    int noiseMeasureLength = millis();
    for (int sample = 0; sample < NUM_NOISE_SAMPLES; sample++) {
      currentSample = analogRead(NOISE_MEASURE_PIN);
      if (currentSample >0 && currentSample < 1020) {
        if (currentSample > noiseSessionMax) {
          noiseSessionMax = currentSample;
        }
        if (currentSample < noiseSessionMin) {
          noiseSessionMin = currentSample;
        }
      }
    }

    int currentSessionNoise = noiseSessionMax - noiseSessionMin;
    if (currentSessionNoise < 0) {
      currentSessionNoise = 0;
      //something bad has happened, but rather send a 0.
    }

    //This will be different on Arduino and ESP. Probably need to attenuate the ESP a bit.
    //TODO: see up to which freq. is needed to sample in order to keep into 'noise' band.
    //Its fine: Noise measurement took: 95ms with 1000 samples.
    #ifdef NO_CONNECTION_PROFILE
      noiseMeasureLength = millis() - noiseMeasureLength;
      SH_DEBUG_PRINT("Noise measurement took: ");
      SH_DEBUG_PRINT_DEC(noiseMeasureLength, DEC);
      SH_DEBUG_PRINT("ms with ");
      SH_DEBUG_PRINT_DEC(NUM_NOISE_SAMPLES, DEC);
      SH_DEBUG_PRINT(" samples. Minumum = ");
      SH_DEBUG_PRINT_DEC(noiseSessionMin, DEC);
      SH_DEBUG_PRINT(", Maxiumum = ");
      SH_DEBUG_PRINT_DEC(noiseSessionMax, DEC);
      SH_DEBUG_PRINT(" samples. Value = ");
      SH_DEBUG_PRINT_DEC(currentSessionNoise, DEC);
      SH_DEBUG_PRINT(", normalized: ");
      SH_DEBUG_PRINTLN_DEC(currentSessionNoise / 4, DEC);
    #endif
    
    noiseTotal += currentSessionNoise;
    
    if (loopCycleCount >= NUM_MEASURE_SESSIONS) {
      
      // done measuring
      // measure dust, temp, hum
      
      int countTempHumReadouts = 10;
      int temp = 0; 
      int humidity = 0;
      int pressure = 0;
      int altitude = 0;
      int gasResistance = 0;
      while (--countTempHumReadouts > 0) {
        if (hasBME680) {
          if (! bme680.performReading()) {
            SH_DEBUG_PRINTLN("Failed to perform BME reading!");
            //return;
          } else {
            temp = bme680.temperature;
            humidity = bme680.humidity;
            pressure = bme680.pressure / 100;
            gasResistance = bme680.gas_resistance;
            altitude = bme680.readAltitude(SEALEVELPRESSURE_HPA);
          }
        } else if (hasBME280) {
          temp = bme280.readTemperature();
          humidity = bme280.readHumidity();
          pressure = bme280.readPressure() / 100;
          altitude = bme280.readAltitude(SEALEVELPRESSURE_HPA);
        } else {
          // No temp/hum sensor
          break;
        }
        if (humidity <= 0 || humidity > 100 || temp > 100 || temp < -100 || pressure <= 0) {
          //fake result, pause and try again.
          delay(3000);
        } else {
          // OK result
          break;
        }
      }

      if (countTempHumReadouts <=0) {
        //failed to read temp/hum/pres/gas
        //disable BME sensors
        hasBME680 = false;
        hasBME280 = false;
      }
      
      int noise = ((int)noiseTotal / loopCycleCount) / 4 + 10; //mapped to 0-255
      
      bool pm10SensorOK = true;

      //sdsSerial.listen();
      sdsSensor.set_sleep(false);
      sdsSensor.set_mode(sds011::QUERY);
      
      //wait just enough for it to get back on its senses
      delay(5000);
      pm10SensorOK = sdsSensor.query_data_auto(&pm25, &pm10, 10);
      delay(100);
      
      sdsSensor.set_sleep(true);

      if (pm10SensorOK) {
        SH_DEBUG_PRINT("pm25: ");
        SH_DEBUG_PRINT_DEC(pm25, DEC);
        SH_DEBUG_PRINT(", pm10: ");
        SH_DEBUG_PRINT_DEC(pm10, DEC);
      }

      if (noise > 10) {
        SH_DEBUG_PRINT(", noise: ");
        SH_DEBUG_PRINT_DEC(noise, DEC);
      }

      if (hasBME280 || hasBME680) {
        SH_DEBUG_PRINT(", temp: ");
        SH_DEBUG_PRINT_DEC(temp, DEC);
        SH_DEBUG_PRINT(", hum: ");
        SH_DEBUG_PRINT_DEC(humidity, DEC); 
        SH_DEBUG_PRINT(", pres: ");
        SH_DEBUG_PRINT_DEC(pressure, DEC);
        SH_DEBUG_PRINT(", alt: ");
        SH_DEBUG_PRINT_DEC(altitude, DEC);
      }

      if (hasBME680) {
        SH_DEBUG_PRINT(", gasresistance: ");
        SH_DEBUG_PRINT_DEC(gasResistance, DEC); 
      }

      if (pm10SensorOK) {
        measurements.pm10 = pm10;
        measurements.pm25 = pm25;
      }

      if (noise > 10) {
        measurements.noise = noise;
      }

      if (hasBME280 || hasBME680) {
        measurements.temperature = temp;
        measurements.humidity = humidity;
        measurements.pressure = pressure;
        measurements.altitude = altitude;

        if (hasBME680) {
            measurements.gasResistance = gasResistance;
        }
      }
    
      //reset
      noiseTotal = 0;
      loopCycleCount = 0;
    }

    return measurements;
}

bool SensorData::hasAnyData() {
  int arr[] = {pm10, pm25, altitude, temperature, pressure, humidity, gasResistance};
  
  for (int i = 0; i < sizeof(arr); i++) {
    if (arr[i] != INT_MIN) {
      return true;
    }

    return false;
  }
}

void NullSensors::init() {
}

facade::SensorData NullSensors::readMeasurements() {
    facade::SensorData data;

    data.temperature = random(0, 25);
    data.humidity = random(500, 1000);
    data.pressure = random(250, 499);
    data.altitude = random(1000, 10000);
    data.gasResistance = random(10, 100);

    data.pm10 = random(50, 300);
    data.pm25 = random(5, 100);

    return data;
}
