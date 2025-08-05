// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

// User must call ShiftRegister165::begin() in setup()
class ShiftRegister165 {
public:
    // plPin: Parallel load (SH/LD), clkPin: Clock, qhPin: Serial data out
    ShiftRegister165(uint8_t plPin, uint8_t clkPin, uint8_t qhPin, uint8_t count);

    void begin();
    // Reads all bits from the shift register chain into buffer (LSB first)
    void read(uint8_t* buffer);

    uint8_t getCount() const { return _count; }

private:
    uint8_t _plPin, _clkPin, _qhPin, _count;
};
