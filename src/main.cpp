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

/*void printPin34567State() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    pinMode(3, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);
    pinMode(5, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);
    int state3 = digitalRead(3);
    int state4 = digitalRead(4);
    int state5 = digitalRead(5);
    int state6 = digitalRead(6);
    int state7 = digitalRead(7);
    Serial.print("Pin 3: ");
    Serial.print(state3);
    Serial.print(" | Pin 4: ");
    Serial.print(state4);
    Serial.print(" | Pin 5: ");
    Serial.print(state5);
    Serial.print(" | Pin 6: ");
    Serial.print(state6);
    Serial.print(" | Pin 7: ");
    Serial.println(state7);
  }
}*/

void loop() {
  updateButtons();
  updateEncoders();
  updateMatrix();
 // printPin34567State();
}
