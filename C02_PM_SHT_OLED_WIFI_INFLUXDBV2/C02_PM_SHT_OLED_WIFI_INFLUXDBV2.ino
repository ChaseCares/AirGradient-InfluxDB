/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

The following libraries installed:
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg" (tested with Version 4.1.0)
"InfluxDBClient for Arduino" (tested with Version 3.9.0)

Configuration:
Please set in the code below which sensor you are using.

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

MIT License

---modifications---

The original example code was modified to report data directly to a local influxDB v2 server.
Other changes in features:
  - Removed captive portal, Wi-Fi credentials are now defined below
  - data is now sent to influxDB
  - added the ability to display Fahrenheit on display, celsius is still reported to the database ()
  - added a calibration offset for temperature (as Jeff Gerling noted temperatures are usually a bit high)
      the calibration value is subtracted from the reading
*/

// ------------------------------------------start config-----------------------------------------------

// ------ these variables require changing ------
// InfluxDB v2 server url, e.g. https://influxdb.example.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "REPLACE_WITH_YOUR_INFLUXDB_URL_OR_IP"

// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "REPLACE_WITH_YOUR_TOKEN"

// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "REPLACE_WITH_YOUR_ORGANIZATION_ID"

// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "REPLACE_WITH_YOUR_BUCKET"

// device name (_measurement), if you have multiple devices this needs to be unique
#define DEVICE_NAME "REPLACE_WITH_YOUR_DEVICE_NAME"

// Wi-Fi credentials
String SSID = "REPLACE_WITH_YOUR_WI-FI_SSID";
String PASSWORD = "REPLACE_WITH_YOUR_WI-FI_PASSWORD";


//  ------ these values can be left alone ------

// set sensors that you do not use to false
#define HAS_PM true
#define HAS_CO2 true
#define HAS_SHT true

// delay for how long each reading is displayed on the screen in milliseconds
int DELAY = 6000;

// true display is Fahrenheit on the display, false displays in Celsius
#define FAHRRENHEIT false

// an amount to subtract from the temperature reading, i found 2 °C to be a good sweet spot
float TEMP_OFFSET = 2.0;

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples: Pacific Time "PST8PDT", Japanesse "JST-9", Central Europe "CET-1CEST,M3.5.0,M10.5.0/3"

// Currently set to Eastern standard Time
#define TZ_INFO "EST+5EDT,M3.2.0/2,M11.1.0/2"

// use the default or the IP address of your local NTP server
#define NTP_SERVER "time.nis.gov"

// ------------------------------------------end config------------------------------------------------

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <ESP8266WiFi.h>
#include <SSD1306Wire.h>
#include <AirGradient.h>
#include <Wire.h>

// data point
Point sensor(DEVICE_NAME);

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

void setup() {
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("(*^_^*)", "Starting", true, 500);

  if (HAS_PM) ag.PMS_Init();
  if (HAS_CO2) ag.CO2_Init();
  if (HAS_SHT) ag.TMP_RH_Init(0x44);

  // setup and wait for Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  showTextRectangle("Setting", "up WiFi", true, 0);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  showTextRectangle("Host-", "name:", true, DELAY);
  showTextRectangle(String(WiFi.hostname().substring(0, 4)), String(WiFi.hostname().substring(4, 10)), true, DELAY);

  Serial.println("");
  timeSync(TZ_INFO, NTP_SERVER);


  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Hostname: ");
  Serial.println(String(WiFi.hostname()));
}

void loop() {
  // Clear fields for reusing the point.
  sensor.clearFields();

  if (HAS_PM) {
    int PM2 = ag.getPM2_Raw();
    sensor.addField("pm02", PM2);
    showTextRectangle("PM2", String(PM2), false, DELAY);
  }

  if (HAS_CO2) {
    int CO2 = ag.getCO2_Raw();
      sensor.addField("co2", CO2);
      showTextRectangle("CO2", String(CO2), false, DELAY);
  }

  if (HAS_SHT) {
    TMP_RH result = ag.periodicFetchData();
    float caliTemp = (result.t - TEMP_OFFSET);
    sensor.addField("humidity", result.rh);
    showTextRectangle("HMTY", String(result.rh) + "%", false, DELAY);

    sensor.addField("temperature", caliTemp);
    if (FAHRRENHEIT)
      showTextRectangle("TEMP", String((caliTemp * 1.8) + 32.0, 1) + "°F", true, DELAY);
    else
      showTextRectangle("TEMP", String(caliTemp, 1) + "°C", true, DELAY);
  }

  // write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

// display
void showTextRectangle(String LN1, String LN2, boolean SMALL_TXT, int DELAY)
{
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (SMALL_TXT)
    display.setFont(ArialMT_Plain_16);
  else
    display.setFont(ArialMT_Plain_24);

  display.drawString(32, 16, LN1);
  display.drawString(32, 36, LN2);
  display.display();
  delay(DELAY);
}
