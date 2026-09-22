#include <Arduino.h>
#include "telemetry.h"
#include "lora.h"
#include "gps.h"
#include "wifi_debug.h"
#include "imu.h"
#include "flight_control.h"


unsigned long lastDebugTime = 0;
uint32_t lastGPSBytes = 0;

// ============================================================
// SETUP
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    Serial.println();
    Serial.println("==============================");
    Serial.println("        UAV SYSTEM");
    Serial.println("==============================");
    initWiFi();
    initIMU();
    initGPS();
    initLoRa();
    setupESC();

    debugLog("==============================");
    debugLog("       SYSTEM READY");
    debugLog("==============================");
}

void loop()
{
    updateGPS();
    updateIMU();
    updateLoRa();
    updateWiFi();

    // ========================================================
    // DEBUG
    // ========================================================
    if (millis() - lastDebugTime >= 1000)
    {
        lastDebugTime = millis();
        // ----------------------------------------------------
        // GPS BYTE SAYACI
        // ----------------------------------------------------
        uint32_t currentBytes = getGPSCharsProcessed();
        uint32_t bytesPerSecond = currentBytes - lastGPSBytes;

        lastGPSBytes = currentBytes;

        // ----------------------------------------------------
        // IMU
        // ----------------------------------------------------
        debugLog(
            "[IMU] "
            "ACC: " +
            String(getAccelX(), 2) +
            "," +
            String(getAccelY(), 2) +
            "," +
            String(getAccelZ(), 2) +
            " | GYRO: " +
            String(getGyroX(), 2) +
            "," +
            String(getGyroY(), 2) +
            "," +
            String(getGyroZ(), 2)
        );

        // ----------------------------------------------------
        // GPS
        // ----------------------------------------------------
        String message =
            "[GPS] "
            "RX/s: " +
            String(bytesPerSecond) +
            " | TOTAL: " +
            String(currentBytes) +
            " | SAT: " +
            String(getGPSSatellites()) +
            " | VALID: " +
            String(
                isGPSValid()
                    ? "YES"
                    : "NO"
            ) +
            " | LAT: " +
            String(
                getGPSLatitude(),
                6
            ) +
            " | LON: " +
            String(
                getGPSLongitude(),
                6
            ) +
            " | ALT: " +
            String(
                getGPSAltitude(),
                1
            ) +
            "m | SPEED: " +
            String(
                getGPSSpeed(),
                1
            ) +
            "km/h | COURSE: " +
            String(
                getGPSCourse(),
                1
            ) +
            "deg | HDOP: " +
            String(
                getGPSHDOP(),
                2
            );
        debugLog(message);
    }
    delay(1);
} 

