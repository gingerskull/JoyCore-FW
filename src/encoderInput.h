// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "Config.h" 

struct EncoderPins {
    uint8_t pinA;
    uint8_t pinB;
};

struct EncoderButtons {
    uint8_t cw;   // Clockwise button ID
    uint8_t ccw;  // Counter-clockwise button ID
};

void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count);
void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount);
void updateEncoders();
