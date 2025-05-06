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
#define PIN_LUMIERE 34
#define PIN_WIND_DIRECTION 35
#define PIN_WIND_SPEED 27
#define PIN_TOTAL_RAIN_FALL 23

SFEWeatherMeterKit weatherMeterKit(PIN_WIND_DIRECTION, PIN_WIND_SPEED, PIN_TOTAL_RAIN_FALL);
Adafruit_DPS310 dps;
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLE2901 *descriptor_2901 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// En-tête de fonctions
String lectureCapteurs();
String parseValeurs(String data);
void temperatureHumidity(float *humidite, float *temperature);
float intensiteLumineuse();


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

void setup() {
  // Moniteur de la station de capteurs
  Serial.begin(115200);
  // Communication UART
  Serial1.begin(9600, SERIAL_8N1, RX, TX);

  // Configuration des objets dps et weatherMeterKit
  if (! dps.begin_I2C()) {
    Serial.println("Failed to find DPS");
    while (1) yield();
  }
  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
  weatherMeterKit.begin();

 
  // Création du périphérique Bluetooth avec Server, Service et Characteristic correspondant
  BLEDevice::init("Alex - Station de capteurs");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );

  // Création du BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("My own description for this characteristic.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  pCharacteristic->addDescriptor(descriptor_2901);
  
  // Activation du service et du advertising
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("En attente de connection pour l'envoie de notification...");
}
 
void loop() {
  String donnees = lectureCapteurs();

  // Envoi des données des capteurs sur le lien UART et notification BLE
  if (deviceConnected) {
    if (donnees != "" && donnees != NULL) {
      Serial1.println(donnees);
      Serial.println(parseValeurs(donnees));
      Serial.println("Données transmises");

      pCharacteristic->setValue((uint8_t *)&value, 4);
      pCharacteristic->notify();
      value++;
    }
  }
  
  // Déconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = deviceConnected;
  }
  
  // Connection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}


String lectureCapteurs() {
  sensors_event_t temp_event, pressure_event;

  while (!dps.temperatureAvailable() || !dps.pressureAvailable()) {
    return ""; // retourne une chaîne vide si les données ne sont pas prêtes
  }
  
  // Lecture de l'humidité (%) et de la température (°C)
  float humidite = 0;
  float temperature = 0;
  temperatureHumidity(&humidite, &temperature);

  // Lecture de l'intensité lumineuse (W/m²)
  float irradiance = intensiteLumineuse();

  // Lecture de la température (°C) et pression (hPa)
  dps.getEvents(&temp_event, &pressure_event);

  // Lecture de la direction (°), (km/h) et (mm)
  float windDir = weatherMeterKit.getWindDirection();
  float windSpeed = weatherMeterKit.getWindSpeed();
  float rainfall = weatherMeterKit.getTotalRainfall();


  String valeursCapteurs = 
    String(humidite, 0) + ";" +
    String(temperature, 2) + ";" +
    String(irradiance, 2) + ";" +
    String(temp_event.temperature, 2) + ";" +
    String(pressure_event.pressure, 2) + ";" +
    String(windDir, 1) + ";" +
    String(windSpeed, 1) + ";" +
    String(rainfall, 1);
  

  return valeursCapteurs;
}

// Retourne une chaine permettant de présenter les valeurs de capteurs avec leurs unités
String parseValeurs(String data) {
  float valeurs[8];
  int i = 0;

  while (data.length() > 0 && i < 8) {
    int index = data.indexOf(';');
    if (index == -1) {
      valeurs[i++] = data.toFloat();
      break;
    } else {
      valeurs[i++] = data.substring(0, index).toFloat();
      data = data.substring(index + 1);
    }
  }

  String result = "";
  result += "Humidité : " + String(valeurs[0], 0) + " %\n";
  result += "Température (capteur d'humidité) : " + String(valeurs[1], 2) + " °C\n";
  result += "Intensité lumineuse : " + String(valeurs[2], 2) + " W/m²\n";
  result += "Température (baromètre) : " + String(valeurs[3], 2) + " °C\n";
  result += "Pression : " + String(valeurs[4], 2) + " hPa\n";
  result += "Direction du vent : " + String(valeurs[5], 1) + " °\n";
  result += "Vitesse du vent : " + String(valeurs[6], 1) + " km/h\n";
  result += "Précipitations totales : " + String(valeurs[7], 1) + " mm";

  return result;
}

// Lecture du capteur Température en °C + Humidité %
void temperatureHumidity(float *humidite, float *temperature) {
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

// Lecture du capteur d'intensité lumineuse en W/m²
float intensiteLumineuse() {
  float voltage = analogRead(PIN_LUMIERE) * 3.3 / 4095.0;
  float lux = pow(voltage / 3.3, 2.0) * 1000.0;
  return lux / 120.0;
}
