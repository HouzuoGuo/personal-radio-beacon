# Hardware specification
- Hardware unit name: Wemos® TTGO LORA32 868/915Mhz ESP32 LoRa OLED 0.96 Inch Blue Display Bluetooth WIFI ESP-32 Development Board Module With Antenna
- Date of purchase: 2019-02-27
- Date of delivery: 2019-03-21
- Board info "VID": 10C4
- Board info "PID": EA60
- "esptool.py" chip identification: ESP32D0WDQ6 (revision 1)

# Arduino IDE environment
- Operating system: Windows 10 version 1803
- Arduino IDE version: 1.8.9 (latest as of 2019-03-21)
- Arduino IDE boards manager URL: https://dl.espressif.com/dl/package_esp32_index.json (as instructed by https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md)
- Arduino IDE boards manager installation: esp32 by Espressif Systems version 1.0.1 (latest as of 2019-03-21)
- "esptool.py" version: v2.6-beta1

# Arduino IDE board configuration
- Board selection: TTGO LoRa32-OLED V1
- Upload speed: 921600
- Flash frequency: 80Mhz
- Core debug level: None

# VSCode environment
- Visual Studio Code version: 1.32.3 (latest as of 2019-03-21)
- Arduino plugin vsciot-vscode.vscode-arduino version: 0.2.25 (latest as of 2019-03-21)
- VSCode global settings file "settings.json"

        {
            "arduino.defaultBaudRate": 115200,
            "arduino.path": "C:\\Program Files (x86)\\Arduino"
        }

- ".vscode/arduino.json" inside project repository

        {
            "board": "esp32:esp32:ttgo-lora32-v1",
            "configuration": "FlashFreq=80,UploadSpeed=921600,DebugLevel=none",
            "sketch": "main.ino",
            "port": "COM4"
        }

- ".vscode/c_cpp_properties.json" inside project repository

        {
            "configurations": [
                {
                    "name": "Win32",
                    "includePath": [
                        "C:\\\\Users\\\\Houzuo Guo\\\\AppData\\\\Local\\\\Arduino15\\\\packages\\\\esp32\\\\tools\\\\**",
                        "C:\\\\Users\\\\Houzuo Guo\\\\AppData\\\\Local\\\\Arduino15\\\\packages\\\\esp32\\\\hardware\\\\esp32\\\\1.0.1\\\\**"
                    ],
                    "forcedInclude": [],
                    "intelliSenseMode": "msvc-x64",
                    "compilerPath": "/usr/bin/gcc",
                    "cStandard": "c11",
                    "cppStandard": "c++17"
                }
            ],
            "version": 4
        }

# Install up-to-date esptool for advanced diagnosis
ESP development tools, as installed via Arduino IDE Boards Manager, already comes with an `esptool.py`. 

1. Enter Administrator's PowerShell, use chocolatey package manager to install Python: `choco install python3.
2. Restart the PowerShell session, and then install Python package `esptool`: `pip install esptool`.
3. Verify the tool works with the following PowerShell command, it should successfully read WiFi MAC address: `esptool.py --port COM4 read_mac`

# Repartition to allow larger program to be uploaded
A program that attempts to scan for both WiFi and Bluetooth devices run out ot space under the default partition scheme:

"Sketch uses 1470638 bytes (112%) of program storage space. Maximum is 1310720 bytes."

There are two causes that altogether result in the out-of-space error:

1. The definition of the board contains an artificial number that limits maximum program upload size to 1280KBytes:

        (C:\Users\Houzuo Guo\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.1\boards.txt)
        ...
        ...
        ttgo-lora32-v1.upload.maximum_size=1310720
        ...
        ...

   Using a text editor I changed the value to 3145728 (3MBytes). This limit is immediately lifted, and now move on to re-partitioning the storage.

2. The default partition scheme, already baked into the micro-controller storage, offers only 1280KBytes to uploaded program, and leaves room for OTA capability which I do not need for this exercise:

        (C:\Users\Houzuo Guo\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.1\tools\partitions\default.csv)
        # Name,   Type, SubType, Offset,  Size, Flags
        nvs,      data, nvs,     0x9000,  0x5000,
        otadata,  data, ota,     0xe000,  0x2000,
        app0,     app,  ota_0,   0x10000, 0x140000,
        app1,     app,  ota_1,   0x150000,0x140000,
        eeprom,   data, 0x99,    0x290000,0x1000,
        spiffs,   data, spiffs,  0x291000,0x16F000,

   Using a text editor I changed the content of "default.csv" to match the "huge_app.cvs" reference that removes OTA capability, allocates 3MBytes of capacity to program partition, leaves 1KBytes to EEPROM and 1MBytes to SPIFFS ():

        (C:\Users\Houzuo Guo\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.1\tools\partitions\default.csv)
        # Name,   Type, SubType, Offset,  Size, Flags
        nvs,      data, nvs,     0x9000,  0x5000,
        otadata,  data, ota,     0xe000,  0x2000,
        app0,     app,  ota_0,   0x10000, 0x300000,
        eeprom,   data, 0x99,    0x310000,0x1000,
        spiffs,   data, spiffs,  0x311000,0xEF000,

   The new partition scheme will be written into the micro-controller storage automatically upon next successful upload of a program.

# Install software library for programming on-board SSD1306 OLED
Use Arduino IDE Library Manager to install "ESP8266 and ESP32 Oled Driver for SSD1306 display by Daniel Eichhorn, Fabrice Weinberg Version 4.0.0", the library provides OLED drawing and text displaying capabilities via library header files such as `SSD1306.h` and `OLEDDisplayUi.h`.

An alternative library called "Adafruit_SSD1306" advertises that it too supports this OLED, though I have not experimented with the library and it has got different header file names too.

To help VSCode C++ to introspect the newly installed library and provide auto-completion, edit `.vscode/c_cpp_properties.json` under project repository, and add Arduino IDE library directories into `includePath`:

    {
        ...
        "configurations": [
            {
                ...
                "includePath": [
                    ...
                    "C:\\\\Users\\\\Houzuo Guo\\\\Documents\\\\Arduino\\\\libraries\\\\**"
                    ...
                ],
                ...
            }
        ],
        ...
    }

Now with WiFi, Bluetooth, and OLED libraries in the source code, the compiled program uses 1.5MBytes of program partition: "Sketch uses 1501878 bytes (47%) of program storage space. Maximum is 3145728 bytes."