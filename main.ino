#include <Arduino.h>

#include "WiFi.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan *bleScan;

class ScanCallBack : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
  }
};

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  BLEDevice::init("");
  bleScan = BLEDevice::getScan();
  bleScan->setAdvertisedDeviceCallbacks(new ScanCallBack());
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(99);

  delay(1000);
}

void scanWifi()
{
  Serial.println("wifi scan start");
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    Serial.println("no wifi around");
    return;
  }

  Serial.print(n);
  Serial.println(" networks found");
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
    delay(10);
  }
  Serial.println("wifi scan done");
}

void scanBT()
{
  Serial.println("bt scan start");
  BLEScanResults foundDevices = bleScan->start(5, false);
  Serial.print("devices found: ");
  Serial.println(foundDevices.getCount());
  bleScan->clearResults();
  Serial.println("bt scan done");
}

void loop()
{
  scanWifi();
  scanBT();
}
