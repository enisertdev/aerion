#include <Arduino.h>
#include <TinyGPSPlus.h>

#include "gps.h"
#include "config.h"

HardwareSerial GPSSerial(2);
TinyGPSPlus gps;

unsigned long lastGPSLog = 0;
unsigned long gpsBytes = 0;

void initGPS()
{
    GPSSerial.begin(
        GPS_BAUD,
        SERIAL_8N1,
        GPS_RX_PIN,
        GPS_TX_PIN
    );

    Serial.println("GPS Started.");

    Serial.printf(
        "GPS RX: GPIO%d | TX: GPIO%d | BAUD: %d\n",
        GPS_RX_PIN,
        GPS_TX_PIN,
        GPS_BAUD
    );
}

void updateGPS()
{
    while (GPSSerial.available())
    {
        char c = GPSSerial.read();

        gps.encode(c);
        gpsBytes++;
    }

    // Saniyede bir debug
    if (millis() - lastGPSLog >= 1000)
    {
        lastGPSLog = millis();

        Serial.println();
        Serial.println("=============== GPS ===============");

        Serial.printf("SAT      : %d\n", getGPSSatellites());
        Serial.printf("LAT      : %.6f\n", getGPSLatitude());
        Serial.printf("LON      : %.6f\n", getGPSLongitude());
        Serial.printf("ALT      : %.2f m\n", getGPSAltitude());
        Serial.printf("SPEED    : %.2f km/h\n", getGPSSpeed());
        Serial.printf("GPS HDG  : %.2f deg\n", getGPSCourse());
        Serial.printf("HDOP     : %.2f\n", getGPSHDOP());

        Serial.printf(
            "DATE     : %04d-%02d-%02d\n",
            getGPSYear(),
            getGPSMonth(),
            getGPSDay()
        );

        Serial.printf(
            "UTC TIME : %02d:%02d:%02d\n",
            getGPSHour(),
            getGPSMinute(),
            getGPSSecond()
        );

        Serial.printf(
            "VALID    : %s\n",
            isGPSValid() ? "YES" : "NO"
        );

        Serial.printf(
            "BYTES    : %lu\n",
            gpsBytes
        );

        Serial.printf(
            "SENTENCES: %lu\n",
            gps.sentencesWithFix()
        );

        Serial.printf(
            "CHECKSUM : %lu\n",
            gps.failedChecksum()
        );

        Serial.println("===================================");
    }
}


// ============================================================
// SATELLITES
// ============================================================

uint8_t getGPSSatellites()
{
    if (gps.satellites.isValid())
        return gps.satellites.value();

    return 0;
}


// ============================================================
// POSITION
// ============================================================

double getGPSLatitude()
{
    if (gps.location.isValid())
        return gps.location.lat();

    return 0.0;
}

double getGPSLongitude()
{
    if (gps.location.isValid())
        return gps.location.lng();

    return 0.0;
}


// ============================================================
// ALTITUDE
// ============================================================

double getGPSAltitude()
{
    if (gps.altitude.isValid())
        return gps.altitude.meters();

    return 0.0;
}


// ============================================================
// SPEED
// ============================================================

double getGPSSpeed()
{
    if (gps.speed.isValid())
        return gps.speed.kmph();

    return 0.0;
}


// ============================================================
// GPS COURSE / HEADING
// ============================================================

double getGPSCourse()
{
    if (gps.course.isValid())
        return gps.course.deg();

    return 0.0;
}


// ============================================================
// HDOP
// ============================================================

double getGPSHDOP()
{
    if (gps.hdop.isValid())
        return gps.hdop.hdop();

    return 0.0;
}


// ============================================================
// DATE
// ============================================================

uint16_t getGPSYear()
{
    if (gps.date.isValid())
        return gps.date.year();

    return 0;
}

uint8_t getGPSMonth()
{
    if (gps.date.isValid())
        return gps.date.month();

    return 0;
}

uint8_t getGPSDay()
{
    if (gps.date.isValid())
        return gps.date.day();

    return 0;
}


// ============================================================
// TIME
// ============================================================

uint8_t getGPSHour()
{
    if (gps.time.isValid())
        return gps.time.hour();

    return 0;
}

uint8_t getGPSMinute()
{
    if (gps.time.isValid())
        return gps.time.minute();

    return 0;
}

uint8_t getGPSSecond()
{
    if (gps.time.isValid())
        return gps.time.second();

    return 0;
}


// ============================================================
// VALIDITY
// ============================================================

bool isGPSValid()
{
    return gps.location.isValid();
}

bool isGPSSpeedValid()
{
    return gps.speed.isValid();
}

bool isGPSCourseValid()
{
    return gps.course.isValid();
}

bool isGPSAltitudeValid()
{
    return gps.altitude.isValid();
}
uint32_t getGPSCharsProcessed()
{
    return gps.charsProcessed();
}