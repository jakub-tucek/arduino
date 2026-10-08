#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Device configuration - Copy this file to config.h in this folder and fill in your values
const char* DEVICE_NAME = "living-room";
// Empty means use the ESP32 MAC address. Keep this stable across renames.
const char* DEVICE_ID = "";
const char* API_ENDPOINT = "https://sensors.example.com/api/readings";
// For HTTPS, paste the endpoint's trusted root CA PEM here. Never skip validation.
const char* API_ROOT_CA = "";

// WiFi credentials
const char* WIFI_SSID = "your-wifi-ssid";
const char* WIFI_PASSWORD = "your-wifi-password";

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

    constexpr SensorConfig(const char* sensorId, uint8_t sensorAddress,
                           int8_t csb = -1, int8_t sdo = -1)
        : id(sensorId), address(sensorAddress), csbPin(csb), sdoPin(sdo) {}
};

const SensorConfig SENSORS[] = {
    {"room", 0x76, 25, 26},  // CSB -> GPIO 25, SDO -> GPIO 26.
    // {"outside", 0x77, 27, 33},  // Second sensor: CSB -> 27, SDO -> 33.
};

#endif
