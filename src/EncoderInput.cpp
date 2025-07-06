#include "EncoderInput.h"
#include "JoystickWrapper.h"
#include "Config.h"
#include <RotaryEncoder.h>

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
    encoders[i] = new RotaryEncoder(pins[i].pinA, pins[i].pinB, RotaryEncoder::LatchMode::FOUR3);
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

    if (newPos != lastPositions[i] && activeBtns[i] == 255) {
      uint8_t btn = (newPos > lastPositions[i]) ? encoderBtnMap[i].cw : encoderBtnMap[i].ccw;
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
