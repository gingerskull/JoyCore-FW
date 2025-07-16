// SPDX-License-Identifier: GPL-3.0-or-later
#include "ShiftRegister165.h"

ShiftRegister165::ShiftRegister165(uint8_t plPin, uint8_t clkPin, uint8_t qhPin, uint8_t count)
    : _plPin(plPin), _clkPin(clkPin), _qhPin(qhPin), _count(count) {}

void ShiftRegister165::begin() {
    pinMode(_plPin, OUTPUT);
    pinMode(_clkPin, OUTPUT);
    pinMode(_qhPin, INPUT);
    digitalWrite(_plPin, HIGH);
    digitalWrite(_clkPin, HIGH);
}

void ShiftRegister165::read(uint8_t* buffer) {
    // Parallel load: latch inputs - reduced timing for faster response
    digitalWrite(_plPin, LOW);
    delayMicroseconds(1);  // Reduced from 2µs
    digitalWrite(_plPin, HIGH);
    delayMicroseconds(1);  // Reduced from 2µs

    // Read bits (LSB first for each byte) - optimized timing
    for (uint8_t i = 0; i < _count; ++i) {
        uint8_t value = 0;
        for (uint8_t b = 0; b < 8; ++b) {
            value |= (digitalRead(_qhPin) ? 1 : 0) << b;
            digitalWrite(_clkPin, LOW);
            // No delay needed - Teensy 4.0 is fast enough
            digitalWrite(_clkPin, HIGH);
            // Minimal delay only if needed
            if (b < 7) delayMicroseconds(1);  // Only between bits, not after last bit
        }
        buffer[i] = value;
    }
}
