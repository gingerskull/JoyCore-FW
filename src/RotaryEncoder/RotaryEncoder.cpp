// RotaryEncoder - modified for GNGR-ButtonBox by gingerskull, 2025
// Original: Copyright (c) Matthias Hertel, http://www.mathertel.de
// Licensed under the terms of the original project and the GNU General Public License v3.0 or later.
// See LICENSE for details. All modifications are marked and described below.

// -----
// RotaryEncoder.cpp - Library for using rotary encoders.
// This class is implemented for use with the Arduino environment.
//
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
//
// This work is licensed under a BSD 3-Clause style license,
// https://www.mathertel.de/License.aspx.
//
// More information on: http://www.mathertel.de/Arduino
// -----
// Changelog: see RotaryEncoder.h
// -----

#include "RotaryEncoder.h"
#include "Arduino.h"

#define LATCH0 0 // input state at position 0
#define LATCH3 3 // input state at position 3


// The array holds the values �1 for the entries where a position was decremented,
// a 1 for the entries where the position was incremented
// and 0 in all the other (no change or not valid) cases.

const int8_t KNOBDIR[] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};


// positions: [3] 1 0 2 [3] 1 0 2 [3]
// [3] is the positions where my rotary switch detends
// ==> right, count up
// <== left,  count down


// ----- Initialization and Default Values -----

RotaryEncoder::RotaryEncoder(int pin1, int pin2, LatchMode mode, PinReadFn pinRead)
{
  _pin1 = pin1;
  _pin2 = pin2;
  _mode = mode;
  _pinReadFn = pinRead;

  pinMode(pin1, INPUT_PULLUP);
  pinMode(pin2, INPUT_PULLUP);

  // Use the pin read function if provided
  int sig1 = _pinReadFn ? _pinReadFn(_pin1) : digitalRead(_pin1);
  int sig2 = _pinReadFn ? _pinReadFn(_pin2) : digitalRead(_pin2);
  _oldState = sig1 | (sig2 << 1);

  // start with position 0;
  _position = 0;
  _positionExt = 0;
  _positionExtPrev = 0;
} // RotaryEncoder()


long RotaryEncoder::getPosition()
{
  return _positionExt;
} // getPosition()


RotaryEncoder::Direction RotaryEncoder::getDirection()
{
  RotaryEncoder::Direction ret = Direction::NOROTATION;

  if (_positionExtPrev > _positionExt) {
    ret = Direction::COUNTERCLOCKWISE;
    _positionExtPrev = _positionExt;
  } else if (_positionExtPrev < _positionExt) {
    ret = Direction::CLOCKWISE;
    _positionExtPrev = _positionExt;
  } else {
    ret = Direction::NOROTATION;
    _positionExtPrev = _positionExt;
  }

  return ret;
}


void RotaryEncoder::setPosition(long newPosition)
{
  switch (_mode) {
  case LatchMode::FOUR3:
  case LatchMode::FOUR0:
    // only adjust the external part of the position.
    _position = ((newPosition << 2) | (_position & 0x03L));
    _positionExt = newPosition;
    _positionExtPrev = newPosition;
    break;

  case LatchMode::TWO03:
    // only adjust the external part of the position.
    _position = ((newPosition << 1) | (_position & 0x01L));
    _positionExt = newPosition;
    _positionExtPrev = newPosition;
    break;
  } // switch

} // setPosition()


void RotaryEncoder::tick(void)
{
  // Use the pin read function if provided
  int sig1 = _pinReadFn ? _pinReadFn(_pin1) : digitalRead(_pin1);
  int sig2 = _pinReadFn ? _pinReadFn(_pin2) : digitalRead(_pin2);
  int8_t thisState = sig1 | (sig2 << 1);

  if (_oldState != thisState) {
    _position += KNOBDIR[thisState | (_oldState << 2)];
    _oldState = thisState;

    switch (_mode) {
    case LatchMode::FOUR3:
      if (thisState == LATCH3) {
        // The hardware has 4 steps with a latch on the input state 3
        _positionExt = _position >> 2;
        _positionExtTimePrev = _positionExtTime;
        _positionExtTime = millis();
      }
      break;

    case LatchMode::FOUR0:
      if (thisState == LATCH0) {
        // The hardware has 4 steps with a latch on the input state 0
        _positionExt = _position >> 2;
        _positionExtTimePrev = _positionExtTime;
        _positionExtTime = millis();
      }
      break;

    case LatchMode::TWO03:
      if ((thisState == LATCH0) || (thisState == LATCH3)) {
        // The hardware has 2 steps with a latch on the input state 0 and 3
        _positionExt = _position >> 1;
        _positionExtTimePrev = _positionExtTime;
        _positionExtTime = millis();
      }
      break;
    } // switch
  } // if
} // tick()


unsigned long RotaryEncoder::getMillisBetweenRotations() const
{
  return (_positionExtTime - _positionExtTimePrev);
}

unsigned long RotaryEncoder::getRPM()
{
  // calculate max of difference in time between last position changes or last change and now.
  unsigned long timeBetweenLastPositions = _positionExtTime - _positionExtTimePrev;
  unsigned long timeToLastPosition = millis() - _positionExtTime;
  unsigned long t = max(timeBetweenLastPositions, timeToLastPosition);
  return 60000.0 / ((float)(t * 20));
}

// Simple quadrature decoder implementation
SimpleQuadratureDecoder::SimpleQuadratureDecoder(uint8_t pinA, uint8_t pinB, RotaryEncoder::PinReadFn pinRead)
    : _pinA(pinA), _pinB(pinB), _pinReadFn(pinRead) {
    _lastStateA = _pinReadFn ? _pinReadFn(_pinA) : digitalRead(_pinA);
    _lastStateB = _pinReadFn ? _pinReadFn(_pinB) : digitalRead(_pinB);
}

int8_t SimpleQuadratureDecoder::tick() {
    uint8_t stateA = _pinReadFn ? _pinReadFn(_pinA) : digitalRead(_pinA);
    uint8_t stateB = _pinReadFn ? _pinReadFn(_pinB) : digitalRead(_pinB);
    
    uint8_t currentState = (stateA << 1) | stateB;
    uint8_t lastState = (_lastStateA << 1) | _lastStateB;
    
    int8_t result = 0;
    if (currentState != lastState) {
        // Look for specific quadrature patterns
        // CW: 11 -> 01 (trigger on 11 -> 01)
        // CCW: 11 -> 10 (trigger on 11 -> 10)
        if (lastState == 3 && currentState == 1) {
            result = 1; // CW
        } else if (lastState == 3 && currentState == 2) {
            result = -1; // CCW
        }
    }
    
    _lastStateA = stateA;
    _lastStateB = stateB;
    return result;
}

// End