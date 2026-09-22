#pragma once

#include <stdint.h>

extern uint8_t pitchAngle;
extern uint8_t rollAngle;
extern uint8_t yawAngle;
extern uint8_t throttle;
extern uint8_t flightMode;

extern bool isFailsafeActive;

void executeFlightCommands();
void triggerFailsafe();
void setupESC();
