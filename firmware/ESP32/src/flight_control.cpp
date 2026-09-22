#include "flight_control.h"
#include "config.h"
#include <Arduino.h>

#define ESC_PWM_PIN 27
#define ESC_PWM_CHANNEL 0
#define ESC_PWM_FREQ 50
#define ESC_PWM_RESOLUTION 16

uint8_t pitchAngle = DEFAULT_PITCH_ANGLE;
uint8_t rollAngle = DEFAULT_ROLL_ANGLE;
uint8_t yawAngle = DEFAULT_YAW_ANGLE;
uint8_t throttle = DEFAULT_THROTTLE;
uint8_t flightMode = 0;

bool isFailsafeActive = false;

void setupESC()
{
    ledcSetup(
        ESC_PWM_CHANNEL,
        ESC_PWM_FREQ,
        ESC_PWM_RESOLUTION
    );

    ledcAttachPin(
        ESC_PWM_PIN,
        ESC_PWM_CHANNEL
    );

    // ESC başlangıçta minimum throttle
    ledcWrite(
        ESC_PWM_CHANNEL,
        3277
    );
}

void executeFlightCommands()
{
    // GCS throttle: 0-255
    // ESC PWM: 1.0ms - 2.0ms

    uint32_t pwm = map(
        throttle,
        0,
        255,
        3277,
        6554
    );

    ledcWrite(
        ESC_PWM_CHANNEL,
        pwm
    );
}

void triggerFailsafe()
{
    throttle = DEFAULT_THROTTLE;
    pitchAngle = DEFAULT_PITCH_ANGLE;
    rollAngle = DEFAULT_ROLL_ANGLE;
    yawAngle = DEFAULT_YAW_ANGLE;

    isFailsafeActive = true;

    ledcWrite(
        ESC_PWM_CHANNEL,
        3277
    );
}