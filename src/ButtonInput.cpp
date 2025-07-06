#include "ButtonInput.h"
#include "JoystickWrapper.h" 
#include "Config.h"

static const uint8_t* btnPins = nullptr;
static uint8_t btnCount = 0;

void initButtons(const uint8_t* pins, uint8_t count) {
  btnPins = pins;
  btnCount = count;

  for (uint8_t i = 0; i < count; i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
  }
}

void updateButtons() {
  for (uint8_t i = 0; i < btnCount; i++) {
    bool pressed = digitalRead(btnPins[i]) == LOW;
    Joystick.setButton(i, pressed);
  }
}
