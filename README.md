# Custom AirGradient Sketch

[![Arduino Compile CI](../../actions/workflows/Arduino_CI.yml/badge.svg)](../../actions/workflows/Arduino_CI.yml)

AirGradients sensor kit is quite good and reasonably affordable. This Arduino sketch allows you to integrate with a locally run InfluxDB instance. This was loosely based off of AirGradient original example with everything that wasn't required for the display removed and replaced with InfluxDB. There are a couple additional features which were added as well.

Note, InfluxDB isn't required there is a config option that will allow you to disable it.

## Getting Started

### Step 0 | Prerequisites

- Asseemble the AirGradient device, instructions [here](https://www.airgradient.com/diy/)
- Install Arduino, download [here](https://www.arduino.cc/en/software)
- Download this repository. Click the green Code button and then Download ZIP
- Unzip the folder, navigate to the AirQualityMonitor and copy the example config file, pasting it in the same directory with the name `DeviceConfig`

### Step 1 | Settting up Arduino

Add ESP8266 platform information

- Open the Arduino sketch file, open the preferences menu, and add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to *Additional Board Manager URLs* [Image](./Images/Preferences.png)
- Open the Board Manager, Search for and install  `esp8266`. by `ESP8266 comunity` [Image](./Images/BoardManager.png)
- Load the board preferences by navigating to Tool -> Board: -> ESP8266 Boards, then select `LOLIN(WEMOS) D1 R2 & mini` [Image](./Images/BM-D1mini.png)

Add the required libraries

- Open library manager and search for and install `AirGradient Air Quality Sensor` [Image](./Images/LM-AirGradient.png)

If only using sensors, with no connectivity skip to step two

- Search for and install `AirGradient Air Quality Sensor` by AirGradient [Image](./Images/LM-AirGradient.png)
- Search for and install `ESP8266 and ESP32 OLED driver for SSD1306 displays` by ThingPulse [Image](./Images/LM-OLED.png)

Only needed if you enable Influxdb

- Search for and install `ESP8266 Influxdb` by Tobias Schürg, InfluxData [Image](./Images/LM-Influxdb.png)

Only needed if you enable MQTT

- Search for and install `arduino-home-assistant` by Dawid Chyrzynski [Image](./Images/LM-HA.png) You will be asked if you would like to install dependencies, select *Install all* [Image](./Images/LM-Dependencies.png)

### Step 2 | Configuration

Navigate to the DeviceConfig.hpp tab and unable any features you would like to use. Then file their corresponding credentials.

Once you're done customizing, with the device you plugged into your computer, click upload. Optionally, you can open the serial monitor, to monitor what the device is doing

## Modifications

- Removed captive portal. Wi-Fi credentials are now defined in the config file
- Data can now be sent to influxDB
- Added the ability to display Fahrenheit on display, celsius is still reported to the database
- Added a calibration offset for temperature, the calibration value is subtracted from the reading
- Added the ability to update the database and display independently
- Added the ability to control the timings of all features independently
- Added MQTT, with Home Assistant autodiscovery
- Added ability to easily enable/disable Wi-Fi, MQTT and influxDB
