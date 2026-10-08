#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <time.h>
#include "../config.h"

constexpr size_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static_assert(SENSOR_COUNT > 0 && SENSOR_COUNT <= 2,
              "One I2C bus supports one or two BMP280 addresses (0x76, 0x77)");
static_assert(REPORT_INTERVAL_MS > 0, "Report interval must be positive");

Adafruit_BMP280 sensors[SENSOR_COUNT];
bool sensorReady[SENSOR_COUNT] = {};
String deviceId;

bool initSensor(size_t index) {
    bool ready = sensors[index].begin(SENSORS[index].address);
    if (ready) {
        // One conversion per report; sleep between reports to reduce self-heating.
        sensors[index].setSampling(Adafruit_BMP280::MODE_FORCED);
    }
    Serial.printf("Sensor %s at 0x%02X: %s\n", SENSORS[index].id,
                  SENSORS[index].address, ready ? "ready" : "BMP280 not found");
    return ready;
}

void postReading(size_t index, float temperature, float pressure) {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    HTTPClient http;
    if (String(API_ENDPOINT).startsWith("https://")) {
        if (!API_ROOT_CA[0] || time(nullptr) < 1700000000) {
            Serial.println("HTTPS needs API_ROOT_CA and a synced clock; skipping upload");
            return;
        }
        secureClient.setCACert(API_ROOT_CA);
        if (!http.begin(secureClient, API_ENDPOINT)) {
            Serial.println("Failed to initialize HTTPS request");
            return;
        }
    } else if (!http.begin(plainClient, API_ENDPOINT)) {
        Serial.println("Failed to initialize HTTP request");
        return;
    }
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<512> doc;
    doc["device_id"] = deviceId;
    doc["device_name"] = DEVICE_NAME;
    doc["sensor_id"] = SENSORS[index].id;
    doc["sensor_type"] = "bmp280";
    doc["temperature"] = temperature;
    doc["pressure_hpa"] = pressure;
    // Preserve the original timestamp field: uptime in milliseconds, not Unix time.
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);
    int code = http.POST(payload);
    if (code >= 200 && code < 300) {
        Serial.printf("Sensor %s uploaded: HTTP %d\n", SENSORS[index].id, code);
    } else {
        Serial.printf("Sensor %s upload failed: HTTP %d\n", SENSORS[index].id, code);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Reject duplicate addresses/IDs rather than silently mixing measurements.
    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
        if (!SENSORS[i].id[0] ||
            (SENSORS[i].address != 0x76 && SENSORS[i].address != 0x77) ||
            (i > 0 && (SENSORS[i].address == SENSORS[0].address ||
                       strcmp(SENSORS[i].id, SENSORS[0].id) == 0))) {
            Serial.println("Invalid SENSORS configuration");
            while (true) {
                delay(1000);
            }
        }
    }

    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
        if (SENSORS[i].sdoPin >= 0) {
            digitalWrite(SENSORS[i].sdoPin, SENSORS[i].address == 0x77 ? HIGH : LOW);
            pinMode(SENSORS[i].sdoPin, OUTPUT);
        }
        if (SENSORS[i].csbPin >= 0) {
            digitalWrite(SENSORS[i].csbPin, HIGH);
            pinMode(SENSORS[i].csbPin, OUTPUT);
        }
    }
    delay(10);  // Let address/mode pins settle before the first I2C transaction.
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    for (uint8_t address : {0x76, 0x77}) {
        Wire.beginTransmission(address);
        Wire.write(0xD0);  // Bosch chip ID: BME280 = 0x60, BMP280 = 0x58.
        if (Wire.endTransmission() != 0) {
            Serial.printf("I2C 0x%02X: no response\n", address);
        } else if (Wire.requestFrom(address, static_cast<uint8_t>(1)) == 1) {
            Serial.printf("I2C 0x%02X: chip ID 0x%02X\n", address, Wire.read());
        } else {
            Serial.printf("I2C 0x%02X: chip ID read failed\n", address);
        }
    }
    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
        sensorReady[i] = initSensor(i);
    }

    WiFi.mode(WIFI_STA);
    deviceId = DEVICE_ID[0] ? String(DEVICE_ID) : WiFi.macAddress();
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    configTime(0, 0, NTP_SERVER);
    Serial.printf("Device %s (%s); connecting to WiFi\n", DEVICE_NAME, deviceId.c_str());
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi offline; readings will not be uploaded");
        WiFi.reconnect();
    }

    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
        if (!sensorReady[i]) {
            sensorReady[i] = initSensor(i);
            if (!sensorReady[i]) {
                continue;
            }
        }

        if (!sensors[i].takeForcedMeasurement()) {
            Serial.printf("Measurement failed for sensor %s\n", SENSORS[i].id);
            sensorReady[i] = false;
            continue;
        }
        float temperature = sensors[i].readTemperature();
        float pressure = sensors[i].readPressure() / 100.0F;
        // BMP280 operating ranges. Reject corrupted reads instead of uploading them.
        if (!isfinite(temperature) || !isfinite(pressure) ||
            temperature < -40 || temperature > 85 || pressure < 300 || pressure > 1100) {
            Serial.printf("Invalid reading from %s: %.2f°C, %.2f hPa\n",
                          SENSORS[i].id, temperature, pressure);
            sensorReady[i] = false;
            continue;
        }

        Serial.printf("%s: %.2f°C, %.2f hPa\n", SENSORS[i].id,
                      temperature, pressure);
        postReading(i, temperature, pressure);
    }
    delay(REPORT_INTERVAL_MS);
}
