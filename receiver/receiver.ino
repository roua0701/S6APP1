#include "BLEDevice.h"
 
#define TX 13
#define RX 14
 
// Constantes globales
static BLEUUID serviceUUID("2d6abefe-59a1-4352-b95e-8e1fc2ad8e77");
static BLEUUID charUUID("a2b6e2b0-c7d7-4eee-b879-61573f8d7c30");
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
boolean canReceiveData = false;

// Prototypes de fonctions
String parseValeurs(String data);


static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
   Serial.print("data: ");
  Serial.write(pData, length);
  Serial.println();
  canReceiveData = true;
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}
 
  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};


bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
 
  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");
 
  pClient->setClientCallbacks(new MyClientCallback());
 
  // Connect to the remove BLE Server.
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  pClient->setMTU(517);
 
  // Réference du service
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");
 
  // Réference de la caractéristique
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");
 
  // Valueur de la caractéristique
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }
 
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
 
  connected = true;
  return true;
}
 
/**
 * Scan les serveurs BLE
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
 
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
 
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
 
    }
  }
};
 
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RX, TX); // Bluetooth
  Serial.println("Station de base");
 
  BLEDevice::init("Alex - Station de base");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}
 
void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connection établie.");
    } else {
      Serial.println("Connection échouée.");
    }
    doConnect = false;
  }
 
  if (!connected && doScan) {
    BLEDevice::getScan()->start(0);
  }
 
  if (canReceiveData && Serial1.available()) {
    String incomingData = Serial1.readStringUntil('\n');
    Serial.println("Valeurs de capteurs modifiees.");
    String donnees = parseValeurs(incomingData);
    if(incomingData != "" && incomingData != NULL){
      Serial.println(donnees);
    }
    else{
      Serial.println("Valeurs invalides");
    }
    Serial.println("");
    canReceiveData = false;
  } else {
    delay(1000);
  }
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
