#pragma once

#include <Arduino.h>

void initCompass();

void updateCompass();

bool isCompassValid();

float getCompassHeading();

int16_t getCompassX();

int16_t getCompassY();

int16_t getCompassZ();