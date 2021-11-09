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

The original example code was modified to add additional features.
	Changes:
	- removed captive portal, Wi-Fi credentials are now defined below
	- data is can be sent to influxDB
	- added the ability to display Fahrenheit on display, celsius is still reported to the database
	- added a calibration offset for temperature (as Jeff Gerling noted temperatures are usually a bit high)
		the calibration value is subtracted from the reading
	- added the ability to update the database and the display independently
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

// time in seconds to add data to the database. must be less than 24 days
const unsigned int ADD_TO_DATABASE_INTERVAL = 20;

// time in seconds to update the display. must be less than 24 days
const unsigned int DISPLAY_INTERVAL = 30;

// write data to the database
#define DATABASE_EXPORT true

// true display is Fahrenheit on the display, false displays in Celsius
#define FAHRENHEIT false

// amount to subtract from the temperature reading, i found 2 °C to be a good sweet spot
float TEMP_OFFSET = 2.0;

// set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// examples: Pacific Time "PST8PDT", Japanesse "JST-9", Central Europe "CET-1CEST,M3.5.0,M10.5.0/3"

// currently set to Eastern standard Time
#define TZ_INFO "EST+5EDT,M3.2.0/2,M11.1.0/2"

// use the default or the IP address of your local NTP server
#define NTP_SERVER "time.nis.gov"

// ------------------------------------------end config------------------------------------------------

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <ESP8266WiFi.h>
#include <SSD1306Wire.h>
#include <AirGradient.h>
#include <iterator>
#include <Wire.h>

// data point
Point sensor(DEVICE_NAME);

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

unsigned long elapsedMs = 0;
const unsigned int MS = 1000;

void writeToDatabase();
void displayPM();
void displayCO2();
void displayTemp();
void displayHUM();

struct Task {
	void (*callback)();	// function to execute every time the timeout expires
	long timeout; // how much time is left before the callback should be executed
	unsigned long interval; // what to reset the timeout to once it expires
};

Task tasks[] = {
	#if HAS_SHT
	{displayTemp},
	{displayHUM,},
	#endif

	#if HAS_PM
	{displayPM,},
	#endif

	#if HAS_CO2
	{displayCO2,},
	#endif

	#if DATABASE_EXPORT
	{writeToDatabase,}
	#endif
};


void setup() {
	Serial.begin(9600);

	display.init();
	display.flipScreenVertically();
	showTextRectangle("(*^_^*)", "Starting", true);

	if (HAS_PM) ag.PMS_Init();
	if (HAS_CO2) ag.CO2_Init();
	if (HAS_SHT) ag.TMP_RH_Init(0x44);

	const int totalTasks = HAS_PM + HAS_CO2 + HAS_SHT * 2;

	for (size_t i = 0; i < totalTasks; i++) {
		tasks[i].timeout = DISPLAY_INTERVAL * MS * i;
		tasks[i].interval = DISPLAY_INTERVAL * MS * totalTasks;
	}

	if (DATABASE_EXPORT) {
		tasks[totalTasks].timeout = ADD_TO_DATABASE_INTERVAL * MS;
		tasks[totalTasks].interval = ADD_TO_DATABASE_INTERVAL * MS;
	}

	// setup and wait for Wi-Fi
	WiFi.begin(SSID, PASSWORD);
	Serial.println("");

	showTextRectangle("Setting", "up WiFi", true);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	showTextRectangle("Host-", "name:", true);
	showTextRectangle(String(WiFi.hostname().substring(0, 4)), String(WiFi.hostname().substring(4, 10)), true);

	Serial.println("");
	timeSync(TZ_INFO, NTP_SERVER);

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(SSID);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	delay(5000);
	Serial.print("MAC address: ");
	Serial.println(WiFi.macAddress());
	Serial.print("Hostname: ");
	Serial.println(String(WiFi.hostname()));
	delay(5000);

	elapsedMs = millis();
	}


void loop() {
	const unsigned long currentMillis0 = millis();
	const unsigned long deltaMs = currentMillis0 - elapsedMs;
	elapsedMs = currentMillis0;

	for (size_t i = 0; i < std::size(tasks); i++) {
		tasks[i].timeout -= deltaMs;
		if (tasks[i].timeout <= 0) {
			tasks[i].callback();
			tasks[i].timeout += tasks[i].interval;
		}
	}
}


void writeToDatabase() {
	// clear fields for reusing the point.
	sensor.clearFields();

	if (HAS_PM) {
		int PM2 = ag.getPM2_Raw();
		sensor.addField("pm02", PM2);
	}

	if (HAS_CO2) {
		int CO2 = ag.getCO2_Raw();
		sensor.addField("co2", CO2);
	}

	if (HAS_SHT) {
		delay(80)
		TMP_RH result = ag.periodicFetchData();
		float caliTemp = (result.t - TEMP_OFFSET);
		sensor.addField("humidity", result.rh);
		sensor.addField("temperature", caliTemp);
	}

	// write point
	if (!client.writePoint(sensor)) {
		Serial.print("InfluxDB write failed: ");
		Serial.println(client.getLastErrorMessage());
	}
}


void displayPM() {
	int PM2 = ag.getPM2_Raw();
	showTextRectangle("PM2", String(PM2), false);
}


void displayCO2() {
	int CO2 = ag.getCO2_Raw();
	showTextRectangle("CO2", String(CO2), false);
}


void displayTemp() {
	TMP_RH result = ag.periodicFetchData();
	float caliTemp = (result.t - TEMP_OFFSET);
	if (FAHRENHEIT) {
			showTextRectangle("TEMP", String((caliTemp * 1.8) + 32.0, 1) + "°F", true);
	} else {
			showTextRectangle("TEMP", String(caliTemp, 1) + "°C", true);
	}
}


void displayHUM() {
	TMP_RH result = ag.periodicFetchData();
	showTextRectangle("HMTY", String(result.rh) + "%", false);
}


// display
void showTextRectangle(String LN1, String LN2, boolean SMALL_TXT) {
	display.clear();
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	if (SMALL_TXT) {
	display.setFont(ArialMT_Plain_16);
	} else {
	display.setFont(ArialMT_Plain_24);
	}

	display.drawString(32, 16, LN1);
	display.drawString(32, 36, LN2);
	display.display();
}
