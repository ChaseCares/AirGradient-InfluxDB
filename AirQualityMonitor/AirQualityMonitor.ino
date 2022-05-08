/*
For build instructions please visit https://www.airgradient.com/diy/

Please install ESP8266 board manager (tested with version 3.0.0)

The following libraries installed:
"ESP8266 and ESP32 OLED driver for SSD1306 displays" by ThingPulse, Fabrice Weinberg (tested with Version 4.2.1)
"ESP8266 Influxdb" by Tobias Schürg, InfluxData (tested with Version 3.9.0)
"arduino-home-assistant" by Dawid Chyrzynski (tested with Version 1.3.0)


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

#include <functional>
#include <optional>
#include <sstream>
#include <iterator>
#include <map>

#include <SSD1306Wire.h>
#include <AirGradient.h>
#include <Wire.h>

#include "DeviceConfig.hpp"

#if ENABLE_WI_FI
	#include <ESP8266WiFi.h>
#endif

#if ENABLE_INFLUXDB
	#include <InfluxDbClient.h>
	#include <InfluxDbCloud.h>
Point sensor(DEVICE_NAME);
#endif


#if ENABLE_MQTT
	#include <ArduinoHA.h>

HADevice device;

WiFiClient client;

HAMqtt mqtt(client, device);
	#if HAS_PM2_5
HASensor MQTT_PM2_5("PM2_5");
	#endif
	#if HAS_CO2
HASensor MQTT_CO2("CO2");
	#endif
	#if HAS_SHT
HASensor MQTT_temperature("Temperature");
HASensor MQTT_humidity("Humidity");
	#endif

void publishToMQTT();
#endif

AirGradient ag = AirGradient();

#if ENABLE_DISPLAY
SSD1306Wire display(0x3c, SDA, SCL);
#endif

unsigned long elapsedMs = 0;
const unsigned int MS = 1000;

#if ENABLE_INFLUXDB
// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient influxClient(
	INFLUXDB_URL,
	INFLUXDB_ORG,
	INFLUXDB_BUCKET,
	INFLUXDB_TOKEN
);
#endif

void writeToDatabase();
#if ENABLE_DISPLAY
void showTextRectangle(const char* line1, const char* line2, boolean smallText);
void showTextRectangle(std::string line1, std::string line2, boolean smallText);
void showTextRectangle(String line1, String line2, boolean smallText);
template <typename... Args>
std::string toStr(Args &&...args);
#endif

template <class T>
class Cache {
	private:
		std::function<T ()> m_source; // Function to call when cached value is out of date
		std::optional<T> m_value;
		unsigned long m_valueAge;     // How long ago m_source() was last queried for a new value
		unsigned long m_lastCallTime; // When the getValue() function was last called
		unsigned long m_ttl;          // How many ms out of date the source is valid for

	public:
		Cache<T>(std::function<T ()> source, unsigned long ttl);
		T getValue();
};

template <class T>
Cache<T>::Cache(std::function<T ()> source, unsigned long ttl) {
	m_source = source;
	m_value = {};
	m_ttl = ttl;
}

template <class T>
T Cache<T>::getValue() {
	const unsigned long currentTime = millis();
	m_valueAge += currentTime - m_lastCallTime;
	m_lastCallTime = currentTime;

	if (!m_value || m_valueAge > m_ttl) {
		m_value = m_source();
		m_valueAge = 0;
	}

	return *m_value;
}

#if HAS_SHT
#define SHT_CACHE_TIME 1000
Cache<TMP_RH> tempHumCache([]() { return ag.periodicFetchData(); }, SHT_CACHE_TIME);
inline float getTemperature() { return tempHumCache.getValue().t; }
inline int getHumidity() { return tempHumCache.getValue().rh; }
#endif
#if HAS_PM2_5
#define PM2_5_CACHE_TIME 3000
Cache<int> pm2_5Cache([]() { return ag.getPM2_Raw(); }, PM2_5_CACHE_TIME);
inline int getPM2_5() { return pm2_5Cache.getValue(); }
#endif
#if HAS_CO2
#define CO2_CACHE_TIME 1000
Cache<int> co2Cache([]() { return ag.getCO2_Raw(); }, CO2_CACHE_TIME);
inline int getCO2() { return co2Cache.getValue(); }
#endif

struct Task {
	void (*m_callback)();     // Function to execute every time the timeout expires
	unsigned long m_timeout;  // How much time is left before the callback should be executed
	unsigned long m_interval; // What to reset the timeout to once it expires
};

const size_t TOTAL_TASKS = (HAS_PM2_5 + HAS_CO2 + HAS_SHT * 2) * ENABLE_DISPLAY + ENABLE_INFLUXDB + ENABLE_MQTT;

#if ENABLE_DISPLAY
// Slight hack to be able to compute the display timesharing at compile time.
// __COUNTER__ increments every time it's used, however it seems like other library code also
// uses it, so it doesn't start at zero, meaning we need to keep the initial value around to
// subtract with.
const int INTIAL_COUNTER_VAL = __COUNTER__ + 1;
#endif

Task tasks[TOTAL_TASKS] = {
#if HAS_SHT && ENABLE_DISPLAY
	{
		.m_callback = []() {
			float caliTemp = getTemperature() - TEMP_OFFSET;
			auto tempStr = FAHRENHEIT ? toStr(caliTemp*1.8+32.0, "°F") : toStr(caliTemp, "°C");
			showTextRectangle("TEMP", tempStr, true);
		},
		.m_timeout  = DISPLAY_INTERVAL * MS * (__COUNTER__ - INTIAL_COUNTER_VAL),
		.m_interval = DISPLAY_INTERVAL * MS * TOTAL_TASKS,
	},
	{
		.m_callback = []() { showTextRectangle("HMTY", toStr(getHumidity(), "%"), true); },
		.m_timeout  = DISPLAY_INTERVAL * MS * (__COUNTER__ - INTIAL_COUNTER_VAL),
		.m_interval = DISPLAY_INTERVAL * MS * TOTAL_TASKS,
	},
#endif
#if HAS_PM2_5 && ENABLE_DISPLAY
	{
		.m_callback = []() { showTextRectangle("PM2", toStr(getPM2_5()), false); },
		.m_timeout  = DISPLAY_INTERVAL * MS * (__COUNTER__ - INTIAL_COUNTER_VAL),
		.m_interval = DISPLAY_INTERVAL * MS * TOTAL_TASKS,
	},
#endif
#if HAS_CO2 && ENABLE_DISPLAY
	{
		.m_callback = []() { showTextRectangle("CO2", toStr(getCO2()), false); },
		.m_timeout  = DISPLAY_INTERVAL * MS * (__COUNTER__ - INTIAL_COUNTER_VAL),
		.m_interval = DISPLAY_INTERVAL * MS * TOTAL_TASKS,
	},
#endif
#if ENABLE_INFLUXDB
	{
		.m_callback = writeToDatabase,
		.m_timeout  = ADD_TO_INFLUXDB_INTERVAL * MS,
		.m_interval = ADD_TO_INFLUXDB_INTERVAL * MS,
	},
#endif
#if ENABLE_MQTT
	{
		.m_callback = publishToMQTT,
		.m_timeout  = ADD_TO_MQTT_INTERVAL * MS,
		.m_interval = ADD_TO_MQTT_INTERVAL * MS,
	}
#endif
};

void setup() {
	Serial.begin(9600);
	#if ENABLE_DISPLAY
	display.init();
	display.flipScreenVertically();
	showTextRectangle("(*^_^*)", "Starting", true);
	#endif

#if (HAS_PM2_5)
	ag.PMS_Init();
#endif
#if (HAS_CO2)
	ag.CO2_Init();
#endif
#if (HAS_SHT)
	ag.TMP_RH_Init(0x44);
#endif

#if ENABLE_WI_FI
	// Setup and wait for Wi-Fi
	WiFi.begin(WI_FI_SSID, WI_FI_PASSWORD);
	Serial.println("");

	#if ENABLE_DISPLAY
	showTextRectangle("Setting", "up WiFi", true);
	#endif
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to " WI_FI_SSID);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("MAC address: ");
	Serial.println(WiFi.macAddress());
	Serial.print("Hostname: ");
	Serial.println(String(WiFi.hostname()));
#endif

#if ENABLE_INFLUXDB
	Serial.println("");
	timeSync(TZ_INFO, NTP_SERVER);
#endif

#if ENABLE_MQTT
	// Unique ID must be set
	byte mac[WL_MAC_ADDR_LENGTH];
	WiFi.macAddress(mac);
	device.setUniqueId(mac, sizeof(mac));

	device.setName(DEVICE_NAME);

	#if HAS_PM2_5
	MQTT_PM2_5.setUnitOfMeasurement("µg/m3");
	MQTT_PM2_5.setDeviceClass("pm25");
	MQTT_PM2_5.setIcon("mdi:air-filter");
	MQTT_PM2_5.setName(DEVICE_NAME " Particulate");
	#endif

	#if HAS_CO2
	MQTT_CO2.setUnitOfMeasurement("PPM");
	MQTT_CO2.setDeviceClass("carbon_dioxide");
	MQTT_CO2.setIcon("mdi:molecule-co2");
	MQTT_CO2.setName(DEVICE_NAME " Carbon Dioxide");
	#endif

	#if HAS_SHT
	MQTT_temperature.setUnitOfMeasurement("°C");
	MQTT_temperature.setDeviceClass("temperature");
	MQTT_temperature.setIcon("mdi:thermometer");
	MQTT_temperature.setName(DEVICE_NAME " Temperature");

	MQTT_humidity.setUnitOfMeasurement("%");
	MQTT_humidity.setDeviceClass("humidity");
	MQTT_humidity.setIcon("mdi:water-percent");
	MQTT_humidity.setName(DEVICE_NAME " Humidity");
	#endif

	if (MQTT_AUTHENTICATION) {
		mqtt.begin(MQTT_ADDR, MQTT_USERNAME, MQTT_PASSWORD);
	} else {
		mqtt.begin(MQTT_ADDR);
	}
#endif

	elapsedMs = millis();
}


void loop() {
	const unsigned long currentMS = millis();
	const unsigned long deltaMS = currentMS - elapsedMs;
	elapsedMs = currentMS;

	for (auto& task : tasks) {
		// If it underflows around when subtracting, that means it would have been negative
		if (task.m_timeout - deltaMS > task.m_timeout) {
			task.m_callback();
			// Take into consideration how late we were to prevent drift
			task.m_timeout = task.m_interval + (task.m_timeout - deltaMS);
			// In the (hopefully) rare case that we were so late as to need to run it in
			// (what would have been) negative milliseconds next time, we just have it
			// run next time it would line up.
			// This unfortunately means it might not execute as many times on average
			// as you might expect given its interval, but the alternative is storing
			// an additional signed "catch up" counter and dealing with the edge cases
			// with that
			while (task.m_timeout > task.m_interval) task.m_timeout += task.m_interval;
		} else {
			task.m_timeout -= deltaMS;
		}
	}
#if ENABLE_MQTT
	mqtt.loop();
#endif
}

#if ENABLE_INFLUXDB
void writeToDatabase() {
	// Clear fields for reusing the point.
	sensor.clearFields();

	#if HAS_PM2_5
	sensor.addField("pm2.5", getPM2_5());
	#endif
	#if HAS_CO2
	sensor.addField("co2", getCO2());
	#endif
	#if HAS_SHT
	sensor.addField("humidity", getHumidity());
	sensor.addField("temperature", getTemperature() - TEMP_OFFSET);
	#endif

	// Write point
	if (!influxClient.writePoint(sensor)) {
		Serial.print("InfluxDB write failed: ");
		Serial.println(influxClient.getLastErrorMessage());
	}
}
#endif

#if ENABLE_MQTT
void publishToMQTT() {
	#if HAS_PM2_5
	int PM2_5 = getPM2_5();
	MQTT_PM2_5.setValue(PM2_5);
	#endif
	#if HAS_CO2
	int CO2 = getCO2();
	MQTT_CO2.setValue(CO2);
	#endif
	#if HAS_SHT
	MQTT_temperature.setValue(getTemperature() - TEMP_OFFSET);
	MQTT_humidity.setValue(getHumidity());
	#endif
}
#endif

#if ENABLE_DISPLAY
void showTextRectangle(String line1, String line2, boolean smallText) {
	display.clear();
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(smallText ? ArialMT_Plain_16 : ArialMT_Plain_24);
	display.drawString(32, 16, line1);
	display.drawString(32, 36, line2);
	display.display();
}

void showTextRectangle(const char *line1, const char *line2, boolean smallText) {
	showTextRectangle(String(line1), String(line2), smallText);
}

void showTextRectangle(std::string line1, std::string line2, boolean smallText) {
	showTextRectangle(String(line1.c_str()), String(line2.c_str()), smallText);
}

template <typename... Args>
std::string toStr(Args &&...args) {
	std::ostringstream ostr;
	(ostr << std::dec << ... << args);
	return ostr.str();
}
#endif
