# Custom AirGradient Sketch

[![Arduino Compile CI](../../actions/workflows/Arduino_CI.yml/badge.svg)](../../actions/workflows/Arduino_CI.yml)

AirGradients sensor kit is quite good and reasonably affordable. This Arduino sketch allows you to integrate with a locally run InfluxDB instance. This was loosely based off of AirGradient original example with everything that wasn't required for the display removed and replaced with InfluxDB. There are a couple additional features which were added as well.

Note, InfluxDB isn't required there is a config option that will allow you to disable it.

## Modifications

- Removed captive portal, Wi-Fi credentials are now defined below
- Data is now sent to influxDB
- Added the ability to display Fahrenheit on display, celsius is still reported to the database
- Added a calibration offset for temperature (as Jeff Gerling noted temperatures are usually a bit high) the calibration value is subtracted from the reading
- Added the ability to update the database and the display independently
- Added MQTT, with home assistant autodiscovery
- Added ability to easily enable/disable Wi-Fi, MQTT and influxDB
