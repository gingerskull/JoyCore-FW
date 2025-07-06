#pragma once
#include <Arduino.h>
#include "Config.h" 

void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count);
void updateEncoders();
