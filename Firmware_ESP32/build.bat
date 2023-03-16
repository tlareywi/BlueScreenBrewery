@echo off 
REM This Powershell script assumes an ESP32 board. Download arduino-cli from https://arduino.github.io/arduino-cli/0.21/installation/ 
REM and place it in the Firmware_ESP32 forlder. Run this script within the folder from a Powershell prompt.

REM Be sure to edit BSB_Firmware\Config.h and set the port below prior to proceeding! 

.\arduino-cli config init
.\arduino-cli.exe core update-index --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
.\arduino-cli.exe core install esp32:esp32 --additional-urls https://dl.espressif.com/dl/package_esp32_index.json

.\arduino-cli lib install ArduinoJson
.\arduino-cli lib install MQTTPubSubClient
.\arduino-cli lib install OneWire
.\arduino-cli lib install DallasTemperature
.\arduino-cli lib install NimBLE-Arduino
.\arduino-cli lib install EspSoftwareSerial

.\arduino-cli compile --fqbn esp32:esp32:esp32 .\BSB_Firmware --verbose

REM Change the port on the line below to match the location of your board. You can find this by running `.\arduino-cli.exe board list`
REM with the board plugged in and with the board unpluggged. Use the port that appears when the board is plugged in.

.\arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32 .\BSB_Firmware