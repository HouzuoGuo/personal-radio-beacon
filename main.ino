// Personal radio beacon and DNS TXT record client implemented with Wemos TTGO LORA32 development with built-in OLED.
#include <Arduino.h>

// Radio libraries came installed with Board Manager board "esp32 by Espressif Systems version 1.0.1".
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/*
OLED libraries came installed with "ESP8266 and ESP32 Oled Driver for SSD1306 display by Daniel Eichhorn, Fabrice Weinberg Version 4.0.0".
*/
#include <Wire.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>

// The built-in OLED uses I2C for control
#define OLED_I2C_ADDR 0x3c
#define OLED_I2C_SDA 4
#define OLED_I2C_SCL 15
#define OLED_PIN_RESET_START 16
SSD1306 oled(OLED_I2C_ADDR, OLED_I2C_SDA, OLED_I2C_SCL);
OLEDDisplayUi oled_ui(&oled);

// oled_redraw redraws the content of the entire OLED.
void oled_redraw(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0, 0, "hello world");
}

FrameCallback oled_only_frame[] = {oled_redraw};

// oled_setup resets and initialises OLED hardware and its user interface library.
void oled_setup()
{
  // Reset OLED by setting its pin to LOW, wait 50MS (magic indeed), and set its pin to HIGH for display operation.
  pinMode(OLED_PIN_RESET_START, OUTPUT);
  digitalWrite(OLED_PIN_RESET_START, LOW);
  delay(50);
  digitalWrite(OLED_PIN_RESET_START, HIGH);

  // Initialise user interface
  // Further lowering FPS results in negative "budget milliseconds" in oled_loop, possibly due to bug in OLED library?
  oled_ui.setTargetFPS(10);
  oled_ui.setIndicatorPosition(BOTTOM);
  oled_ui.setIndicatorDirection(LEFT_RIGHT);
  oled_ui.setFrameAnimation(SLIDE_LEFT);
  oled_ui.setFrames(oled_only_frame, 1);
  oled_ui.init();
}

/*
oled_loop runs an iteration of the micro-controller's main loop.
It redraws content on OLED screen at a stable number of frames per second, and calculates the remainder
number of milliseconds available for more program logic to run.
If the additional program logic takes more time to run than the budget (milliseconds) available, OLED
will simply miss couple of frames, costing in UI responsiveness, but otherwise not a big deal.
*/
void oled_loop(void (*other_logic)(int))
{
  int budget_ms = oled_ui.update();
  Serial.printf("budget is %d\n", budget_ms);
  if (budget_ms > 0)
  {
    other_logic(budget_ms);
  }
}

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

  oled_setup();

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

void scan_loop(int _)
{
  scanWifi();
  scanBT();
}

void loop()
{
  oled_loop(scan_loop);
}
