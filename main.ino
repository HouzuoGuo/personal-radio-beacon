// Personal radio beacon and DNS TXT record client implemented with TTGO T-Beam ESP32 LoRa32 development board with OLED and battery holder.
#include <Arduino.h>

/***
 *     ██████╗ ██╗     ███████╗██████╗ 
 *    ██╔═══██╗██║     ██╔════╝██╔══██╗
 *    ██║   ██║██║     █████╗  ██║  ██║
 *    ██║   ██║██║     ██╔══╝  ██║  ██║
 *    ╚██████╔╝███████╗███████╗██████╔╝
 *     ╚═════╝ ╚══════╝╚══════╝╚═════╝ 
 *                                     
 */

// OLED libraries came installed with "ESP8266 and ESP32 Oled Driver for SSD1306 display by Daniel Eichhorn, Fabrice Weinberg Version 4.0.0".
#include <Wire.h>
#include <SSD1306Wire.h>
#include <OLEDDisplay.h>
#include <OLEDDisplayFonts.h>
#include <OLEDDisplayUi.h>

// I soldered OLED SCL and SDA pins to board pin 22 and 21 (same side as the LoRA antenna), the OLED uses I2C for control.
#define OLED_I2C_ADDR 0x3c
#define OLED_I2C_SCL 22
#define OLED_I2C_SDA 21
// OLED RESET/START pin does not seem to physically exist on the board, though code samples always use the magic number 16.
#define OLED_PIN_RESET_START 16

SSD1306Wire oled(OLED_I2C_ADDR, OLED_I2C_SDA, OLED_I2C_SCL);
OLEDDisplayUi oled_ui(&oled);

// oled_redraw redraws the content of the entire OLED.
void oled_redraw(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t unused_x, int16_t unused_y)
{
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);
    display->drawStringMaxWidth(0, 0, 128, "012345678901234567890123456789012345678901234567890123456789");
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

    // Further lowering FPS results in negative "budget milliseconds" in oled_loop, possibly due to bug in OLED library?
    oled_ui.setTargetFPS(10);
    // Not going to use automated transition or frame indicators
    oled_ui.disableAllIndicators();
    oled_ui.disableAutoTransition();
    oled_ui.setFrames(oled_only_frame, 1);
    oled_ui.init();
    
    /*
    Flip vertically so that display orientation is same as the LoRa antenna on this TTGO T-Beam board.
    The flip orientation function works only after UI is initialised.
    */
    oled.flipScreenVertically();
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

/***
 *    ██╗    ██╗██╗███████╗██╗
 *    ██║    ██║██║██╔════╝██║
 *    ██║ █╗ ██║██║█████╗  ██║
 *    ██║███╗██║██║██╔══╝  ██║
 *    ╚███╔███╔╝██║██║     ██║
 *     ╚══╝╚══╝ ╚═╝╚═╝     ╚═╝
 *                            
 */

// WiFi radio library came installed with Board Manager board "esp32 by Espressif Systems version 1.0.1".
#include <WiFi.h>
#include <WiFiType.h>

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

// wifi_scan conducts a fresh round of discovery of nearby WiFi networks.
void wifi_scan()
{
    // WiFi scan must be done in station mode without any active connection
    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_STA);
    // Use maximum transmission power
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
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

/*
wifi_start_access_point instructs WiFi tranceiver to work as an access point, which will broadcast its network name to nearby WiFi scanners.
The access point does not provide network access and chooses a random channel to work on.
*/
bool wifi_start_access_point(const char *ssid)
{
    // WiFi AP advertisement must be done in access point mode without any active connection
    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_AP);
    // Use maximum transmission power
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    return WiFi.softAP(ssid, NULL, random(1, 14), 0, 1);
}

/***
 *    ██████╗ ██╗     ██╗   ██╗███████╗████████╗ ██████╗  ██████╗ ████████╗██╗  ██╗    ██╗     ███████╗
 *    ██╔══██╗██║     ██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔═══██╗╚══██╔══╝██║  ██║    ██║     ██╔════╝
 *    ██████╔╝██║     ██║   ██║█████╗     ██║   ██║   ██║██║   ██║   ██║   ███████║    ██║     █████╗  
 *    ██╔══██╗██║     ██║   ██║██╔══╝     ██║   ██║   ██║██║   ██║   ██║   ██╔══██║    ██║     ██╔══╝  
 *    ██████╔╝███████╗╚██████╔╝███████╗   ██║   ╚██████╔╝╚██████╔╝   ██║   ██║  ██║    ███████╗███████╗
 *    ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝  ╚═════╝    ╚═╝   ╚═╝  ╚═╝    ╚══════╝╚══════╝
 *                                                                                                     
 */
// Bluetooth LE radio library came installed with Board Manager board "esp32 by Espressif Systems version 1.0.1".
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEAdvertisedDevice.h>

// BT_MAX_DISCOVERED_DEVICES is the maximum number of devices allowed to be discovered during a scan.
#define BT_MAX_DISCOVERED_DEVICES 100
/*
BT_MAX_AD_NAME_LEN is the maximum length of name advertised by bluetooth LE tranceiver while it is made discoverable.
ESP32 only has a bluetooth LE 4.? tranceiver, its protocol specification only offers up to 29 characters for a device name.
*/
#define BT_MAX_AD_NAME_LEN 29

// bt_scan_result is a record of details about a discovered bluetooth device.
struct bt_scan_result
{
    int rssi;
    std::string name;
    std::string mac;
};

// bt_ad advertises the bluetooth tranceiver, making it discoverable to nearby devices.
BLEAdvertising *bt_ad = NULL;
// bt_scanner scans for bluetooth devices nearby and decodes their signal strength and manufacturer information.
BLEScan *bt_scanner = NULL;
// bt_scan_results is an array of discovered bluetooth devices from the latest round of scan.
struct bt_scan_result bt_scan_results[BT_MAX_DISCOVERED_DEVICES] = {};
// bt_scan_result_next_index is the index number of array which the next discovered device will occupy. In between each scan it is reset to 0;
int bt_scan_results_next_index = 0;

// bt_setup initialises bluetooth tranceiver hardware and library.
void bt_setup()
{
    /*
    The initialisation name also becomes the default device name during advertisement.
    The function that turns on advertisement will change the device name later, hence this name does not matter.
    Advertised device name needs to be changed by manipulating advertisement data record, because BLEDevice::init can be used only once.
    */
    BLEDevice::init("HZGL-PRB");
    bt_scanner = BLEDevice::getScan();
    bt_ad = BLEDevice::getAdvertising();
}

// bt_start_advertisement instructs bluetooth tranceiver to make itself discoverable, which will make its name visible to nearby scanners.
void bt_start_advertisement(std::string name)
{
    // Use maximum transmission power
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    bt_ad->stop();
    // Seem good to do...
    bt_ad->setScanResponse(false);
    bt_ad->setMinPreferred(0x00);
    // Allow everyone to discover this tranceiver but no one to connect due to having an empty white-list
    bt_ad->setScanFilter(false, true);
    /*
    Advertise the specified device name.
    Advertised device name needs to be changed by manipulating advertisement data record, because BLEDevice::init can be used only once.
    */
    BLEAdvertisementData ad_data;
    std::string truncated_name = name;
    truncated_name.resize(BT_MAX_AD_NAME_LEN);
    ad_data.setName(truncated_name);
    bt_ad->setAdvertisementData(ad_data);
    bt_ad->setScanResponseData(ad_data);
    bt_ad->start();
}

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

// bt_scan conducts a fresh round of discovery of nearby bluetooth devices.
void bt_scan(int max_duration_sec)
{
    // Use maximum transmission power
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    bt_scanner->setAdvertisedDeviceCallbacks(new BTDeviceDiscoveryCallBack());
    bt_scanner->setActiveScan(true);
    // 100 and 99 seem like magic borrowed from examples, I need to develop a better understanding of these two numbers.
    bt_scanner->setInterval(100);
    bt_scanner->setWindow(99);
    // Reset storage index to 0 and let newly discovered devices fill the array up one by one
    bt_scan_results_next_index = 0;
    BLEScanResults scan_results = bt_scanner->start(max_duration_sec, false);
}

#include <esp_gap_ble_api.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "BluetoothSerial.h"

/***
 *    ██╗      ██████╗ ██████╗  █████╗ 
 *    ██║     ██╔═══██╗██╔══██╗██╔══██╗
 *    ██║     ██║   ██║██████╔╝███████║
 *    ██║     ██║   ██║██╔══██╗██╔══██║
 *    ███████╗╚██████╔╝██║  ██║██║  ██║
 *    ╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝
 *                                     
 */

// LoRa radio library came installed with Library Manager library "LoRa by Sandeep Mistry - Supports Semtech SX1276/77/78/79 based boards/shields"
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>

// SPI and pin definitions are inspired by https://github.com/LilyGO/TTGO-T-Beam/blob/master/OLED_LoRa_Receive/OLED_LoRa_Receive.ino
#define LORA_SPI_SCK 5
#define LORA_SPI_MISO 19
#define LORA_SPI_MOSI 27
#define LORA_PIN_SS 18
#define LORA_PIN_RST 14
#define LORA_PIN_DI0 26

/*
The tables of channel frequencies are inspired by:
- Minimum set of channels supported by all LoRaWAN devices: https://lora-alliance.org/sites/default/files/2018-04/lorawantm_regional_parameters_v1.1rb_-_final.pdf
- Waspmote's take on LoRaWAN channel assignment: http://www.libelium.com/development/waspmote/documentation/waspmote-lorawan-networking-guide/?action=download
- Waspmote's take on LoRa (no WAN) channel assignment: http://www.libelium.com/development/waspmote/documentation/waspmote-lora-868mhz-915mhz-sx1272-networking-guide/?action=download
*/

// lora_setup initialises LoRa hardware and driver.
void lora_setup()
{
    SPI.begin(LORA_SPI_SCK, LORA_SPI_MISO, LORA_SPI_MOSI, LORA_PIN_SS);
    LoRa.setPins(LORA_PIN_SS, LORA_PIN_RST, LORA_PIN_DI0);
}

/***
 *    ███╗   ███╗ █████╗ ██╗███╗   ██╗
 *    ████╗ ████║██╔══██╗██║████╗  ██║
 *    ██╔████╔██║███████║██║██╔██╗ ██║
 *    ██║╚██╔╝██║██╔══██║██║██║╚██╗██║
 *    ██║ ╚═╝ ██║██║  ██║██║██║ ╚████║
 *    ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝
 *                                    
 */

void setup()
{
    Serial.begin(115200);

    // Seed random number generator
    unsigned long rand_seed = 0;
    for (int i = 0; i < 32; i++)
    {
        rand_seed += analogRead(i);
    }
    Serial.printf("%s: random seed is %d\n", __func__, rand_seed);
    randomSeed(rand_seed);

    oled_setup();
    bt_setup();
    lora_setup();
}

char name[20];

int turn = 0;
void each_turn(int _)
{
    turn++;
    sprintf(name, "HZGL-PRB-%d", turn);
    switch (turn % 5)
    {
    case 0:
        Serial.println("WiFi advertisement");
        wifi_start_access_point(name);
        delay(5 * 1000);
        break;
    case 1:
        Serial.println("Bluetooth advertisement");
        bt_start_advertisement(std::string(name));
        delay(5 * 1000);
        break;
    case 2:
        Serial.println("WiFi scan");
        wifi_scan();
        Serial.println("WiFi scan result:");
        for (int i = 0; i < wifi_scan_results_next_index; ++i)
        {
            Serial.printf("%d - %s - %s\r\n", wifi_scan_results[i].rssi, wifi_scan_results[i].mac.c_str(), wifi_scan_results[i].ssid.c_str());
        }
        break;
    case 3:
        Serial.println("Bluetooth scan");
        bt_scan(5);
        Serial.println("BT scan result:");
        for (int i = 0; i < bt_scan_results_next_index; ++i)
        {
            Serial.printf("%d - %s - %s\r\n", bt_scan_results[i].rssi, bt_scan_results[i].mac.c_str(), bt_scan_results[i].name.c_str());
        }
        break;
    case 4:
        break;
    }
}

void loop()
{
    oled_loop(each_turn);
}
