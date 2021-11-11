
//  Before editing, copy this file to DeviceConfig.hpp


// Device name, if you have multiple devices this needs to be unique
#define DEVICE_NAME "REPLACE_WITH_YOUR_DEVICE_NAME"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples: Pacific Time "PST8PDT", Japanesse "JST-9", Central Europe "CET-1CEST,M3.5.0,M10.5.0/3"
// Currently set to Eastern standard Time
#define TZ_INFO "EST+5EDT,M3.2.0/2,M11.1.0/2"

// Use the default or the IP address of your local NTP server
#define NTP_SERVER "time.nis.gov"

// ------ WI-FI ------
// Enables or disables Wi-Fi
#define ENABLE_WI_FI true

// If Wi-Fi is enabled, add Wi-Fi credentials
#define WI_FI_SSID "REPLACE_WITH_YOUR_WI-FI_SSID"
#define WI_FI_PASSWORD "REPLACE_WITH_YOUR_WI-FI_PASSWORD"

// ------ InfluxDB ------

// Enables or disables InfluxDB
#define ENABLE_INFLUXDB true
// If InfluxDB is enabled, fill out information below

// Time in seconds to add data to the database
const unsigned int ADD_TO_INFLUXDB_INTERVAL = 30;

// InfluxDB v2 server url, e.g. https://influxdb.example.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "REPLACE_WITH_YOUR_INFLUXDB_URL_OR_IP"

// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "REPLACE_WITH_YOUR_TOKEN"

// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "REPLACE_WITH_YOUR_ORGANIZATION_ID"

// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "REPLACE_WITH_YOUR_BUCKET"

// ------ MQTT ------
// Enables or disables MQTT
#define ENABLE_MQTT false
// If MQTT is enabled, fill out information below

// The IP address of your MQTT broker
#define MQTT_ADDR IPAddress(127, 0, 0, 1)

// The interval in which to send MQTT data
const unsigned int ADD_TO_MQTT_INTERVAL = 30;

// Enables or disables MQTT authentication
#define MQTT_AUTHENTICATION false

// If Authentication is enabled, add MQTT credentials
#define MQTT_USERNAME "REPLACE_WITH_YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "REPLACE_WITH_YOUR_MQTT_PASSWORD"


//  ------ These values can be left alone ------

// Frequency in seconds to update the display
const unsigned int DISPLAY_INTERVAL = 30;

// Set sensors that you do not use to false
#define HAS_SHT true
#define HAS_PM2_5 true
#define HAS_CO2 true

// Set to true to use Fahrenheit on the display, false for Celsius
#define FAHRENHEIT false

// Amount to subtract from the temperature reading. I found 2 °C to be a good sweet spot
float TEMP_OFFSET = 2.0;
