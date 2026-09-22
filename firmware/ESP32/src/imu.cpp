#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "imu.h"
#include "wifi_debug.h"

#define IMU_SDA 19
#define IMU_SCL 18

#define MPU9250_ADDRESS 0x68

#define PWR_MGMT_1     0x6B
#define ACCEL_CONFIG   0x1C
#define GYRO_CONFIG    0x1B
#define ACCEL_XOUT_H   0x3B
#define WHO_AM_I       0x75


// ============================================================
// RAW SENSOR VALUES
// ============================================================

float accelX = 0.0f;
float accelY = 0.0f;
float accelZ = 0.0f;

float gyroX = 0.0f;
float gyroY = 0.0f;
float gyroZ = 0.0f;


// ============================================================
// ATTITUDE
// ============================================================

float imuPitch = 0.0f;
float imuRoll  = 0.0f;


// TARE / LEVEL REFERENCE

float imuZeroPitch = 0.0f;
float imuZeroRoll  = 0.0f;


// ============================================================
// GYRO BIAS
// ============================================================

float gyroBiasX = 0.0f;
float gyroBiasY = 0.0f;
float gyroBiasZ = 0.0f;


// ============================================================
// STATE
// ============================================================

bool imuReady = false;

unsigned long lastIMUTime = 0;


// ============================================================
// FILTER
// ============================================================

const float GYRO_WEIGHT = 0.98f;
const float ACCEL_WEIGHT = 0.02f;


// ============================================================
// REGISTER WRITE
// ============================================================

void writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(MPU9250_ADDRESS);

    Wire.write(reg);
    Wire.write(value);

    uint8_t error = Wire.endTransmission();

    if (error != 0)
    {
        Serial.printf(
            "[IMU] WRITE ERROR | REG=0x%02X | ERROR=%d\n",
            reg,
            error
        );
    }
}


// ============================================================
// REGISTER READ
// ============================================================

uint8_t readRegister(uint8_t reg)
{
    Wire.beginTransmission(MPU9250_ADDRESS);

    Wire.write(reg);

    uint8_t error =
        Wire.endTransmission(false);

    if (error != 0)
    {
        Serial.printf(
            "[IMU] READ ERROR | REG=0x%02X | ERROR=%d\n",
            reg,
            error
        );

        return 0;
    }

    uint8_t received =
        Wire.requestFrom(
            MPU9250_ADDRESS,
            (uint8_t)1
        );

    if (received != 1 || !Wire.available())
    {
        return 0;
    }

    return Wire.read();
}


// ============================================================
// READ SENSOR
// ============================================================

bool readIMUSensor()
{
    Wire.beginTransmission(MPU9250_ADDRESS);

    Wire.write(ACCEL_XOUT_H);

    uint8_t error =
        Wire.endTransmission(false);

    if (error != 0)
        return false;


    uint8_t received =
        Wire.requestFrom(
            MPU9250_ADDRESS,
            (uint8_t)14
        );

    if (received != 14)
        return false;


    int16_t ax =
        (Wire.read() << 8) |
        Wire.read();

    int16_t ay =
        (Wire.read() << 8) |
        Wire.read();

    int16_t az =
        (Wire.read() << 8) |
        Wire.read();


    // Temperature
    Wire.read();
    Wire.read();


    int16_t gx =
        (Wire.read() << 8) |
        Wire.read();

    int16_t gy =
        (Wire.read() << 8) |
        Wire.read();

    int16_t gz =
        (Wire.read() << 8) |
        Wire.read();


    // ========================================================
    // ACCEL ±2G
    // ========================================================

    accelX = (float)ax / 16384.0f;
    accelY = (float)ay / 16384.0f;
    accelZ = (float)az / 16384.0f;


    // ========================================================
    // GYRO ±250 DEG/S
    // ========================================================

    gyroX = ((float)gx / 131.0f) - gyroBiasX;
    gyroY = ((float)gy / 131.0f) - gyroBiasY;
    gyroZ = ((float)gz / 131.0f) - gyroBiasZ;


    return true;
}


// ============================================================
// GYRO BIAS CALIBRATION
//
// Sadece gyro bias ölçülüyor.
// ACCELEROMETER ZERO CALIBRATION YOK.
// ============================================================

void calibrateGyroBias()
{
    const int samples = 100;

    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;

    int validSamples = 0;


    debugLog("[IMU] GYRO BIAS CALIBRATION BASLIYOR");
    debugLog("[IMU] SENSORU SABIT TUT");


    for (int i = 0; i < samples; i++)
    {
        Wire.beginTransmission(MPU9250_ADDRESS);
        Wire.write(ACCEL_XOUT_H);

        if (Wire.endTransmission(false) != 0)
        {
            delay(10);
            continue;
        }


        if (Wire.requestFrom(MPU9250_ADDRESS, (uint8_t)14) != 14)
        {
            delay(10);
            continue;
        }


        // ACC
        Wire.read();
        Wire.read();
        Wire.read();
        Wire.read();
        Wire.read();
        Wire.read();


        // TEMP
        Wire.read();
        Wire.read();


        int16_t gx =
            (Wire.read() << 8) |
            Wire.read();

        int16_t gy =
            (Wire.read() << 8) |
            Wire.read();

        int16_t gz =
            (Wire.read() << 8) |
            Wire.read();


        sumX += (float)gx / 131.0f;
        sumY += (float)gy / 131.0f;
        sumZ += (float)gz / 131.0f;

        validSamples++;

        delay(10);
    }


    if (validSamples > 0)
    {
        gyroBiasX = sumX / validSamples;
        gyroBiasY = sumY / validSamples;
        gyroBiasZ = sumZ / validSamples;
    }


    debugLog(
        "[IMU] GYRO BIAS X=" +
        String(gyroBiasX, 3) +
        " Y=" +
        String(gyroBiasY, 3) +
        " Z=" +
        String(gyroBiasZ, 3)
    );
}


// ============================================================
// INIT
// ============================================================

void initIMU()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("[IMU] BASLATILIYOR");
    Serial.println("==============================");


    Wire.begin(
        IMU_SDA,
        IMU_SCL
    );

    Wire.setClock(400000);

    delay(100);


    // ========================================================
    // I2C SCAN
    // ========================================================

    int deviceCount = 0;

    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);

        if (Wire.endTransmission() == 0)
        {
            Serial.printf(
                "[IMU] I2C CIHAZ: 0x%02X\n",
                address
            );

            deviceCount++;
        }
    }


    Serial.printf(
        "[IMU] I2C CIHAZ SAYISI: %d\n",
        deviceCount
    );


    // ========================================================
    // WHO AM I
    // ========================================================

    uint8_t whoAmI =
        readRegister(WHO_AM_I);

    Serial.printf(
        "[IMU] WHO_AM_I = 0x%02X\n",
        whoAmI
    );


    if (
        whoAmI != 0x70 &&
        whoAmI != 0x71 &&
        whoAmI != 0x73
    )
    {
        Serial.println(
            "[IMU] DESTEKLENMEYEN IMU!"
        );

        imuReady = false;

        return;
    }


    // ========================================================
    // WAKE UP
    // ========================================================

    writeRegister(
        PWR_MGMT_1,
        0x00
    );

    delay(100);


    // ========================================================
    // ACCEL ±2G
    // ========================================================

    writeRegister(
        ACCEL_CONFIG,
        0x00
    );


    // ========================================================
    // GYRO ±250 DEG/S
    // ========================================================

    writeRegister(
        GYRO_CONFIG,
        0x00
    );


    imuReady = true;


    // ========================================================
    // INITIAL GYRO BIAS
    // ========================================================

    calibrateGyroBias();


    // ========================================================
    // INITIAL ATTITUDE
    //
    // Accelerometerden ilk açı alınır.
    // ========================================================

    if (readIMUSensor())
    {
        float rawPitch =
            atan2(
                accelY,
                sqrt(
                    accelX * accelX +
                    accelZ * accelZ
                )
            )
            * 180.0f / PI;


        float rawRoll =
            atan2(
                accelX,
                accelZ
            )
            * 180.0f / PI;


        imuPitch = rawPitch;
        imuRoll = rawRoll;

        imuZeroPitch = 0.0f;
        imuZeroRoll = 0.0f;
    }


    lastIMUTime = micros();


    debugLog(
        "[IMU] BASARILI"
    );

    debugLog(
        "[IMU] INITIAL PITCH=" +
        String(imuPitch, 2) +
        " ROLL=" +
        String(imuRoll, 2)
    );


    Serial.println("==============================");
}


// ============================================================
// UPDATE
// ============================================================

void updateIMU()
{
    if (!imuReady)
        return;


    if (!readIMUSensor())
        return;


    // ========================================================
    // DT
    // ========================================================

    unsigned long now = micros();

    float dt =
        (now - lastIMUTime) / 1000000.0f;

    lastIMUTime = now;


    // Güvenlik
    if (dt <= 0.0f || dt > 0.1f)
        dt = 0.01f;


    // ========================================================
    // ACCEL ANGLE
    // ========================================================

    float accelPitch =
        atan2(
            accelY,
            sqrt(
                accelX * accelX +
                accelZ * accelZ
            )
        )
        * 180.0f / PI;


    float accelRoll =
        atan2(
            accelX,
            accelZ
        )
        * 180.0f / PI;


    // ========================================================
    // GYRO INTEGRATION
    //
    // Senin fiziksel eksenlerine göre:
    //
    // Gyro Y -> Pitch
    // Gyro X -> Roll
    // ========================================================

    float gyroPitch =
        imuPitch +
        gyroY * dt;


    float gyroRoll =
        imuRoll +
        gyroX * dt;


    // ========================================================
    // COMPLEMENTARY FILTER
    // ========================================================

    float filteredPitch =
        GYRO_WEIGHT * gyroPitch +
        ACCEL_WEIGHT * accelPitch;


    float filteredRoll =
        GYRO_WEIGHT * gyroRoll +
        ACCEL_WEIGHT * accelRoll;


    // ========================================================
    // TARE
    // ========================================================

    imuPitch =
        filteredPitch -
        imuZeroPitch;


    imuRoll =
        filteredRoll -
        imuZeroRoll;
}


// ============================================================
// IMU LEVEL / TARE CALIBRATION
//
// O ANKİ FİZİKSEL DURUM = 0°
// ============================================================

void calibrateIMULevel()
{
    if (!imuReady)
    {
        debugLog(
            "[IMU CAL] IMU HAZIR DEGIL"
        );

        return;
    }


    // Mevcut attitude'i referans yap.

    imuZeroPitch =
        imuPitch + imuZeroPitch;

    imuZeroRoll =
        imuRoll + imuZeroRoll;


    imuPitch = 0.0f;
    imuRoll = 0.0f;


    debugLog(
        "[IMU CAL] LEVEL SET"
    );

    debugLog(
        "[IMU CAL] ZERO PITCH=" +
        String(imuZeroPitch, 2) +
        " ROLL=" +
        String(imuZeroRoll, 2)
    );
}


// ============================================================
// GETTERS
// ============================================================

float getAccelX()
{
    return accelX;
}


float getAccelY()
{
    return accelY;
}


float getAccelZ()
{
    return accelZ;
}


float getGyroX()
{
    return gyroX;
}


float getGyroY()
{
    return gyroY;
}


float getGyroZ()
{
    return gyroZ;
}


float getIMUPitch()
{
    return imuPitch;
}


float getIMURoll()
{
    return imuRoll;
}


bool isIMUReady()
{
    return imuReady;
}