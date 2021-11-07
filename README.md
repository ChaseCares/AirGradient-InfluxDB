# AirGradient Pushing to InfluxDB

![Arduino Compile CI](../../actions/workflows/arduino.yaml/badge.svg)

AirGradients sensor kit is quite good and reasonably affordable. This Arduino sketch allows you to integrated with a locally run InfluxDB instance. This was loosely based off of AirGradient original example with everything that wasn't required for the display removed and replaced with InfluxDB. There are a couple additional features which were added as well.

## Modifications

- Removed captive portal, Wi-Fi credentials are now defined below
- data is now sent to influxDB
- added the ability to display Fahrenheit on display, celsius is still reported to the database
- added a calibration offset for temperature (as Jeff Gerling noted temperatures are usually a bit high) the calibration value is subtracted from the reading
