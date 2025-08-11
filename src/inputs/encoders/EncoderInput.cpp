// SPDX-License-Identifier: GPL-3.0-or-later
#include "EncoderInput.h"
#include "EncoderBuffer.h"
#include "../../rp2040/JoystickWrapper.h"
#include "../../Config.h"
#include "RotaryEncoder.h"
#include "../shift_register/ShiftRegister165.h"
#include "../../config/PoolConfig.h"

// External variable for matrix pin states
extern bool g_encoderMatrixPinStates[20];
// Add shift register buffer access
extern uint8_t* shiftRegBuffer;
extern ShiftRegister165* shiftReg;

// Helper to get pin state for encoder (matrix-aware and shift-register-aware)
static int encoderReadPin(uint8_t pin) {
    // For shift register encoders, pin encodes reg and bit: (reg << 4) | bit
    if (pin >= 100) {  // Use pin >= 100 to indicate shift register
        // DO NOT read shift register here!
        uint8_t reg = (pin - 100) >> 4;
        uint8_t bit = (pin - 100) & 0x0F;
        if (shiftRegBuffer && reg < SHIFTREG_COUNT && bit < 8) {
            return ((shiftRegBuffer[reg] >> bit) & 1) ? 0 : 1;  // Invert for 74HC165
        }
        return 1;  // Default HIGH
    }
    
    // Check if this pin is configured as a matrix pin or direct pin
    bool isMatrixPin = false;
    for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
        HardwarePinName pinName = hardwarePinMap[i].name;
        if (atoi(pinName) == pin) {
            PinType type = hardwarePinMap[i].type;
            if (type == BTN_ROW || type == BTN_COL) {
                isMatrixPin = true;
            }
            break;
        }
    }
    
    if (isMatrixPin) {
        // Use matrix state for matrix pins
        return g_encoderMatrixPinStates[pin];
    } else {
        // Use direct read for direct pins
        return digitalRead(pin);
    }
}



// Unified encoder system with static pools
static RotaryEncoder* encoders[MAX_ENCODERS];
static EncoderButtons encoderBtnMap[MAX_ENCODERS];
static int lastPositions[MAX_ENCODERS];
static uint8_t encoderTotal = 0;

void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count) {
    if (count > MAX_ENCODERS) count = MAX_ENCODERS;
    encoderTotal = count;
    initEncoderBuffers();
    for (uint8_t i = 0; i < count; i++) {
        RotaryEncoder::LatchMode latchMode;
        switch (pins[i].latchMode) {
            case FOUR3: latchMode = RotaryEncoder::LatchMode::FOUR3; break;
            case FOUR0: latchMode = RotaryEncoder::LatchMode::FOUR0; break;
            case TWO03: latchMode = RotaryEncoder::LatchMode::TWO03; break;
            default: latchMode = RotaryEncoder::LatchMode::FOUR3; break;
        }
        encoders[i] = new RotaryEncoder(pins[i].pinA, pins[i].pinB, latchMode, encoderReadPin);
        if (pins[i].pinA < 100 && pins[i].pinB < 100) {
            pinMode(pins[i].pinA, INPUT_PULLUP);
            pinMode(pins[i].pinB, INPUT_PULLUP);
        }
        encoderBtnMap[i] = buttons[i];
        lastPositions[i] = encoders[i]->getPosition();
        createEncoderBufferEntry(buttons[i].cw, buttons[i].ccw);
    }
}






void updateEncoders() {
    // Handle all encoders with RotaryEncoder library
    for (uint8_t i = 0; i < encoderTotal; i++) {
        // Call tick() multiple times to catch up on missed transitions
        for (uint8_t t = 0; t < 3; ++t) {
            encoders[i]->tick();
        }
        
        int newPos = encoders[i]->getPosition();
        int diff = newPos - lastPositions[i];
        
        if (diff != 0) {
          uint8_t btnCW = encoderBtnMap[i].cw;
          uint8_t btnCCW = encoderBtnMap[i].ccw;

          // Handle multiple steps for fast rotation
          uint8_t steps = abs(diff);
          uint8_t btn = (diff > 0) ? btnCW : btnCCW;
          

          
          // Add to timing buffer
          addEncoderSteps(btn, steps);
          
          lastPositions[i] = newPos;
        }
    }
    
    // Process timing buffers for consistent intervals
    processEncoderBuffers();
}

void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    uint8_t encoderCount = 0;
    for (uint8_t i = 0; i < logicalCount - 1; ++i) {
        // Check for any encoder pairs (direct pin, matrix, or shift register)
        if (((logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) ||
             (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A) ||
             (logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A)) &&
            ((logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) ||
             (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B) ||
             (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B))) {
            encoderCount++;
        }
    }
    if (encoderCount > MAX_ENCODERS) encoderCount = MAX_ENCODERS;
    if (encoderCount > 0) {
        EncoderPins pinsLocal[MAX_ENCODERS];
        EncoderButtons buttonsLocal[MAX_ENCODERS];
        uint8_t idx = 0;
        for (uint8_t i = 0; i < logicalCount - 1 && idx < encoderCount; ++i) {
            // Check for any encoder pairs
            if (((logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) ||
                 (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A) ||
                 (logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A)) &&
                ((logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) ||
                 (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B) ||
                 (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B))) {
                
                bool isEncA = false, isEncB = false;
                uint8_t pinA = 0, pinB = 0;
                uint8_t joyA = 0, joyB = 0;
                
                // Get ENC_A info
                if (logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) {
                    isEncA = true;
                    pinA = logicals[i].u.pin.pin;
                    joyA = logicals[i].u.pin.joyButtonID;
                } else if (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A) {
                    isEncA = true;
                    // Find the actual pin for this matrix position
                    uint8_t rowIdx = 0;
                    for (uint8_t j = 0; j < hardwarePinMapCount; ++j) {
                        HardwarePinName pinName = hardwarePinMap[j].name;
                        if (getPinType(pinName) == BTN_ROW) {
                            if (rowIdx == logicals[i].u.matrix.row) {
                                pinA = atoi(pinName); // use Arduino pin number
                                break;
                            }
                            rowIdx++;
                        }
                    }
                    joyA = logicals[i].u.matrix.joyButtonID;
                } else if (logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A) {
                    isEncA = true;
                    // Encode shift register info in pin number: (reg << 4) | bit
                    pinA = 100 + (logicals[i].u.shiftreg.regIndex << 4) + logicals[i].u.shiftreg.bitIndex;
                    joyA = logicals[i].u.shiftreg.joyButtonID;
                }
                
                // Get ENC_B info
                if (logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) {
                    isEncB = true;
                    pinB = logicals[i + 1].u.pin.pin;
                    joyB = logicals[i + 1].u.pin.joyButtonID;
                } else if (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B) {
                    isEncB = true;
                    uint8_t rowIdx = 0;
                    for (uint8_t j = 0; j < hardwarePinMapCount; ++j) {
                        HardwarePinName pinName = hardwarePinMap[j].name;
                        if (getPinType(pinName) == BTN_ROW) {
                            if (rowIdx == logicals[i + 1].u.matrix.row) {
                                pinB = atoi(pinName); // use Arduino pin number
                                break;
                            }
                            rowIdx++;
                        }
                    }
                    joyB = logicals[i + 1].u.matrix.joyButtonID;
                } else if (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B) {
                    isEncB = true;
                    // Encode shift register info in pin number: (reg << 4) | bit
                    pinB = 100 + (logicals[i + 1].u.shiftreg.regIndex << 4) + logicals[i + 1].u.shiftreg.bitIndex;
                    joyB = logicals[i + 1].u.shiftreg.joyButtonID;
                }
                
                if (isEncA && isEncB) {
                    pinsLocal[idx].pinA = pinA;
                    pinsLocal[idx].pinB = pinB;
                    LatchMode latchMode = logicals[i].encoderLatchMode;
                    pinsLocal[idx].latchMode = latchMode;
                    buttonsLocal[idx].cw = joyA;
                    buttonsLocal[idx].ccw = joyB;
                    idx++;
                }
            }
        }
        initEncoders(pinsLocal, buttonsLocal, encoderCount);
    }
}