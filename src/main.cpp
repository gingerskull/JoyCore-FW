#include <Arduino.h>
#include "Config.h"
#include "JoystickWrapper.h"
#include "ButtonInput.h"
#include "EncoderInput.h"

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   32, 0, false, false, false,
                   false, false, false,
                   false, false, false);

void setup() {
  Joystick.begin();
  initButtons(buttonConfigs, buttonCount);
  initEncoders(encoderPins, encoderButtons, encoderCount);
}

void loop() {
  updateButtons();
  updateEncoders();
}
