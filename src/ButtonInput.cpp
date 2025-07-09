#include "ButtonInput.h"
#include "JoystickWrapper.h" 
#include "Config.h"

static ButtonConfig* buttons = nullptr;  // Changed from const ButtonConfig*
static uint8_t btnCount = 0;
static bool* lastStates = nullptr;

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

    switch (buttons[i].behavior) {
      case NORMAL:
        Joystick.setButton(buttons[i].joyButtonID, isLow);
        break;

      case MOMENTARY:
        // Trigger a one-shot press+release on LOW edge only
        if (!wasLow && isLow) {
          Joystick.setButton(buttons[i].joyButtonID, 1);
          delay(10);  // brief press duration
          Joystick.setButton(buttons[i].joyButtonID, 0);
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
}

void initButtonsFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
  // Count buttons (excluding encoder behaviors)
  uint8_t count = 0;
  for (uint8_t i = 0; i < logicalCount; ++i)
    if (logicals[i].type == LOGICAL_BTN && 
        logicals[i].u.btn.behavior != ENC_A && 
        logicals[i].u.btn.behavior != ENC_B) count++;

  if (count == 0) return;

  ButtonConfig* configs = new ButtonConfig[count];
  uint8_t idx = 0;
  for (uint8_t i = 0; i < logicalCount; ++i) {
    if (logicals[i].type == LOGICAL_BTN && 
        logicals[i].u.btn.behavior != ENC_A && 
        logicals[i].u.btn.behavior != ENC_B) {
      configs[idx].pin = logicals[i].u.btn.pin;
      configs[idx].joyButtonID = logicals[i].u.btn.joyButtonID;
      configs[idx].behavior = logicals[i].u.btn.behavior;
      idx++;
    }
  }
  initButtons(configs, count);
  delete[] configs;  // Now safe to delete since initButtons() copies the data
}

