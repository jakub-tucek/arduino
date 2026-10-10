#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "trusted_ca.h"

#ifndef SENSOR_API_ENDPOINT
#define SENSOR_API_ENDPOINT "https://sensors.example.com/api/readings"
#endif
#ifndef SENSOR_API_KEY
#define SENSOR_API_KEY ""
#endif
#ifndef SENSOR_WIFI_SSID
#define SENSOR_WIFI_SSID "your-wifi-ssid"
#endif
#ifndef SENSOR_WIFI_PASSWORD
#define SENSOR_WIFI_PASSWORD "your-wifi-password"
#endif
#ifndef SENSOR_API_ROOT_CA
#define SENSOR_API_ROOT_CA DEFAULT_API_ROOT_CA
#endif
#ifndef SENSOR_DEVICE_NAME
#define SENSOR_DEVICE_NAME "living-room"
#endif
#ifndef SENSOR_SENSOR_ID
#define SENSOR_SENSOR_ID "room"
#endif
#ifndef SENSOR_WIFI_HOSTNAME
#define SENSOR_WIFI_HOSTNAME SENSOR_SENSOR_ID
#endif
#ifndef SENSOR_LOCATION
#define SENSOR_LOCATION "home"
#endif
#ifndef SENSOR_GROUP
#define SENSOR_GROUP "room"
#endif
#ifndef SENSOR_TEMPERATURE_OFFSET_C
#define SENSOR_TEMPERATURE_OFFSET_C 0.0F
#endif

// Device configuration - Copy this file to config.h in this folder and fill in your values
const char* DEVICE_NAME = SENSOR_DEVICE_NAME;
const char* LOCATION = SENSOR_LOCATION;
const char* GROUP = SENSOR_GROUP;
// Empty means use the ESP32 MAC address. Keep this stable across renames.
const char* DEVICE_ID = "";
const char* API_ENDPOINT = SENSOR_API_ENDPOINT;
const char* API_KEY = SENSOR_API_KEY;
// Default: Let's Encrypt ISRG Root X1. Override for other certificate issuers.
const char* API_ROOT_CA = SENSOR_API_ROOT_CA;

// WiFi credentials
const char* WIFI_SSID = SENSOR_WIFI_SSID;
const char* WIFI_PASSWORD = SENSOR_WIFI_PASSWORD;
const char* WIFI_HOSTNAME = SENSOR_WIFI_HOSTNAME;

constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;
constexpr uint32_t REPORT_INTERVAL_MS = 30000;
constexpr uint16_t HTTP_TIMEOUT_MS = 5000;
const char* NTP_SERVER = "pool.ntp.org";

struct SensorConfig {
    const char* id;  // Unique within this device; keep stable across deployments.
    uint8_t address;
    int8_t csbPin;  // -1: module/hardware already holds CSB high.
    int8_t sdoPin;  // -1: module/hardware already sets the address.
    float temperatureOffsetC;

    constexpr SensorConfig(const char* sensorId, uint8_t sensorAddress,
                           int8_t csb = -1, int8_t sdo = -1, float offsetC = 0.0F)
        : id(sensorId), address(sensorAddress), csbPin(csb), sdoPin(sdo),
          temperatureOffsetC(offsetC) {}
};

const SensorConfig SENSORS[] = {
    {SENSOR_SENSOR_ID, 0x76, 25, 26, SENSOR_TEMPERATURE_OFFSET_C},
    // {"outside", 0x77, 27, 33},  // Second sensor: CSB -> 27, SDO -> 33.
};

#endif
