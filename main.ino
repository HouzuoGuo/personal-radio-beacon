// Personal radio beacon and DNS TXT record client implemented with TTGO T-Beam ESP32 LoRa32 development board with OLED and battery holder.
#include <Arduino.h>

// OLED libraries came installed with "ESP8266 and ESP32 Oled Driver for SSD1306 display by Daniel Eichhorn, Fabrice Weinberg Version 4.0.0".
#include <Wire.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>

// I soldered OLED SCL and SDA pins to board pin 22 and 21 (same side as the LoRA antenna), the OLED uses I2C for control.
#define OLED_I2C_ADDR 0x3c
#define OLED_I2C_SCL 22
#define OLED_I2C_SDA 21
// The OLED RESET+START pin does not seem to physically exist on the board.
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

#include <Arduino.h>

// Radio libraries came installed with Board Manager board "esp32 by Espressif Systems version 1.0.1".
#include <WiFi.h>

// WIFI_MAX_DISCOVERED_NETWORKS is the maximum number of stations allowed to be discovered during a scan.
#define WIFI_MAX_DISCOVERED_NETWORKS 100

// wifi_scan_result is a record of details about a discovered WiFi station.
struct wifi_scan_result
{
    int32_t rssi;
    String ssid;
    String mac;
    wifi_auth_mode_t auth_mode;
};

// wifi_scan_results is an array of discovered WiFi stations in the latest round of scan.
struct wifi_scan_result wifi_scan_results[WIFI_MAX_DISCOVERED_NETWORKS] = {};
// wifi_scan_results_next_index is the index number of array which the next discovered station will occupy. In between each scan it is reset to 0;
int wifi_scan_results_next_index = 0;

// wifi_setup initialises WiFi hardware and scanner.
void wifi_setup()
{
    // There is nothing to initialise right now
}

// wifi_scan conducts a fresh round of discovery of nearby WiFi networks.
void wifi_scan()
{
    // WiFi scan has to be conducted in station mode without an active network connection
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int num_stations = WiFi.scanNetworks();
    // Store up to a predefined number of discovered networks
    wifi_scan_results_next_index = 0;
    if (num_stations == 0)
    {
        return;
    }
    if (num_stations > WIFI_MAX_DISCOVERED_NETWORKS)
    {
        num_stations = WIFI_MAX_DISCOVERED_NETWORKS;
    }
    for (; wifi_scan_results_next_index < num_stations; ++wifi_scan_results_next_index)
    {
        struct wifi_scan_result result;
        result.ssid = WiFi.SSID(wifi_scan_results_next_index);
        result.rssi = WiFi.RSSI(wifi_scan_results_next_index);
        result.mac = WiFi.BSSIDstr(wifi_scan_results_next_index);
        result.auth_mode = WiFi.encryptionType(wifi_scan_results_next_index);
        wifi_scan_results[wifi_scan_results_next_index] = result;
    }
    WiFi.scanDelete();
}

#include <Arduino.h>

// Radio libraries came installed with Board Manager board "esp32 by Espressif Systems version 1.0.1".
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// BT_MAX_DISCOVERED_DEVICES is the maximum number of devices allowed to be discovered during a scan.
#define BT_MAX_DISCOVERED_DEVICES 100

// bt_scan_result is a record of details about a discovered bluetooth device.
struct bt_scan_result
{
    int rssi;
    std::string name;
    std::string mac;
};

// bt_scanner scans for bluetooth devices nearby and decodes their signal strength and manufacturer information.
BLEScan *bt_scanner = NULL;
// bt_scan_results is an array of discovered bluetooth devices from the latest round of scan.
struct bt_scan_result bt_scan_results[BT_MAX_DISCOVERED_DEVICES] = {};
// bt_scan_result_next_index is the index number of array which the next discovered device will occupy. In between each scan it is reset to 0;
int bt_scan_results_next_index = 0;

// BTDeviceDiscoveryCallBack stores the next discovered device in the result array.
class BTDeviceDiscoveryCallBack : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice dev)
    {
        // Store up to a predefined number of discovered devices
        if (bt_scan_results_next_index < BT_MAX_DISCOVERED_DEVICES)
        {
            struct bt_scan_result result;
            result.name = dev.getName();
            result.mac = dev.getAddress().toString();
            result.rssi = dev.getRSSI();
            bt_scan_results[bt_scan_results_next_index] = result;
            bt_scan_results_next_index++;
        }
    }
};

// bt_setup initialises bluetooth hardware and scanner.
void bt_setup()
{
    BLEDevice::init("");
    bt_scanner = BLEDevice::getScan();
    bt_scanner->setAdvertisedDeviceCallbacks(new BTDeviceDiscoveryCallBack());
    bt_scanner->setActiveScan(true);
    // 100 and 99 seem like magic borrowed from examples, I need to develop a better understanding of these two numbers.
    bt_scanner->setInterval(100);
    bt_scanner->setWindow(99);
}

// bt_scan conducts a fresh round of discovery of nearby bluetooth devices.
void bt_scan(int max_duration_sec)
{
    // Reset storage index to 0 and let newly discovered devices fill the array up one by one
    bt_scan_results_next_index = 0;
    BLEScanResults scan_results = bt_scanner->start(max_duration_sec, false);
    bt_scanner->clearResults();
}

void setup()
{
    Serial.begin(115200);

    oled_setup();
    wifi_setup();
    bt_setup();

    delay(1000);
}

void scan_loop(int _)
{
    wifi_scan();
    bt_scan(5);
    Serial.println("WiFi scan result:");
    for (int i = 0; i < wifi_scan_results_next_index; ++i)
    {
        Serial.printf("%d - %s - %s\r\n", wifi_scan_results[i].rssi, wifi_scan_results[i].mac.c_str(), wifi_scan_results[i].ssid.c_str());
    }
    Serial.println("BT scan result:");
    for (int i = 0; i < bt_scan_results_next_index; ++i)
    {
        Serial.printf("%d - %s - %s\r\n", bt_scan_results[i].rssi, bt_scan_results[i].mac.c_str(), bt_scan_results[i].name.c_str());
    }
}

void loop()
{
    oled_loop(scan_loop);
}
