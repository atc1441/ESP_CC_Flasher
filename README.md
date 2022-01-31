# ESP CC flasher
This software brings you the possibility to Read and Write the internal Flash of the Texas Instruments CC 8051 series with an ESP32 using the CC interface.

The following CC Microcontroller are supported right now: CC1110, CC2430, CC2431, CC2510 and CC2511

### You can support my work via PayPal: https://paypal.me/hoverboard1 this keeps projects like this coming.

To flash an CC Microcontroller connect the following pins:
CC-DebugData to ESP-23
CC-DebugClock to ESP-19
CC-Reset to ESP-33
GND to GND
3.3V to 3.3V
Or take a look at this basic schematic: 
https://github.com/atc1441/ESP_CC_Flasher/blob/main/esp32_cc_flasher_connections.jpg

The PCB Project can be found here: https://easyeda.com/lolerino/epd_esp32

This repo is made together with this explanation video:(click on it)


[![YoutubeVideo](https://img.youtube.com/vi/1mIrL0A4vOM/0.jpg)](https://www.youtube.com/watch?v=1mIrL0A4vOM)




### Needed Software

- Visual Studio Code https://code.visualstudio.com/
- Platform IO https://platformio.org/
- ESP32 core Library https://github.com/espressif/arduino-esp32
- This version of the WifiManager https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver
- AsyncTCP https://github.com/me-no-dev/AsyncTCP
- ESPAsyncWebServer https://github.com/me-no-dev/ESPAsyncWebServer


### HowTo:

Install Visual Studio Code and PlatformIO
Download this repo and open it in Visual Studio Code

In the Project:
Select the correct COM port of your ESP32
Enter your Wifi Credentials or leave it on Managed and Connect to the Created Wifi Netowrk after Upload to config the Wifi

Connect to http://cc.local/edit and upload the index.htm file to the esp32, open http://cc.local and start using it :)


