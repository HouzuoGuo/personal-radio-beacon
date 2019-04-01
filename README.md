# HZGL-PRB - Personal radio beacon (WiFi + Bluetooth + LoRA) for microcontroller boards
This is an adventure in learning to program microcontroller boards and on-board radio - WiFi, Bluetooth, and LoRA.

Check out various experiments and milestones in the sub-directories.

Stay tuned for more updates to come!

# Hardware specification
- Hardware unit name: TTGO T-Beam ESP32 433/868/915Mhz WiFi wireless Bluetooth Module ESP 32 GPS NEO-6M SMA LORA 32 18650 Battery holder with OLED (Variant: 868Mhz with OLED display)
- Date of purchase: 2019-03-14
- Date of delivery: 2019-03-31
- Board info "VID": 10C4
- Board info "PID": EA60
- "esptool.py" chip identification: ESP32D0WDQ6 (revision 1)

# OLED configuration
The sellor offers a small OLED (Driver IC: SSD1306) with the development board, I soldered it onto the board by connecting the OLED VCC, GND, SCL, and SDA pins to the board's 3V3, GND, 22, and 21 pins (on the same side as LoRA antenna).


# Arduino IDE board configuration
- Board selection: T-Beam
- Upload speed: 921600
- Flash frequency: 80Mhz
- Core debug level: None

# Repartition microcontroller storage to allow uploading a larger program
A program that attempts to scan for both WiFi and Bluetooth devices run out ot space under the default partition scheme:

"Sketch uses 1470638 bytes (112%) of program storage space. Maximum is 1310720 bytes."

There are two causes that altogether result in the out-of-space error:

1. The definition of the board contains an artificial number that limits maximum program upload size to 1280KBytes:

        (C:\Users\Houzuo Guo\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.1\boards.txt)
        ...
        ...
        t-beam.upload.maximum_size=1310720
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