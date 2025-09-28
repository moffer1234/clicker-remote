#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"

BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

const int BUTTON_NEXT = 5;
const int BUTTON_PREV = 18;
const int BUTTON_CUE = 19;

bool nextButtonState = false;
bool prevButtonState = false;
bool cueButtonState = false;
bool lastNextButtonState = false;
bool lastPrevButtonState = false;
bool lastCueButtonState = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long lastStatusPrint = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("✅ BLE CLIENT CONNECTED!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("❌ BLE CLIENT DISCONNECTED!");
      Serial.println("🔄 Restarting advertising...");
      BLEDevice::startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 PROMPTLY CLICKER BLE DEBUG VERSION");
  Serial.println("=====================================");
  
  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_PREV, INPUT_PULLUP);
  pinMode(BUTTON_CUE, INPUT_PULLUP);
  
  Serial.println("📡 Initializing BLE...");
  BLEDevice::init("Promptly Clicker");
  
  Serial.println("🖥️  Creating BLE Server...");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  Serial.println("⚙️  Creating BLE Service...");
  BLEService *pService = pServer->createService(SERVICE_UUID);

  Serial.println("📝 Creating TX Characteristic...");
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  Serial.println("▶️  Starting BLE Service...");
  pService->start();
  
  Serial.println("📢 Starting BLE Advertising...");
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  
  Serial.println("\n✨ PROMPTLY CLICKER READY!");
  Serial.println("Device Name: 'Promptly Clicker'");
  Serial.println("Service UUID: " + String(SERVICE_UUID));
  Serial.println("TX Char UUID: " + String(CHARACTERISTIC_UUID_TX));
  Serial.println("\n🔍 Looking for this device in iOS app...");
  Serial.println("(Note: Won't appear in iOS Settings > Bluetooth)");
  Serial.println("=====================================\n");
}

void loop() {
  // Print status every 10 seconds
  if (millis() - lastStatusPrint > 10000) {
    Serial.println("⏰ Status: " + String(deviceConnected ? "CONNECTED" : "ADVERTISING"));
    lastStatusPrint = millis();
  }
  
  // Handle reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("🔄 Restarted advertising for reconnection");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  bool nextReading = !digitalRead(BUTTON_NEXT);
  bool prevReading = !digitalRead(BUTTON_PREV);
  bool cueReading = !digitalRead(BUTTON_CUE);
  
  if (nextReading != lastNextButtonState || prevReading != lastPrevButtonState || cueReading != lastCueButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (nextReading != nextButtonState) {
      nextButtonState = nextReading;
      
      if (nextButtonState) {
        Serial.println("🔘 NEXT button pressed");
        if (deviceConnected) {
          pTxCharacteristic->setValue("1");
          pTxCharacteristic->notify();
          Serial.println("📤 Sent: '1' (NEXT LINE)");
        } else {
          Serial.println("⚠️  No client connected - button press ignored");
        }
      }
    }
    
    if (prevReading != prevButtonState) {
      prevButtonState = prevReading;
      
      if (prevButtonState) {
        Serial.println("🔘 PREV button pressed");
        if (deviceConnected) {
          pTxCharacteristic->setValue("0");
          pTxCharacteristic->notify();
          Serial.println("📤 Sent: '0' (PREV LINE)");
        } else {
          Serial.println("⚠️  No client connected - button press ignored");
        }
      }
    }
    
    if (cueReading != cueButtonState) {
      cueButtonState = cueReading;
      
      if (cueButtonState) {
        Serial.println("🔘 CUE button pressed");
        if (deviceConnected) {
          pTxCharacteristic->setValue("2");
          pTxCharacteristic->notify();
          Serial.println("📤 Sent: '2' (EXECUTE NEXT CUE)");
        } else {
          Serial.println("⚠️  No client connected - button press ignored");
        }
      }
    }
  }
  
  lastNextButtonState = nextReading;
  lastPrevButtonState = prevReading;
  lastCueButtonState = cueReading;
  
  delay(10);
}