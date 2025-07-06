//GNGR ButtonBox - Initial code
//Basic Button and Encoder Input for Joystick
//This code is designed to work with the Arduino Leonardo and similar boards

#include "Config.h"
#include "JoystickWrapper.h"
#include "ButtonInput.h"
#include "EncoderInput.h"

// Create global joystick instance
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   32, 0,                  // Buttons, Hat switches
                   false, false, false,    // No axes
                   false, false, false,    // No rotation
                   false, false,           // No throttle, rudder
                   false, false, false);   // No brake, accel, steering


void setup() {
  // Initialize Joystick Library
  Joystick.begin();
  Serial.begin(115200);
  initButtons(buttonPins, buttonCount);
  initEncoders(encoderPins, encoderButtons, encoderCount);
}

void loop() {
  updateButtons();
  updateEncoders();
}