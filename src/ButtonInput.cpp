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
      case MOM:
        Joystick.setButton(buttons[i].joyButtonID, isLow);
        break;

      case LATCH:
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
