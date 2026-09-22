#ifndef LORA_H
#define LORA_H

#include <Arduino.h>

extern HardwareSerial LoRaSerial;

void initLoRa();
void updateLoRa();

void writeLoRa(
    uint8_t* data,
    uint8_t length
);

#endif