// SPDX-License-Identifier: GPL-3.0-or-later
#include "EncoderInput.h"
#include "JoystickWrapper.h"
#include "Config.h"
#include "RotaryEncoder/RotaryEncoder.h"
#include "ShiftRegister165.h"

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
        PinName pinName = hardwarePinMap[i].name;
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

// Timing buffer system for consistent press intervals
#define MAX_ENCODERS 16  // Increased to handle both direct and shift register encoders
struct EncoderBuffer {
    uint8_t buttonId;
    uint8_t pendingSteps;
    uint32_t lastPressTime;
    bool buttonPressed;
};

static EncoderBuffer encoderBuffers[MAX_ENCODERS];
static uint8_t bufferCount = 0;
static const uint32_t PRESS_INTERVAL_US = 30000;  // 30ms interval between presses
static const uint32_t PRESS_DURATION_US = 20000;  // 20ms press duration

// Unified encoder state - ALL encoders use the same RotaryEncoder class
static RotaryEncoder** encoders = nullptr;
static EncoderButtons* encoderBtnMap = nullptr;
static int* lastPositions = nullptr;
static uint8_t encoderTotal = 0;

void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count) {
  encoderTotal = count;

  encoders = new RotaryEncoder*[count];
  encoderBtnMap = new EncoderButtons[count];
  lastPositions = new int[count];

  for (uint8_t i = 0; i < count; i++) {
    // Use FOUR3 mode for single click per detent - SAME for all encoder types
    encoders[i] = new RotaryEncoder(
      pins[i].pinA, pins[i].pinB, RotaryEncoder::LatchMode::FOUR3, encoderReadPin
    );
    
    // Only set pinMode for direct pins, not shift register pins
    if (pins[i].pinA < 100 && pins[i].pinB < 100) {
        pinMode(pins[i].pinA, INPUT_PULLUP);
        pinMode(pins[i].pinB, INPUT_PULLUP);
    }
    
    encoderBtnMap[i] = buttons[i];
    lastPositions[i] = encoders[i]->getPosition();
    
    // Set up buffer entries for this encoder - SAME timing system for all
    if (bufferCount < MAX_ENCODERS - 1) {
        encoderBuffers[bufferCount].buttonId = buttons[i].cw;
        encoderBuffers[bufferCount].pendingSteps = 0;
        encoderBuffers[bufferCount].lastPressTime = 0;
        encoderBuffers[bufferCount].buttonPressed = false;
        bufferCount++;
        
        encoderBuffers[bufferCount].buttonId = buttons[i].ccw;
        encoderBuffers[bufferCount].pendingSteps = 0;
        encoderBuffers[bufferCount].lastPressTime = 0;
        encoderBuffers[bufferCount].buttonPressed = false;
        bufferCount++;
    }
  }
}

// Add steps to buffer for consistent timing
void addEncoderSteps(uint8_t buttonId, uint8_t steps) {
    for (uint8_t i = 0; i < bufferCount; i++) {
        if (encoderBuffers[i].buttonId == buttonId) {
            encoderBuffers[i].pendingSteps += steps;
            break;
        }
    }
}

// Process timing buffers for consistent intervals
void processEncoderBuffers() {
    uint32_t currentTime = micros();
    
    for (uint8_t i = 0; i < bufferCount; i++) {
        EncoderBuffer& buffer = encoderBuffers[i];
        uint8_t joyIdx = (buffer.buttonId > 0) ? (buffer.buttonId - 1) : 0;
        
        // Handle button release timing
        if (buffer.buttonPressed && (currentTime - buffer.lastPressTime >= PRESS_DURATION_US)) {
            MyJoystick.setButton(joyIdx, 0);  // Release
            buffer.buttonPressed = false;
        }
        
        // Handle new press timing
        if (buffer.pendingSteps > 0 && !buffer.buttonPressed && 
            (currentTime - buffer.lastPressTime >= PRESS_INTERVAL_US)) {
            
            MyJoystick.setButton(joyIdx, 1);  // Press
            buffer.buttonPressed = true;
            buffer.lastPressTime = currentTime;
            buffer.pendingSteps--;
        }
    }
}

void updateEncoders() {
    // For better shift register encoder timing, read the shift register multiple times
    // during the encoder update cycle, giving fresh data similar to direct pin encoders
    const uint8_t TICK_CYCLES = 8;
    
    for (uint8_t t = 0; t < TICK_CYCLES; ++t) {
        // Read shift register at the start of each tick cycle for fresh data
        if (shiftReg && shiftRegBuffer) {
            shiftReg->read(shiftRegBuffer);
        }
        
        // Tick all encoders with this fresh data
        for (uint8_t i = 0; i < encoderTotal; i++) {
            encoders[i]->tick();
        }
    }
    
    // Check for position changes after all ticking is complete
    for (uint8_t i = 0; i < encoderTotal; i++) {
        int newPos = encoders[i]->getPosition();
        int diff = newPos - lastPositions[i];

        if (diff != 0) {
          uint8_t btnCW = encoderBtnMap[i].cw;
          uint8_t btnCCW = encoderBtnMap[i].ccw;

          // Handle multiple steps for fast rotation - same timing buffer for all
          uint8_t steps = abs(diff);
          uint8_t btn = (diff > 0) ? btnCW : btnCCW;
          
          // Add to timing buffer - same system for all encoder types
          addEncoderSteps(btn, steps);
          
          lastPositions[i] = newPos;
        }
    }
    
    // Process timing buffers for consistent intervals - unified system
    processEncoderBuffers();
}

void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    // Count ALL encoder pairs - no separation between direct pin and shift register
    uint8_t totalEncoderCount = 0;
    
    for (uint8_t i = 0; i < logicalCount - 1; ++i) {
        if (((logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) ||
             (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A) ||
             (logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A)) &&
            ((logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) ||
             (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B) ||
             (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B))) {
            totalEncoderCount++;
        }
    }
    
    // Initialize ALL encoders with the same system
    if (totalEncoderCount > 0) {
        EncoderPins* pins = new EncoderPins[totalEncoderCount];
        EncoderButtons* buttons = new EncoderButtons[totalEncoderCount];
        uint8_t idx = 0;
        
        // Process all encoder pairs - direct pin, matrix, AND shift register
        for (uint8_t i = 0; i < logicalCount - 1; ++i) {
            bool isEncA = false, isEncB = false;
            uint8_t pinA = 0, pinB = 0;
            uint8_t joyA = 0, joyB = 0;
            
            // Check if we have a valid ENC_A + ENC_B pair (any type)
            if (((logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) ||
                 (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A) ||
                 (logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A)) &&
                ((logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) ||
                 (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B) ||
                 (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B))) {
                
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
                        PinName pinName = hardwarePinMap[j].name;
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
                    // Encode shift register position as pin number >= 100
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
                        PinName pinName = hardwarePinMap[j].name;
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
                    // Encode shift register position as pin number >= 100
                    pinB = 100 + (logicals[i + 1].u.shiftreg.regIndex << 4) + logicals[i + 1].u.shiftreg.bitIndex;
                    joyB = logicals[i + 1].u.shiftreg.joyButtonID;
                }
                
                if (isEncA && isEncB) {
                    pins[idx].pinA = pinA;
                    pins[idx].pinB = pinB;
                    buttons[idx].cw = joyA;
                    buttons[idx].ccw = joyB;
                    idx++;
                }
            }
        }
        
        // Initialize ALL encoders with the same system
        initEncoders(pins, buttons, totalEncoderCount);
        delete[] pins;
        delete[] buttons;
    }
}