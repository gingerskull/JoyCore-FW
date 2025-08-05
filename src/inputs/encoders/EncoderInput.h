// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "../../Config.h"

/**
 * @brief Pin configuration for a rotary encoder
 */
struct EncoderPins {
    uint8_t pinA;
    uint8_t pinB;
    LatchMode latchMode;
};

/**
 * @brief Button mapping for encoder directions
 */
struct EncoderButtons {
    uint8_t cw;   // Clockwise button ID
    uint8_t ccw;  // Counter-clockwise button ID
};

/**
 * @brief Initialize encoders with pin and button configurations
 */
void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count);

/**
 * @brief Initialize encoders from logical input configuration
 */
void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount);

/**
 * @brief Update all encoder states and send joystick events
 */
void updateEncoders(); 