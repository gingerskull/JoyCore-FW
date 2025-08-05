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
    // Parallel load: latch inputs - stable timing for reliable operation
    digitalWrite(_plPin, LOW);
    delayMicroseconds(2);  // Stable timing for 74HC165
    digitalWrite(_plPin, HIGH);
    delayMicroseconds(2);  // Ensure latch completes

    // Read bits (LSB first for each byte) - stable timing
    for (uint8_t i = 0; i < _count; ++i) {
        uint8_t value = 0;
        for (uint8_t b = 0; b < 8; ++b) {
            value |= (digitalRead(_qhPin) ? 1 : 0) << b;
            digitalWrite(_clkPin, LOW);
            delayMicroseconds(1);  // Stable clock timing
            digitalWrite(_clkPin, HIGH);
            delayMicroseconds(1);  // Ensure clock completes
        }
        buffer[i] = value;
    }
}
