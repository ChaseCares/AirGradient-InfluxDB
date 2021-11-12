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

// Sanity check
#if ENABLE_INFLUXDB && !ENABLE_WI_FI
	#error Wi-Fi must be enabled for InfluxDB to work properly
#endif
#if ENABLE_MQTT && !ENABLE_WI_FI
	#error Wi-Fi must be enabled for MQTT to work properly
#endif
#if !HAS_SHT && !HAS_PM2_5 && !HAS_CO2
	#error Must have at least one sensor enabled
#endif
#if !ENABLE_MQTT && !ENABLE_INFLUXDB
	#define ENABLE_WI_FI false
#endif

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

void mqttExporter();
#endif

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

unsigned long elapsedMs = 0;
const unsigned int MS = 1000;

#if ENABLE_INFLUXDB
// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient influxClient(
	INFLUXDB_URL,
	INFLUXDB_ORG,
	INFLUXDB_BUCKET,
	INFLUXDB_TOKEN,
	InfluxDbCloud2CACert
);
#endif

void writeToDatabase();
void showTextRectangle(const char* line1, const char* line2, boolean smallText);
void showTextRectangle(std::string line1, std::string line2, boolean smallText);
void showTextRectangle(String line1, String line2, boolean smallText);
template <typename... Args>
std::string toStr(Args &&...args);

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
Cache<TMP_RH> tempHumCache([]() { return ag.periodicFetchData(); }, 100);
#endif
#if HAS_PM2_5
Cache<int> pm2_5Cache([]() { return ag.getPM2_Raw(); }, 100);
#endif
#if HAS_CO2
Cache<int> co2Cache([]() { return ag.getCO2_Raw(); }, 100);
#endif

struct Task {
	void (*m_callback)();       // Function to execute every time the timeout expires
	unsigned long m_timeout;    // How much time is left before the callback should be executed
	unsigned long m_interval;   // What to reset the timeout to once it expires
};

const char* TaskTypeNames[] = {
	"DisplayTemp",
	"DisplayHum",
	"DisplayPM2_5",
	"DisplayCO2",
	"WriteToDatabase",
	"MQTTHandler",
};

enum class TaskType {
	DisplayTemp,
	DisplayHum,
	DisplayPM2_5,
	DisplayCO2,
	WriteToDatabase,
	MQTTHandler,
};

std::map<TaskType, Task> tasks = {
#if HAS_SHT
	{
		TaskType::DisplayTemp,
		{
			[]() {
				float caliTemp = tempHumCache.getValue().t - TEMP_OFFSET;
				auto tempStr = FAHRENHEIT ? toStr(caliTemp*1.8+32.0, "°F") : toStr(caliTemp, "°C");
				showTextRectangle("TEMP", tempStr, true);
			}
		}
	},
	{
		TaskType::DisplayHum,
		{ []() { showTextRectangle("HMTY", toStr(tempHumCache.getValue().rh, "%"), true); } }
	},
#endif
#if HAS_PM2_5
	{
		TaskType::DisplayPM2_5,
		{ []() { showTextRectangle("PM2", toStr(pm2_5Cache.getValue()), false); } }
	},
#endif
#if HAS_CO2
	{
		TaskType::DisplayCO2,
		{ []() { showTextRectangle("CO2", toStr(co2Cache.getValue()), false); } }
	},
#endif
#if ENABLE_INFLUXDB
	{
		TaskType::WriteToDatabase,
		{
			writeToDatabase,
			ADD_TO_INFLUXDB_INTERVAL * MS,
			ADD_TO_INFLUXDB_INTERVAL * MS
		}
	},
#endif
#if ENABLE_MQTT
	{
		TaskType::MQTTHandler,
		{
			mqttExporter,
			ADD_TO_MQTT_INTERVAL * MS,
			ADD_TO_MQTT_INTERVAL * MS,
		}
	}
#endif
};

void setup() {
	Serial.begin(9600);

	display.init();
	display.flipScreenVertically();
	showTextRectangle("(*^_^*)", "Starting", true);

#if (HAS_PM2_5)
	ag.PMS_Init();
#endif
#if (HAS_CO2)
	ag.CO2_Init();
#endif
#if (HAS_SHT)
	ag.TMP_RH_Init(0x44);
#endif

	const unsigned int totalTasks = HAS_PM2_5 + HAS_CO2 + HAS_SHT * 2;

	size_t i = 0;
	for(auto& [taskType, task] : tasks) {
		switch(taskType) {
			case TaskType::WriteToDatabase:
			case TaskType::MQTTHandler:
				continue;
			default :
				task.m_timeout  = DISPLAY_INTERVAL * MS * i++;
				task.m_interval = DISPLAY_INTERVAL * MS * totalTasks;
		}
	}

#if ENABLE_WI_FI
	// Setup and wait for Wi-Fi
	WiFi.begin(WI_FI_SSID, WI_FI_PASSWORD);
	Serial.println("");

	showTextRectangle("Setting", "up WiFi", true);
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(WI_FI_SSID);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("MAC address: ");
	Serial.println(WiFi.macAddress());
	Serial.print("Hostname: ");
	Serial.println(String(WiFi.hostname()));
#endif

	Serial.println("");

#if ENABLE_INFLUXDB
	timeSync(TZ_INFO, NTP_SERVER);
#endif

#if ENABLE_MQTT
	// Unique ID must be set
	byte mac[WL_MAC_ADDR_LENGTH];
	WiFi.macAddress(mac);
	device.setUniqueId(mac, sizeof(mac));

	device.setName(DEVICE_NAME);

	#if HAS_PM2_5
	MQTT_PM2_5.setUnitOfMeasurement("PPM");
	MQTT_PM2_5.setDeviceClass("pm25");
	MQTT_PM2_5.setIcon("mdi:air-filter");
	MQTT_PM2_5.setName(DEVICE_NAME " Particulate");
	#endif

	#if HAS_CO2
	MQTT_CO2.setUnitOfMeasurement("µg/m3");
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

	for (auto &[taskType, task] : tasks) {
		// If it underflows around when subtracting, that means it would have been negative
		if (task.m_timeout - deltaMS > task.m_timeout) {
			Serial.print("Running task: ");
			Serial.println(TaskTypeNames[(int)taskType]);
			task.m_callback();
			// Take into consideration how late we were to prevent drift
			task.m_timeout = task.m_interval + (task.m_timeout - deltaMS);
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
	int PM2_5 = pm2_5Cache.getValue();
	sensor.addField("pm2.5", PM2_5);
	#endif
	#if HAS_CO2
	int CO2 = co2Cache.getValue();
	sensor.addField("co2", CO2);
	#endif
	#if HAS_SHT
	TMP_RH result = tempHumCache.getValue();
	float caliTemp = (result.t - TEMP_OFFSET);
	sensor.addField("humidity", result.rh);
	sensor.addField("temperature", caliTemp);
	#endif

	// Write point
	if (!influxClient.writePoint(sensor)) {
		Serial.print("InfluxDB write failed: ");
		Serial.println(influxClient.getLastErrorMessage());
	}
}
#endif

#if ENABLE_MQTT
void mqttExporter() {
	#if HAS_PM2_5
	int PM2_5 = pm2_5Cache.getValue();
	MQTT_PM2_5.setValue(PM2_5);
	#endif
	#if HAS_CO2
	int CO2 = co2Cache.getValue();
	MQTT_CO2.setValue(CO2);
	#endif
	#if HAS_SHT
	TMP_RH result = tempHumCache.getValue();
	float caliTemp = (result.t - TEMP_OFFSET);
	MQTT_temperature.setValue(caliTemp);
	MQTT_humidity.setValue(result.rh);
	#endif
}
#endif

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
