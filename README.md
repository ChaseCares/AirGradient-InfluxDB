# Custom AirGradient Sketch

[![Arduino Compile CI](../../actions/workflows/Arduino_CI.yml/badge.svg)](../../actions/workflows/Arduino_CI.yml)

AirGradient’s sensor set up is a good base and reasonably affordable. This Arduino sketch allows you to integrate with a locally run InfluxDB instance or MQTT. Thes also allowing you to easy integrate with Home Assistant. This was loosely based off of AirGradient’s original example. A couple additional features are also included.

Note: InfluxDB isn't required, there is a config option that will allow you to disable it.

## Getting Started

### Step 0 | Prerequisites

- Assemble the AirGradient device - Instructions [here](https://www.airgradient.com/diy/)
- Install Arduino - Download [here](https://www.arduino.cc/en/software)
- Download this repository - Click the green “Code” button and then “Download ZIP”
- Unzip the folder, navigate to the AirQualityMonitor and copy the example config file. Paste it into the same directory and rename it `DeviceConfig`

### Step 1 | Settting up Arduino

**Add ESP8266 platform information.**

- Open the Arduino sketch file. Then open the Preferences menu, and add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to *Additional Board Manager URLs* [Image](./Images/Preferences.png)
- Open the Board Manager and search for and install: `esp8266` by `ESP8266 comunity` [Image](./Images/BoardManager.png)
- Load the board preferences by navigating to Tool -> Board: -> ESP8266 Boards, then select `LOLIN(WEMOS) D1 R2 & mini` [Image](./Images/BM-D1mini.png)

**Add the required libraries.**

- Open library manager, search for and install: `AirGradient Air Quality Sensor` [Image](./Images/LM-AirGradient.png)

If only using sensors, with no connectivity skip to step two

- Search for and install: `AirGradient Air Quality Sensor` by AirGradient [Image](./Images/LM-AirGradient.png)
- Search for and install: `ESP8266 and ESP32 OLED driver for SSD1306 displays` by ThingPulse [Image](./Images/LM-OLED.png)

Only needed if you enable Influxdb

- Search for and install: `ESP8266 Influxdb` by Tobias Schürg, InfluxData [Image](./Images/LM-Influxdb.png)

Only needed if you enable MQTT

- Search for and install: `home-assistant-integration` by Dawid Chyrzynski [Image](./Images/LM-HA.png) You will be asked if you would like to install dependencies, select *Install all* [Image](./Images/LM-Dependencies.png)

### Step 2 | Configuration

Navigate to DeviceConfig.hpp tab and enable any features you would like to use. Then fill out their corresponding credentials.

Once customization is complete, plug the device (esp8266) into your computer then click upload. Optionally, if you want to monitor what the device is doing, you can open Serial Monitor

## Modifications

- Removed captive portal. Wi-Fi credentials are now defined in the config file
- Data can now be sent to influxDB
- Added the ability to display Fahrenheit on display, celsius is still reported to the database
- Added a calibration offset for temperature, the calibration value is subtracted from the reading
- Added the ability to update the database and display independently
- Added the ability to control the timings of all features independently
- Added MQTT, with Home Assistant autodiscovery
- Added ability to easily enable/disable Wi-Fi, MQTT and influxDB
