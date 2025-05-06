#include <SparkFun_Weather_Meter_Kit_Arduino_Library.h>
#include <Adafruit_DPS310.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>
 
#define SERVICE_UUID        "2d6abefe-59a1-4352-b95e-8e1fc2ad8e77"
#define CHARACTERISTIC_UUID "a2b6e2b0-c7d7-4eee-b879-61573f8d7c30"
#define TX 14
#define RX 13

// Pins for Weather Carrier with ESP32 Processor Board
int windDirectionPin = 35;
int windSpeedPin = 27;
int rainfallPin = 23;

SFEWeatherMeterKit weatherMeterKit(windDirectionPin, windSpeedPin, rainfallPin);
Adafruit_DPS310 dps;
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLE2901 *descriptor_2901 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

String lectureCapteurs();

void temperatureHumidity(float *humidite, float *temperature){
  int i, j;
  int duree[42];
  unsigned long pulse;
  byte data[5];
  int broche = 16;
  
  pinMode(broche, OUTPUT_OPEN_DRAIN);
  digitalWrite(broche, HIGH);
  delay(250);
  digitalWrite(broche, LOW);
  delay(20);
  digitalWrite(broche, HIGH);
  delayMicroseconds(40);
  pinMode(broche, INPUT_PULLUP);
  
  while (digitalRead(broche) == HIGH);
  i = 0;

  do {
        pulse = pulseIn(broche, HIGH);
        duree[i] = pulse;
        i++;
  } while (pulse != 0);
 
  if (i != 42) 
    Serial.printf(" Erreur timing \n"); 

  for (i=0; i<5; i++) {
    data[i] = 0;
    for (j = ((8*i)+1); j < ((8*i)+9); j++) {
      data[i] = data[i] * 2;
      if (duree[j] > 50) {
        data[i] = data[i] + 1;
      }
    }
  }

  if ( (data[0] + data[1] + data[2] + data[3]) != data[4] ) 
    Serial.println(" Erreur checksum");

  *humidite = data[0] + (data[1] / 256.0);
  *temperature = data [2] + (data[3] / 256.0);
  
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RX, TX);
  Serial.println("Station de capteurs");

  if (! dps.begin_I2C()) {
    Serial.println("Failed to find DPS");
    while (1) yield();
  }
  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
  weatherMeterKit.begin();

 
  // Create the BLE Device
  BLEDevice::init("ESP32");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );
  // Creates BLE Descriptor 0x2902: Client Characteristic Configuration Descriptor (CCCD)
  pCharacteristic->addDescriptor(new BLE2902());
  // Adds also the Characteristic User Description - 0x2901 descriptor
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("My own description for this characteristic.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  pCharacteristic->addDescriptor(descriptor_2901);
  // Start the service
  pService->start();
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}
 
void loop() {
  Serial1.println("Valeurs des capteurs modifiées:");
  Serial1.println(lectureCapteurs());
  Serial.println(lectureCapteurs());
  delay(5000);
  Serial.println("Données transmises");
 
    // notify changed value
  if (deviceConnected) {
    pCharacteristic->setValue((uint8_t *)&value, 4);
    pCharacteristic->notify();
    value++;
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
  delay(1000);
}


String lectureCapteurs() {
  sensors_event_t temp_event, pressure_event;

  //while (!dps.temperatureAvailable() || !dps.pressureAvailable()) {
  //  return ""; // retourne une chaîne vide si les données ne sont pas prêtes
  //}

  float humidite = 0;
  float temperature = 0;
  temperatureHumidity(&humidite, &temperature);
  //Serial.printf(" Humidite = %4.0f %%  Temperature = %4.2f *C \n", humidite, temperature);
  
  int lumiere = analogRead(34);
  float voltage = lumiere * 3.3 / 4095.0;
  float lux = pow(voltage / 3.3, 2.0) * 1000.0;
  float irradiance = lux / 120.0;  // en W/m²
  //Serial.printf(" Intensité lumineuse = %4.2f W/m²\n", irradiance);
  
  dps.getEvents(&temp_event, &pressure_event);
  //Serial.print(F(" Température du baromètre = "));
  //Serial.print(temp_event.temperature);
  //Serial.println(" *C");

  //Serial.print(F(" Pression barométrique = "));
  //Serial.print(pressure_event.pressure);
  //Serial.println(" hPa"); 

  // Print data from weather meter kit
  float windDir = weatherMeterKit.getWindDirection();
  float windSpeed = weatherMeterKit.getWindSpeed();
  float rainfall = weatherMeterKit.getTotalRainfall();

  //Serial.print(F("Wind direction (degrees): "));
  //Serial.print(windDir, 1);
  //Serial.print(F("\t\t"));
  //Serial.print(F("Wind speed (kph): "));
  //Serial.print(windSpeed, 1);
  //Serial.print(F("\t\t"));
  //Serial.print(F("Total rainfall (mm): "));
  //Serial.println(rainfall, 1);

  //Serial.println();

  String valeursCapteurs = 
    "Humidité : " + String(humidite, 0) + " %\n" +
    "Température (capteur d'humidité) : " + String(temperature, 2) + " °C\n" +
    "Intensité lumineuse : " + String(irradiance, 2) + " W/m²\n" +
    "Température (baromètre) : " + String(temp_event.temperature, 2) + " °C\n" +
    "Pression : " + String(pressure_event.pressure, 2) + " hPa\n" +
    "Direction du vent : " + String(windDir, 1) + " °\n" +
    "Vitesse du vent : " + String(windSpeed, 1) + " km/h\n" +
    "Précipitations totales : " + String(rainfall, 1) + " mm";

  return valeursCapteurs;
}
