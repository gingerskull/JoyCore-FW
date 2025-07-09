#include <Arduino.h>
#include "Config.h"
#include "JoystickWrapper.h"
#include "ButtonInput.h"
#include "EncoderInput.h"
#include "MatrixInput.h"

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   32, 0, false, false, false,
                   false, false, false,
                   false, false, false);

void setup() {
  Joystick.begin();
  initButtonsFromLogical(logicalInputs, logicalInputCount);
  initEncodersFromLogical(logicalInputs, logicalInputCount);
  initMatrixFromLogical(logicalInputs, logicalInputCount);
}


void loop() {
  updateButtons();
  updateMatrix();
  updateEncoders();
  // printPin34567State();
}
