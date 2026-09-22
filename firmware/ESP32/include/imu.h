#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

void initIMU();
void updateIMU();

float getAccelX();
float getAccelY();
float getAccelZ();

float getGyroX();
float getGyroY();
float getGyroZ();

float getIMUPitch();
float getIMURoll();

void calibrateIMULevel();

bool isIMUReady();

#endif