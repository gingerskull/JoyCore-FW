#include "EncoderInput.h"
#include "JoystickWrapper.h"
#include "Config.h"
#include "RotaryEncoder/RotaryEncoder.h"

// External variable for matrix pin states
extern bool g_encoderMatrixPinStates[20];

// Helper to get pin state for encoder (matrix-aware)
static int encoderReadPin(uint8_t pin) {
    return g_encoderMatrixPinStates[pin];
}

// Internal encoder state
static RotaryEncoder** encoders = nullptr;
static EncoderButtons* encoderBtnMap = nullptr;
static int* lastPositions = nullptr;
static unsigned long* pressStartTimes = nullptr;
static uint8_t* activeBtns = nullptr;
static uint8_t encoderTotal = 0;

void initEncoders(const EncoderPins* pins, const EncoderButtons* buttons, uint8_t count) {
  encoderTotal = count;

  encoders = new RotaryEncoder*[count];
  encoderBtnMap = new EncoderButtons[count];
  lastPositions = new int[count];
  pressStartTimes = new unsigned long[count];
  activeBtns = new uint8_t[count];

  for (uint8_t i = 0; i < count; i++) {
    // Use encoderReadPin for all encoders (works for both matrix and direct pins)
    encoders[i] = new RotaryEncoder(
      pins[i].pinA, pins[i].pinB, RotaryEncoder::LatchMode::FOUR3, encoderReadPin
    );
    pinMode(pins[i].pinA, INPUT_PULLUP);
    pinMode(pins[i].pinB, INPUT_PULLUP);
    encoderBtnMap[i] = buttons[i];
    lastPositions[i] = encoders[i]->getPosition();
    pressStartTimes[i] = 0;
    activeBtns[i] = 255;  // 255 = no active press
  }
}

void updateEncoders() {
  for (uint8_t i = 0; i < encoderTotal; i++) {
    encoders[i]->tick();
    int newPos = encoders[i]->getPosition();
    int diff = newPos - lastPositions[i];

    if (diff != 0 && activeBtns[i] == 255) {
      uint8_t btn = (diff > 0) ? encoderBtnMap[i].cw : encoderBtnMap[i].ccw;
            
      Joystick.setButton(btn, 1);
      pressStartTimes[i] = millis();
      activeBtns[i] = btn;
      lastPositions[i] = newPos;
    }

    if (activeBtns[i] != 255 && millis() - pressStartTimes[i] > 10) {
      Joystick.setButton(activeBtns[i], 0);
      activeBtns[i] = 255;
    }
  }
}

void initEncodersFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
  // Find encoder pairs (adjacent ENC_A and ENC_B from ANY source)
  uint8_t count = 0;
  
  // Check for adjacent encoders (button OR matrix)
  for (uint8_t i = 0; i < logicalCount - 1; ++i) {
    bool isEncA = false, isEncB = false;
    
    // Check if current is ENC_A
    if ((logicals[i].type == LOGICAL_BTN && logicals[i].u.btn.behavior == ENC_A) ||
        (logicals[i].type == LOGICAL_MATRIX && logicals[i].u.matrix.behavior == ENC_A)) {
      isEncA = true;
    }
    
    // Check if next is ENC_B
    if ((logicals[i + 1].type == LOGICAL_BTN && logicals[i + 1].u.btn.behavior == ENC_B) ||
        (logicals[i + 1].type == LOGICAL_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B)) {
      isEncB = true;
    }
    
    if (isEncA && isEncB) count++;
  }

  if (count == 0) return;

  EncoderPins* pins = new EncoderPins[count];
  EncoderButtons* buttons = new EncoderButtons[count];
  uint8_t idx = 0;
  
  // Process all encoder pairs
  for (uint8_t i = 0; i < logicalCount - 1; ++i) {
    bool isEncA = false, isEncB = false;
    uint8_t pinA = 0, pinB = 0;
    uint8_t joyA = 0, joyB = 0;
    
    // Get ENC_A info
    if (logicals[i].type == LOGICAL_BTN && logicals[i].u.btn.behavior == ENC_A) {
      isEncA = true;
      pinA = logicals[i].u.btn.pin;
      joyA = logicals[i].u.btn.joyButtonID;
    } else if (logicals[i].type == LOGICAL_MATRIX && logicals[i].u.matrix.behavior == ENC_A) {
      isEncA = true;
      // Find the actual pin for this matrix position
      uint8_t rowIdx = 0;
      for (uint8_t pin = 0; pin < hardwarePinsCount; ++pin) {
        if (hardwarePins[pin] == BTN_ROW) {
          if (rowIdx == logicals[i].u.matrix.row) {
            pinA = pin;
            break;
          }
          rowIdx++;
        }
      }
      joyA = logicals[i].u.matrix.joyButtonID;
    }
    
    // Get ENC_B info
    if (logicals[i + 1].type == LOGICAL_BTN && logicals[i + 1].u.btn.behavior == ENC_B) {
      isEncB = true;
      pinB = logicals[i + 1].u.btn.pin;
      joyB = logicals[i + 1].u.btn.joyButtonID;
    } else if (logicals[i + 1].type == LOGICAL_MATRIX && logicals[i + 1].u.matrix.behavior == ENC_B) {
      isEncB = true;
      // Find the actual pin for this matrix position
      uint8_t rowIdx = 0;
      for (uint8_t pin = 0; pin < hardwarePinsCount; ++pin) {
        if (hardwarePins[pin] == BTN_ROW) {
          if (rowIdx == logicals[i + 1].u.matrix.row) {
            pinB = pin;
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
      // Fix: ENC_A should be clockwise, ENC_B should be counter-clockwise
      buttons[idx].cw = joyA;   // ENC_A joystick button for clockwise
      buttons[idx].ccw = joyB;  // ENC_B joystick button for counter-clockwise
      idx++;
    }
  }
  
  initEncoders(pins, buttons, count);
  delete[] pins;
  delete[] buttons;
}
