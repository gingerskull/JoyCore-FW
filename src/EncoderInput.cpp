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

// Timing buffer system for consistent press intervals
#define MAX_ENCODERS 16  // Increased to handle both direct and shift register encoders
struct EncoderBuffer {
    uint8_t cwButtonId;
    uint8_t ccwButtonId;
    uint8_t pendingCwSteps;
    uint8_t pendingCcwSteps;
    uint32_t lastUsbPressTime;  // Timing for USB output
    bool usbButtonPressed;      // State for USB output
    uint8_t currentDirection;   // 0 = none, 1 = CW, 2 = CCW
};

static EncoderBuffer encoderBuffers[MAX_ENCODERS];
static uint8_t bufferCount = 0;
static const uint32_t PRESS_INTERVAL_US = 20000;  // 60ms interval between presses
static const uint32_t PRESS_DURATION_US = 30000;  // 30ms press duration for USB

// Unified encoder system: RotaryEncoder for all encoders
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
    // Use FOUR3 mode for single click per detent - for all encoders
    encoders[i] = new RotaryEncoder(
      pins[i].pinA, pins[i].pinB, RotaryEncoder::LatchMode::FOUR3, encoderReadPin
    );
    
    // Set pinMode for direct pins only (shift register pins are handled by encoderReadPin)
    if (pins[i].pinA < 100 && pins[i].pinB < 100) {
        pinMode(pins[i].pinA, INPUT_PULLUP);
        pinMode(pins[i].pinB, INPUT_PULLUP);
    }
    
    encoderBtnMap[i] = buttons[i];
    lastPositions[i] = encoders[i]->getPosition();
    
    // Set up buffer entry for this encoder pair - one buffer per encoder
    if (bufferCount < MAX_ENCODERS) {
        encoderBuffers[bufferCount].cwButtonId = buttons[i].cw;
        encoderBuffers[bufferCount].ccwButtonId = buttons[i].ccw;
        encoderBuffers[bufferCount].pendingCwSteps = 0;
        encoderBuffers[bufferCount].pendingCcwSteps = 0;
        encoderBuffers[bufferCount].lastUsbPressTime = 0;
        encoderBuffers[bufferCount].usbButtonPressed = false;
        encoderBuffers[bufferCount].currentDirection = 0;
        bufferCount++;
    }
  }
}

// Add steps to buffer for consistent timing
void addEncoderSteps(uint8_t buttonId, uint8_t steps) {
    for (uint8_t i = 0; i < bufferCount; i++) {
        bool isCw = (encoderBuffers[i].cwButtonId == buttonId);
        bool isCcw = (encoderBuffers[i].ccwButtonId == buttonId);
        
        if (isCw || isCcw) {
            // Prevent buffer overflow - cap at reasonable maximum
            uint8_t maxSteps = 50;
            
            if (isCw) {
                if (encoderBuffers[i].pendingCwSteps < maxSteps) {
                    encoderBuffers[i].pendingCwSteps += steps;
                    if (encoderBuffers[i].pendingCwSteps > maxSteps) {
                        encoderBuffers[i].pendingCwSteps = maxSteps;
                    }
                    
                    // Debug output
                    Serial.print("ADD CW: ");
                    Serial.print(steps);
                    Serial.print(" -> ");
                    Serial.println(encoderBuffers[i].pendingCwSteps);
                }
            } else { // isCcw
                if (encoderBuffers[i].pendingCcwSteps < maxSteps) {
                    encoderBuffers[i].pendingCcwSteps += steps;
                    if (encoderBuffers[i].pendingCcwSteps > maxSteps) {
                        encoderBuffers[i].pendingCcwSteps = maxSteps;
                    }
                    
                    // Debug output
                    Serial.print("ADD CCW: ");
                    Serial.print(steps);
                    Serial.print(" -> ");
                    Serial.println(encoderBuffers[i].pendingCcwSteps);
                }
            }
            break;
        }
    }
}

// Helper to ensure stable shift register reads for encoders
void ensureStableShiftRegRead() {
    if (shiftReg && shiftRegBuffer) {
        // Read multiple times to ensure stable data
        for (uint8_t i = 0; i < 3; i++) {
            shiftReg->read(shiftRegBuffer);
            delayMicroseconds(5);
        }
    }
}



// Process timing buffers for consistent intervals
void processEncoderBuffers() {
    uint32_t currentTime = micros();
    
    for (uint8_t i = 0; i < bufferCount; i++) {
        EncoderBuffer& buffer = encoderBuffers[i];
        
        // Handle USB button release timing
        if (buffer.usbButtonPressed && (currentTime - buffer.lastUsbPressTime >= PRESS_DURATION_US)) {
            // Release the current button
            uint8_t currentButtonId = (buffer.currentDirection == 1) ? buffer.cwButtonId : buffer.ccwButtonId;
            uint8_t joyIdx = (currentButtonId > 0) ? (currentButtonId - 1) : 0;
            MyJoystick.setButton(joyIdx, 0);  // Release USB button
            buffer.usbButtonPressed = false;
            // DON'T reset currentDirection here - keep it for direction change detection
            

        }
        
        // Process buffer: decide which direction to process next
        if (!buffer.usbButtonPressed && (buffer.pendingCwSteps > 0 || buffer.pendingCcwSteps > 0)) {
            uint32_t timeSinceLastCycle = currentTime - buffer.lastUsbPressTime;
            

            
            // Determine which direction to process
            uint8_t nextDirection = 0;
            uint8_t nextButtonId = 0;
            
            // Priority logic: continue current direction until exhausted, then switch
            if (buffer.currentDirection == 1 && buffer.pendingCwSteps > 0) {
                // Continue CW if we have pending CW steps
                nextDirection = 1;
                nextButtonId = buffer.cwButtonId;
            } else if (buffer.currentDirection == 2 && buffer.pendingCcwSteps > 0) {
                // Continue CCW if we have pending CCW steps
                nextDirection = 2;
                nextButtonId = buffer.ccwButtonId;
            } else if (buffer.pendingCwSteps > 0) {
                // Switch to CW if current direction is exhausted
                nextDirection = 1;
                nextButtonId = buffer.cwButtonId;
            } else if (buffer.pendingCcwSteps > 0) {
                // Switch to CCW if current direction is exhausted
                nextDirection = 2;
                nextButtonId = buffer.ccwButtonId;
            }
            
            // Check timing constraints
            bool canProcess = false;
            if (buffer.lastUsbPressTime == 0) {
                // First press ever - allow immediate processing
                canProcess = true;
            } else if (nextDirection != buffer.currentDirection) {
                // Direction change - allow immediate processing
                canProcess = true;
            } else {
                // Same direction - need to wait for proper timing
                uint32_t fullCycleTime = PRESS_DURATION_US + PRESS_INTERVAL_US;
                if (timeSinceLastCycle >= fullCycleTime) {
                    canProcess = true;
                }
            }
            
            if (canProcess && nextDirection > 0) {
                uint8_t joyIdx = (nextButtonId > 0) ? (nextButtonId - 1) : 0;
                MyJoystick.setButton(joyIdx, 1);  // Press USB button
                buffer.usbButtonPressed = true;
                buffer.lastUsbPressTime = currentTime;
                buffer.currentDirection = nextDirection;
                
                // Decrement the appropriate counter
                if (nextDirection == 1) {
                    buffer.pendingCwSteps--;
                    Serial.print("PRESS CW -> pending: ");
                    Serial.println(buffer.pendingCwSteps);
                } else {
                    buffer.pendingCcwSteps--;
                    Serial.print("PRESS CCW -> pending: ");
                    Serial.println(buffer.pendingCcwSteps);
                }
            }
        }
        
        // Safety mechanism: if we have a stuck USB button for too long, force release
        if (buffer.usbButtonPressed && (currentTime - buffer.lastUsbPressTime >= PRESS_DURATION_US * 2)) {
            uint8_t currentButtonId = (buffer.currentDirection == 1) ? buffer.cwButtonId : buffer.ccwButtonId;
            uint8_t joyIdx = (currentButtonId > 0) ? (currentButtonId - 1) : 0;
            MyJoystick.setButton(joyIdx, 0);  // Force release
            buffer.usbButtonPressed = false;
            // DON'T reset currentDirection here either - keep it for direction tracking
        }
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
          
          // Debug encoder detection with pin info
          Serial.print("ENCODER ");
          Serial.print(i);
          Serial.print(" (pins ");
          Serial.print(encoderBtnMap[i].cw);
          Serial.print("/");
          Serial.print(encoderBtnMap[i].ccw);
          Serial.print("): ");
          Serial.print(lastPositions[i]);
          Serial.print(" -> ");
          Serial.print(newPos);
          Serial.print(" (");
          Serial.print(diff > 0 ? "CW" : "CCW");
          Serial.println(")");
          
          // Add to timing buffer
          addEncoderSteps(btn, steps);
          
          lastPositions[i] = newPos;
        }
    }
    
    // Process timing buffers for consistent intervals
    processEncoderBuffers();
}

void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    // Count all encoder pairs
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
    
    // Initialize all encoders with RotaryEncoder library
    if (encoderCount > 0) {
        EncoderPins* pins = new EncoderPins[encoderCount];
        EncoderButtons* buttons = new EncoderButtons[encoderCount];
        uint8_t idx = 0;
        
        for (uint8_t i = 0; i < logicalCount - 1; ++i) {
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
                    pins[idx].pinA = pinA;
                    pins[idx].pinB = pinB;
                    buttons[idx].cw = joyA;
                    buttons[idx].ccw = joyB;
                    idx++;
                }
            }
        }
        
        initEncoders(pins, buttons, encoderCount);
        delete[] pins;
        delete[] buttons;
    }
}