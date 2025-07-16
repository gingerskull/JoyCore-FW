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
#define MAX_ENCODERS 8
struct EncoderBuffer {
    uint8_t buttonId;
    uint8_t pendingSteps;
    uint32_t lastPressTime;
    bool buttonPressed;
};

static EncoderBuffer encoderBuffers[MAX_ENCODERS];
static uint8_t bufferCount = 0;
static const uint32_t PRESS_INTERVAL_US = 8000;  // 8ms interval between presses (125 Hz)
static const uint32_t PRESS_DURATION_US = 2000;  // 2ms press duration

// Internal encoder state
static RotaryEncoder** encoders = nullptr;
static EncoderButtons* encoderBtnMap = nullptr;
static int* lastPositions = nullptr;
static uint8_t encoderTotal = 0;

// Custom encoder state for shift register encoders using library decoder
struct CustomEncoderState {
    SimpleQuadratureDecoder* decoder;
    uint8_t joyButtonCW, joyButtonCCW;
    uint8_t bufferIndexCW, bufferIndexCCW;  // Track which buffer entries to use
};

static CustomEncoderState* customEncoders = nullptr;
static uint8_t customEncoderCount = 0;

void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count) {
  encoderTotal = count;

  encoders = new RotaryEncoder*[count];
  encoderBtnMap = new EncoderButtons[count];
  lastPositions = new int[count];

  for (uint8_t i = 0; i < count; i++) {
    // Use FOUR3 mode for single click per detent
    encoders[i] = new RotaryEncoder(
      pins[i].pinA, pins[i].pinB, RotaryEncoder::LatchMode::FOUR3, encoderReadPin
    );
    pinMode(pins[i].pinA, INPUT_PULLUP);
    pinMode(pins[i].pinB, INPUT_PULLUP);
    encoderBtnMap[i] = buttons[i];
    lastPositions[i] = encoders[i]->getPosition();
    
    // Set up buffer entries for this encoder
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

// Simple quadrature decoder using library
void updateCustomEncoders() {
    for (uint8_t i = 0; i < customEncoderCount; ++i) {
        // Check multiple times for fast rotation
        int8_t totalDirection = 0;
        for (uint8_t t = 0; t < 8; ++t) {
            int8_t direction = customEncoders[i].decoder->tick();
            if (direction != 0) {
                totalDirection += direction;
            }
        }
        
        if (totalDirection != 0) {
            uint8_t btn = (totalDirection > 0) ? customEncoders[i].joyButtonCW : customEncoders[i].joyButtonCCW;
            uint8_t steps = abs(totalDirection);
            
            // Add to timing buffer instead of immediate processing
            addEncoderSteps(btn, steps);
        }
    }
}

void updateEncoders() {
    // Handle regular encoders (matrix/direct pin)
    for (uint8_t i = 0; i < encoderTotal; i++) {
        // Call tick() multiple times to catch up on missed transitions - increased for fast rotation
        for (uint8_t t = 0; t < 8; ++t) {
            encoders[i]->tick();
        }
        int newPos = encoders[i]->getPosition();
        int diff = newPos - lastPositions[i];

        if (diff != 0) {
          uint8_t btnCW = encoderBtnMap[i].cw;
          uint8_t btnCCW = encoderBtnMap[i].ccw;

          // Handle multiple steps for fast rotation - add to timing buffer
          uint8_t steps = abs(diff);
          uint8_t btn = (diff > 0) ? btnCW : btnCCW;
          
          // Add to timing buffer instead of immediate processing
          addEncoderSteps(btn, steps);
          
          lastPositions[i] = newPos;
        }
    }
    
    // Handle custom shift register encoders
    updateCustomEncoders();
    
    // Process timing buffers for consistent intervals
    processEncoderBuffers();
}

void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    // Count shift register encoders separately
    uint8_t regularCount = 0;
    customEncoderCount = 0;
    
    // Fix: Count actual encoder pairs, not just ENC_A entries
    for (uint8_t i = 0; i < logicalCount - 1; ++i) {
        if ((logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A) &&
            (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B)) {
            customEncoderCount++;
        } else if (((logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) ||
                    (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A)) &&
                   ((logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) ||
                    (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B))) {
            regularCount++;
        }
    }
    
    // Initialize custom encoders
    if (customEncoderCount > 0) {
        customEncoders = new CustomEncoderState[customEncoderCount];
        uint8_t idx = 0;
        
        for (uint8_t i = 0; i < logicalCount - 1; ++i) {
            if ((logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A) &&
                (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B)) {
                
                uint8_t pinA = 100 + (logicals[i].u.shiftreg.regIndex << 4) + logicals[i].u.shiftreg.bitIndex;
                uint8_t pinB = 100 + (logicals[i + 1].u.shiftreg.regIndex << 4) + logicals[i + 1].u.shiftreg.bitIndex;
                
                customEncoders[idx].decoder = new SimpleQuadratureDecoder(pinA, pinB, encoderReadPin);
                customEncoders[idx].joyButtonCW = logicals[i].u.shiftreg.joyButtonID;
                customEncoders[idx].joyButtonCCW = logicals[i + 1].u.shiftreg.joyButtonID;
                idx++;
            }
        }
    }
    
    // Initialize regular encoders (matrix/direct pin)
    if (regularCount > 0) {
        EncoderPins* pins = new EncoderPins[regularCount];
        EncoderButtons* buttons = new EncoderButtons[regularCount];
        uint8_t idx = 0;
        
        // Process all non-shift-register encoder pairs
        for (uint8_t i = 0; i < logicalCount - 1; ++i) {
            bool isEncA = false, isEncB = false;
            uint8_t pinA = 0, pinB = 0;
            uint8_t joyA = 0, joyB = 0;
            
            // Skip shift register encoders (handled by custom decoder)
            if ((logicals[i].type == INPUT_SHIFTREG && logicals[i].u.shiftreg.behavior == ENC_A) &&
                (logicals[i + 1].type == INPUT_SHIFTREG && logicals[i + 1].u.shiftreg.behavior == ENC_B)) {
                continue;
            }
            
            // Check if we have a valid ENC_A + ENC_B pair for regular encoders
            if (((logicals[i].type == INPUT_PIN && logicals[i].u.pin.behavior == ENC_A) ||
                 (logicals[i].type == INPUT_MATRIX && logicals[i].u.matrix.behavior == ENC_A)) &&
                ((logicals[i + 1].type == INPUT_PIN && logicals[i + 1].u.pin.behavior == ENC_B) ||
                 (logicals[i + 1].type == INPUT_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B))) {
                
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
        
        initEncoders(pins, buttons, regularCount);
        delete[] pins;
        delete[] buttons;
    }
}