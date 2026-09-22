#include <Arduino.h>

#include "config.h"
#include "lora.h"
#include "flight_control.h"
#include "telemetry.h"
#include "wifi_debug.h"
#include "imu.h"


HardwareSerial LoRaSerial(1);


// ============================================================
// ZAMANLAMA
// ============================================================

unsigned long lastPacketTime = 0;

// Telemetry artık bağımsız 200 ms timer ile gönderilmiyor.
// Joystick paketlerinden sonra planlanıyor.
unsigned long telemetryDueTime = 0;

bool telemetryScheduled = false;


// ============================================================
// TELEMETRY TX İSTATİSTİKLERİ
// ============================================================

unsigned long telemetrySendCount = 0;
unsigned long telemetryStatsStart = 0;


// ============================================================
// JOYSTICK SAYACI
// ============================================================

unsigned long commandReceiveCount = 0;


// ============================================================
// TELEMETRY AYARLARI
// ============================================================

// Her 2 joystick paketinde 1 telemetry
// 10 Hz joystick -> 5 Hz telemetry
const unsigned long TELEMETRY_DELAY = 30;


// ============================================================
// LORA BAŞLAT
// ============================================================

void initLoRa()
{
    LoRaSerial.begin(
        LORA_BAUD,
        SERIAL_8N1,
        LORA_RX_PIN,
        LORA_TX_PIN
    );

    lastPacketTime = millis();

    telemetryScheduled = false;

    telemetrySendCount = 0;
    telemetryStatsStart = millis();

    commandReceiveCount = 0;


    Serial.println("LORA Started.");

    Serial.printf(
        "LoRa RX: GPIO%d | TX: GPIO%d | BAUD: %d\n",
        LORA_RX_PIN,
        LORA_TX_PIN,
        LORA_BAUD
    );
}


// ============================================================
// LORA UPDATE
// ============================================================

void updateLoRa()
{
    // ========================================================
    // GCS -> ESP KOMUTLARI
    //
    // 8 BYTE
    //
    // [0] HEADER 1
    // [1] HEADER 2
    // [2] PITCH
    // [3] ROLL
    // [4] YAW
    // [5] THROTTLE
    // [6] MODE
    // [7] CHECKSUM
    // ========================================================

    while (LoRaSerial.available() >= 8)
    {
        if (LoRaSerial.read() != TELEMETRY_RX_HEADER_1)
            continue;


        if (LoRaSerial.read() != TELEMETRY_RX_HEADER_2)
            continue;


        uint8_t p = LoRaSerial.read();
        uint8_t r = LoRaSerial.read();
        uint8_t y = LoRaSerial.read();
        uint8_t t = LoRaSerial.read();
        uint8_t m = LoRaSerial.read();

        uint8_t incomingChecksum =
            LoRaSerial.read();


        uint8_t calculatedChecksum =
            p ^ r ^ y ^ t ^ m;


        if (calculatedChecksum ==
            incomingChecksum)
        {
            // =================================================
            // GEÇERLİ JOYSTICK PAKETİ
            // =================================================

            lastPacketTime = millis();

            isFailsafeActive = false;

            commandReceiveCount++;


            // =================================================
            // IMU CALIBRATE COMMAND
            // =================================================

            if (m == FLIGHT_MODE_CALIBRATE_IMU)
            {
                debugLog(
                    "[LORA RX] IMU CALIBRATE"
                );

                calibrateIMULevel();

                continue;
            }


            // =================================================
            // NORMAL FLIGHT COMMAND
            // =================================================

            pitchAngle = p;
            rollAngle = r;
            yawAngle = y;
            throttle = t;
            flightMode = m;


            debugLog(
                "[LORA RX] T=" +
                String(t) +
                " | AGE=0"
            );


            executeFlightCommands();


            // =================================================
            // TELEMETRY ZAMANLAMASI
            //
            // Her 2 joystick paketinde 1 telemetry.
            //
            // Joystick:
            //
            //   #1 -> sadece command
            //   #2 -> command + 30ms sonra telemetry
            //
            //   #3 -> sadece command
            //   #4 -> command + 30ms sonra telemetry
            //
            // Böylece:
            //
            // 10 Hz command
            //  5 Hz telemetry
            //
            // ve telemetry joystick TX'in hemen
            // üzerine bindirilmiyor.
            // =================================================

       telemetryDueTime = millis() + TELEMETRY_DELAY;
telemetryScheduled = true;
        }
    }


    // ========================================================
    // TELEMETRY
    //
    // Artık bağımsız 200 ms timer yok.
    //
    // Telemetry, ikinci joystick paketinden
    // 30 ms sonra gönderiliyor.
    // ========================================================

    if (
        telemetryScheduled &&
        (long)(millis() - telemetryDueTime) >= 0
    )
    {
        telemetryScheduled = false;

        sendTelemetryToGCS();

        telemetrySendCount++;


        // ====================================================
        // ESP GERÇEK TELEMETRY TX HIZI
        // ====================================================

        unsigned long now = millis();

        if (now - telemetryStatsStart >= 10000)
        {
            float elapsedSeconds =
                (now - telemetryStatsStart) / 1000.0f;


            float hz =
                telemetrySendCount /
                elapsedSeconds;


            debugLog(
                "[TELEMETRY TX] " +
                String(telemetrySendCount) +
                " packets | " +
                String(hz, 2) +
                " Hz"
            );


            telemetrySendCount = 0;
            telemetryStatsStart = now;
        }
    }


    // ========================================================
    // FAILSAFE
    // ========================================================

    if (millis() - lastPacketTime >
        FAILSAFE_TIMEOUT)
    {
        if (!isFailsafeActive)
        {
            debugLog(
                "[FAILSAFE] AGE=" +
                String(
                    millis() -
                    lastPacketTime
                ) +
                " ms | BEFORE=" +
                String(throttle)
            );


            triggerFailsafe();


            debugLog(
                "[FAILSAFE] AFTER=" +
                String(throttle)
            );


            executeFlightCommands();
        }
    }
}


// ============================================================
// LORA GÖNDER
// ============================================================

void writeLoRa(
    uint8_t* data,
    uint8_t length
)
{
    LoRaSerial.write(
        data,
        length
    );
}