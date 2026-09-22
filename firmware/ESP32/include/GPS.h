#pragma once

#include <Arduino.h>

void initGPS();
void updateGPS();

uint8_t getGPSSatellites();

double getGPSLatitude();
double getGPSLongitude();
double getGPSAltitude();

double getGPSSpeed();
double getGPSCourse();
double getGPSHDOP();

uint16_t getGPSYear();
uint8_t getGPSMonth();
uint8_t getGPSDay();

uint8_t getGPSHour();
uint8_t getGPSMinute();
uint8_t getGPSSecond();

bool isGPSValid();
bool isGPSSpeedValid();
bool isGPSCourseValid();
bool isGPSAltitudeValid();
uint32_t getGPSCharsProcessed();