#include <Arduino.h>

#include "telemetry.h"
#include "gps.h"
#include "lora.h"
#include "imu.h"
#include "wifi_debug.h"
#include "config.h"


// ============================================================
// LITTLE-ENDIAN YARDIMCI FONKSİYONLAR
// ============================================================

void writeInt32LE(uint8_t* buffer, int32_t value)
{
    buffer[0] = (uint8_t)(value);
    buffer[1] = (uint8_t)(value >> 8);
    buffer[2] = (uint8_t)(value >> 16);
    buffer[3] = (uint8_t)(value >> 24);
}


void writeInt16LE(uint8_t* buffer, int16_t value)
{
    buffer[0] = (uint8_t)(value);
    buffer[1] = (uint8_t)(value >> 8);
}


void writeUInt16LE(uint8_t* buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value);
    buffer[1] = (uint8_t)(value >> 8);
}


// ============================================================
// TELEMETRY
//
// 18 BYTE
//
// [0]      HEADER 1
// [1]      HEADER 2
// [2]      SATELLITES
// [3-6]    LATITUDE   int32 LE  (degree * 1e6)
// [7-10]   LONGITUDE  int32 LE  (degree * 1e6)
// [11-12]  PITCH      int16 LE  (degree * 100)
// [13-14]  ROLL       int16 LE  (degree * 100)
// [15-16]  SEQUENCE   uint16 LE
// [17]     CHECKSUM
//
// CHECKSUM = XOR bytes 2..16
// ============================================================

void sendTelemetryToGCS()
{
    // 16-bit sequence number.
    // 0 -> 65535 -> 0
    //
    // 20 Hz'de yaklaşık 54 dakika boyunca
    // sequence tekrar etmez.
    static uint16_t sequence = 0;


    uint8_t packet[18];
    int index = 0;


    // ========================================================
    // HEADER
    // ========================================================

    packet[index++] = TELEMETRY_TX_HEADER_1;
    packet[index++] = TELEMETRY_TX_HEADER_2;


    // ========================================================
    // GPS SATELLITES
    // ========================================================

    packet[index++] = getGPSSatellites();


    // ========================================================
    // LATITUDE
    // ========================================================

    int32_t latitude =
        (int32_t)(getGPSLatitude() * 1000000.0);

    writeInt32LE(
        &packet[index],
        latitude
    );

    index += 4;


    // ========================================================
    // LONGITUDE
    // ========================================================

    int32_t longitude =
        (int32_t)(getGPSLongitude() * 1000000.0);

    writeInt32LE(
        &packet[index],
        longitude
    );

    index += 4;


    // ========================================================
    // PITCH
    // ========================================================

    int16_t pitch =
        (int16_t)(getIMUPitch() * 100.0f);

    writeInt16LE(
        &packet[index],
        pitch
    );

    index += 2;


    // ========================================================
    // ROLL
    // ========================================================

    int16_t roll =
        (int16_t)(getIMURoll() * 100.0f);

    writeInt16LE(
        &packet[index],
        roll
    );

    index += 2;


    // ========================================================
    // SEQUENCE
    // ========================================================

    writeUInt16LE(
        &packet[index],
        sequence
    );

    index += 2;

    sequence++;


    // ========================================================
    // CHECKSUM
    // ========================================================

    uint8_t checksum = 0;

    for (int i = 2; i < index; i++)
    {
        checksum ^= packet[i];
    }

    packet[index++] = checksum;


    // ========================================================
    // SEND
    // ========================================================

    writeLoRa(
        packet,
        index
    );
}