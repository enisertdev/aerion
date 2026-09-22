#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "compass.h"
#include "config.h"
#include "wifi_debug.h"

// =====================================================
// KALIBRASYON
// =====================================================

#define MIN_X -547.0
#define MAX_X  438.0

#define MIN_Y -545.0
#define MAX_Y  688.0

#define MIN_Z -572.0
#define MAX_Z  662.0

// =====================================================
// INTERNAL
// =====================================================

static int16_t magX = 0;
static int16_t magY = 0;
static int16_t magZ = 0;

static float compassHeading = 0.0;

static bool compassValid = false;

static unsigned long lastCompassUpdate = 0;

const unsigned long COMPASS_INTERVAL = 100;

// =====================================================
// INIT
// =====================================================

void initCompass()
{
    Wire.begin(
        COMPASS_SDA_PIN,
        COMPASS_SCL_PIN
    );

    delay(50);

    Wire.beginTransmission(COMPASS_ADDRESS);
    Wire.write(0x0A);
    Wire.write(0x1D);

    uint8_t result = Wire.endTransmission();

    if (result == 0)
    {
        debugLog("[COMPASS] QMC5883P started.");
    }
    else
    {
        debugLog(
            "[COMPASS] QMC5883P init failed. I2C error: " +
            String(result)
        );
    }
}

// =====================================================
// RAW READ
// =====================================================

static bool readCompass(
    int16_t &x,
    int16_t &y,
    int16_t &z
)
{
    Wire.beginTransmission(COMPASS_ADDRESS);

    Wire.write(0x01);

    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    int received = Wire.requestFrom(
        COMPASS_ADDRESS,
        (uint8_t)6
    );

    if (received != 6)
    {
        return false;
    }

    uint8_t xLow = Wire.read();
    uint8_t xHigh = Wire.read();

    uint8_t yLow = Wire.read();
    uint8_t yHigh = Wire.read();

    uint8_t zLow = Wire.read();
    uint8_t zHigh = Wire.read();

    x = (int16_t)(
        xLow | (xHigh << 8)
    );

    y = (int16_t)(
        yLow | (yHigh << 8)
    );

    z = (int16_t)(
        zLow | (zHigh << 8)
    );

    return true;
}

// =====================================================
// HEADING
// =====================================================

static float calculateHeading(
    int16_t rawX,
    int16_t rawY
)
{
    float offsetX =
        (MIN_X + MAX_X) / 2.0;

    float offsetY =
        (MIN_Y + MAX_Y) / 2.0;

    float rangeX =
        (MAX_X - MIN_X) / 2.0;

    float rangeY =
        (MAX_Y - MIN_Y) / 2.0;

    float x =
        (float)rawX - offsetX;

    float y =
        (float)rawY - offsetY;

    x /= rangeX;
    y /= rangeY;

    float heading =
        atan2(y, x) * 180.0 / PI;

    if (heading < 0)
    {
        heading += 360.0;
    }

    return heading;
}

// =====================================================
// UPDATE
// =====================================================

void updateCompass()
{
    if (
        millis() - lastCompassUpdate
        < COMPASS_INTERVAL
    )
    {
        return;
    }

    lastCompassUpdate = millis();

    compassValid =
        readCompass(
            magX,
            magY,
            magZ
        );

    if (compassValid)
    {
        compassHeading =
            calculateHeading(
                magX,
                magY
            );
    }
}

// =====================================================
// GETTERS
// =====================================================

bool isCompassValid()
{
    return compassValid;
}

float getCompassHeading()
{
    return compassHeading;
}

int16_t getCompassX()
{
    return magX;
}

int16_t getCompassY()
{
    return magY;
}

int16_t getCompassZ()
{
    return magZ;
}