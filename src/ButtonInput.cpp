#include "ButtonInput.h"
#include "JoystickWrapper.h" 
#include "Config.h"

static const ButtonConfig* buttons = nullptr;
static uint8_t btnCount = 0;
static bool* lastStates = nullptr;

void initButtons(const ButtonConfig* configs, uint8_t count) {
  buttons = configs;
  btnCount = count;
  lastStates = new bool[count];

  for (uint8_t i = 0; i < count; i++) {
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
    }

    lastStates[i] = currentState;
  }
}

void initButtonsFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
  // Count buttons
  uint8_t count = 0;
  for (uint8_t i = 0; i < logicalCount; ++i)
    if (logicals[i].type == LOGICAL_BTN) count++;

  ButtonConfig* configs = new ButtonConfig[count];
  uint8_t idx = 0;
  for (uint8_t i = 0; i < logicalCount; ++i) {
    if (logicals[i].type == LOGICAL_BTN) {
      configs[idx].pin = logicals[i].u.btn.pin;
      configs[idx].joyButtonID = logicals[i].u.btn.joyButtonID;
      configs[idx].behavior = logicals[i].u.btn.behavior;
      idx++;
    }
  }
  initButtons(configs, count);
  // Optionally: delete[] configs after initButtons copies data
}

