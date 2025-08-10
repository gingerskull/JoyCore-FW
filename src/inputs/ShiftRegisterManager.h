// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "shift_register/ShiftRegister165.h"

class ShiftRegisterManager {
public:
    void begin(ShiftRegister165* reg, uint8_t* buffer, uint8_t count) {
        _reg = reg; _buffer = buffer; _count = count; _lastRead = 0;
    }
    void update(uint32_t now) {
        if (!_reg || !_buffer) return;
        uint32_t interval = (_count > 1) ? 5 : 1; // ms cadence
        if (now - _lastRead >= interval) { _reg->read(_buffer); _lastRead = now; }
    }
    uint8_t* getBuffer() const { return _buffer; }
private:
    ShiftRegister165* _reg = nullptr;
    uint8_t* _buffer = nullptr;
    uint8_t _count = 0;
    uint32_t _lastRead = 0;
};

extern ShiftRegisterManager g_shiftRegisterManager;
