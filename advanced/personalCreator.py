from configparser import ConfigParser
import os

SKETCH_PATH = './C02_PM_SHT_OLED_WIFI_INFLUXDBV2/C02_PM_SHT_OLED_WIFI_INFLUXDBV2.ino'
PERSONAL_SKETCH = './personal'

DB_URL = 'REPLACE_WITH_YOUR_INFLUXDB_URL_OR_IP'
DB_TOKEN = 'REPLACE_WITH_YOUR_TOKEN'
DB_ORG = 'REPLACE_WITH_YOUR_ORGANIZATION_ID'
DB_BUCKET = 'REPLACE_WITH_YOUR_BUCKET'

DEVICE_NAME = 'REPLACE_WITH_YOUR_DEVICE_NAME'
WIFI_SSID = 'REPLACE_WITH_YOUR_WI-FI_SSID'
WIFI_PASSWORD = 'REPLACE_WITH_YOUR_WI-FI_PASSWORD'

PM = 'HAS_PM true'
CO2 = 'HAS_CO2 true'
SHT = 'HAS_SHT true'

ADD_TO_DB = '= 20'
DISPLAY_INTERVAL = '= 30'

DATABASE_EXPORT = 'DATABASE_EXPORT true'

FAHRENHEIT = 'FAHRENHEIT false'

TEMP_OFFSET = 'TEMP_OFFSET = 2.0'

TZ_INFO = 'EST+5EDT,M3.2.0/2,M11.1.0/2'
NTP_SERVER = 'time.nis.gov'

def main():

    info = 'personal'

    if not os.path.exists(PERSONAL_SKETCH):
        os.mkdir(PERSONAL_SKETCH)

    c = ConfigParser()
    c.read('./Advanced/config.ini')

    with open(SKETCH_PATH, 'r') as s:
        s = s.read()

        s = s.replace(DB_URL, c[info]['INFLUXDB_URL'])
        s = s.replace(DB_TOKEN, c[info]['INFLUXDB_TOKEN'])
        s = s.replace(DB_ORG, c[info]['INFLUXDB_ORG'])
        s = s.replace(DB_BUCKET, c[info]['INFLUXDB_BUCKET'])

        s = s.replace(DEVICE_NAME, c[info]['DEVICE_NAME'])

        s = s.replace(WIFI_SSID, c[info]['Wi-Fi_SSID'])
        s = s.replace(WIFI_PASSWORD, c[info]['Wi-Fi_PASSWORD'])

        s = s.replace(PM, f"HAS_PM {c[info]['HAS_PM']}")
        s = s.replace(CO2, f"HAS_CO2 {c[info]['HAS_CO2']}")
        s = s.replace(SHT, f"HAS_SHT {c[info]['HAS_SHT']}")

        s = s.replace(ADD_TO_DB, f"= {c[info]['ADD_TO_DB']}")
        s = s.replace(DISPLAY_INTERVAL, f"= {c[info]['DISPLAY_INTERVAL']}")
        s = s.replace(DATABASE_EXPORT, f"DATABASE_EXPORT {c[info]['DATABASE_EXPORT']}")


        s = s.replace(FAHRENHEIT, f"FAHRENHEIT {c[info]['FAHRENHEIT']}")

        s = s.replace(TEMP_OFFSET, f"TEMP_OFFSET = {c[info]['TEMP_OFFSET']}")

        s = s.replace(TZ_INFO, c[info]['TZ_INFO'])
        s = s.replace(NTP_SERVER, c[info]['NTP_SERVER'])

        with open(f'./{PERSONAL_SKETCH}/{PERSONAL_SKETCH}.ino', 'w') as personal:
            personal.write(s)

if __name__ == '__main__':
    main()
