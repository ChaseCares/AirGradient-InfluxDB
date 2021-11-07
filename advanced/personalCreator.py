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

DELAY = 'DELAY = 6000'

FAHRRENHEIT = 'FAHRRENHEIT false'

TEMP_OFFSET = 'TEMP_OFFSET = 2.0'

TZ_INFO = 'EST+5EDT,M3.2.0/2,M11.1.0/2'
NTP_SERVER = 'time.nis.gov'

def main():

    if not os.path.exists(PERSONAL_SKETCH):
        os.mkdir(PERSONAL_SKETCH)

    config = ConfigParser()
    config.read('./Advanced/config.ini')

    with open(SKETCH_PATH, 'r') as s:
        s = s.read()

        s = s.replace(DB_URL, config['personal']['INFLUXDB_URL'])
        s = s.replace(DB_TOKEN, config['personal']['INFLUXDB_TOKEN'])
        s = s.replace(DB_ORG, config['personal']['INFLUXDB_ORG'])
        s = s.replace(DB_BUCKET, config['personal']['INFLUXDB_BUCKET'])

        s = s.replace(DEVICE_NAME, config['personal']['DEVICE_NAME'])

        s = s.replace(WIFI_SSID, config['personal']['Wi-Fi_SSID'])
        s = s.replace(WIFI_PASSWORD, config['personal']['Wi-Fi_PASSWORD'])

        s = s.replace(PM, f"HAS_PM {config['personal']['HAS_PM']}")
        s = s.replace(CO2, f"HAS_CO2 {config['personal']['HAS_CO2']}")
        s = s.replace(SHT, f"HAS_SHT {config['personal']['HAS_SHT']}")

        s = s.replace(DELAY, f"DELAY = {config['personal']['DELAY']}")

        s = s.replace(FAHRRENHEIT, f"FAHRRENHEIT {config['personal']['FAHRRENHEIT']}")

        s = s.replace(TEMP_OFFSET, f"TEMP_OFFSET = {config['personal']['TEMP_OFFSET']}")

        s = s.replace(TZ_INFO, config['personal']['TZ_INFO'])
        s = s.replace(NTP_SERVER, config['personal']['NTP_SERVER'])

        with open(f'./{PERSONAL_SKETCH}/{PERSONAL_SKETCH}.ino', 'w') as personal:
            personal.write(s)

if __name__ == '__main__':
    main()
