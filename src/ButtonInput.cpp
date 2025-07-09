// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonInput.h"
#include "JoystickWrapper.h"
#include "Config.h"
#include "ShiftRegister165.h"

static ButtonConfig* buttons = nullptr;  // Changed from const ButtonConfig*
static uint8_t btnCount = 0;
static bool* lastStates = nullptr;

ShiftRegister165* shiftReg = nullptr;  // Remove static to make it accessible
uint8_t* shiftRegBuffer = nullptr;

void initButtons(const ButtonConfig* configs, uint8_t count) {
  btnCount = count;
  
  // Clean up previous allocation
  if (buttons) delete[] buttons;
  if (lastStates) delete[] lastStates;
  
  // Copy the configuration data
  buttons = new ButtonConfig[count];
  lastStates = new bool[count];
  
  for (uint8_t i = 0; i < count; i++) {
    buttons[i] = configs[i];  // Copy the config
    pinMode(buttons[i].pin, INPUT_PULLUP);
    lastStates[i] = digitalRead(buttons[i].pin);
  }
}

void updateButtons() {
  for (uint8_t i = 0; i < btnCount; i++) {
    bool currentState = digitalRead(buttons[i].pin);  // HIGH = released, LOW = pressed
    bool wasLow = lastStates[i] == LOW;
    bool isLow = currentState == LOW;

    uint8_t joyIdx = (buttons[i].joyButtonID > 0) ? (buttons[i].joyButtonID - 1) : 0;

    switch (buttons[i].behavior) {
      case NORMAL:
        Joystick.setButton(joyIdx, isLow);
        break;

      case MOMENTARY:
        // Trigger a one-shot press+release on LOW edge only
        if (!wasLow && isLow) {
          Joystick.setButton(joyIdx, 1);
          delay(10);  // brief press duration
          Joystick.setButton(joyIdx, 0);
        }
        break;

      case ENC_A:
      case ENC_B:
        // Encoder behaviors should be filtered out in initButtonsFromLogical()
        // This case should never be reached
        break;
    }

    lastStates[i] = currentState;
  }

  // Use shiftRegBuffer as filled in main.cpp
  if (shiftReg) {
    for (uint8_t i = 0; i < logicalInputCount; ++i) {
      if (logicalInputs[i].source == SRC_SHIFTREG) {
        ButtonBehavior behavior = logicalInputs[i].u.shiftreg.behavior;
        // Only handle as button if not an encoder channel
        if (behavior != ENC_A && behavior != ENC_B) {
          uint8_t reg = logicalInputs[i].u.shiftreg.regIndex;
          uint8_t bit = logicalInputs[i].u.shiftreg.bitIndex;
          uint8_t joyIdx = (logicalInputs[i].u.shiftreg.joyButtonID > 0) ? (logicalInputs[i].u.shiftreg.joyButtonID - 1) : 0;
          if (reg < SHIFTREG_COUNT && bit < 8) {
            bool pressed = (shiftRegBuffer[reg] & (1 << bit)) == 0; // 74HC165: LOW = pressed
            Joystick.setButton(joyIdx, pressed);
          }
        }
      }
    }
  }
}

void initButtonsFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
  // Count buttons (excluding encoder behaviors, including shiftreg encoders)
  uint8_t count = 0;
  for (uint8_t i = 0; i < logicalCount; ++i)
    if (logicals[i].type == LOGICAL_BTN &&
        logicals[i].u.btn.behavior != ENC_A &&
        logicals[i].u.btn.behavior != ENC_B &&
        !(logicals[i].source == SRC_SHIFTREG &&
          (logicals[i].u.shiftreg.behavior == ENC_A || logicals[i].u.shiftreg.behavior == ENC_B)))
      count++;

  if (count == 0) return;

  ButtonConfig* configs = new ButtonConfig[count];
  uint8_t idx = 0;
  for (uint8_t i = 0; i < logicalCount; ++i) {
    if (logicals[i].type == LOGICAL_BTN &&
        logicals[i].u.btn.behavior != ENC_A &&
        logicals[i].u.btn.behavior != ENC_B &&
        !(logicals[i].source == SRC_SHIFTREG &&
          (logicals[i].u.shiftreg.behavior == ENC_A || logicals[i].u.shiftreg.behavior == ENC_B))) {
      configs[idx].pin = logicals[i].u.btn.pin;
      configs[idx].joyButtonID = logicals[i].u.btn.joyButtonID;
      configs[idx].behavior = logicals[i].u.btn.behavior;
      idx++;
    }
  }
  initButtons(configs, count);
  delete[] configs;  // Now safe to delete since initButtons() copies the data

  // Initialize shift register if any SRC_SHIFTREG present
  bool hasShiftReg = false;
  for (uint8_t i = 0; i < logicalCount; ++i)
    if (logicals[i].source == SRC_SHIFTREG)
      hasShiftReg = true;
  if (hasShiftReg) {
    int plPin = -1, clkPin = -1, qhPin = -1;
    for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
      if (hardwarePinMap[i].type == SHIFTREG_PL) plPin = atoi(hardwarePinMap[i].name);
      if (hardwarePinMap[i].type == SHIFTREG_CLK) clkPin = atoi(hardwarePinMap[i].name);
      if (hardwarePinMap[i].type == SHIFTREG_QH) qhPin = atoi(hardwarePinMap[i].name);
    }
    if (plPin >= 0 && clkPin >= 0 && qhPin >= 0) {
      shiftReg = new ShiftRegister165(plPin, clkPin, qhPin, SHIFTREG_COUNT);
      shiftReg->begin();
      if (shiftRegBuffer) delete[] shiftRegBuffer;
      shiftRegBuffer = new uint8_t[SHIFTREG_COUNT];
    }
  }
}
