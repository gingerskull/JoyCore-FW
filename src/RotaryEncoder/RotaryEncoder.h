// RotaryEncoder - modified for GNGR-ButtonBox by gingerskull, 2025
// Original: Copyright (c) Matthias Hertel, http://www.mathertel.de
// Licensed under the terms of the original project and the GNU General Public License v3.0 or later.
// See LICENSE for details. All modifications are marked and described below.

// -----
// RotaryEncoder.h - Library for using rotary encoders.
// This class is implemented for use with the Arduino environment.
//
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
//
// This work is licensed under a BSD 3-Clause style license,
// https://www.mathertel.de/License.aspx.
//
// More information on: http://www.mathertel.de/Arduino
// -----
// 18.01.2014 created by Matthias Hertel
// 16.06.2019 pin initialization using INPUT_PULLUP
// 10.11.2020 Added the ability to obtain the encoder RPM
// 29.01.2021 Options for using rotary encoders with 2 state changes per latch.
// -----

#ifndef RotaryEncoder_h
#define RotaryEncoder_h

#include "Arduino.h"

class RotaryEncoder
{
public:
  enum class Direction {
    NOROTATION = 0,
    CLOCKWISE = 1,
    COUNTERCLOCKWISE = -1
  };

  enum class LatchMode {
    FOUR3 = 1, // 4 steps, Latch at position 3 only (compatible to older versions)
    FOUR0 = 2, // 4 steps, Latch at position 0 (reverse wirings)
    TWO03 = 3  // 2 steps, Latch at position 0 and 3 
  };

  // Add a type for the pin read function
  typedef int (*PinReadFn)(uint8_t);

  // ----- Constructor -----
  // Add an optional pin read function to the constructor
  RotaryEncoder(int pin1, int pin2, LatchMode mode = LatchMode::FOUR0, PinReadFn pinRead = nullptr);

  // retrieve the current position
  long getPosition();

  // simple retrieve of the direction the knob was rotated last time. 0 = No rotation, 1 = Clockwise, -1 = Counter Clockwise
  Direction getDirection();

  // adjust the current position
  void setPosition(long newPosition);

  // call this function every some milliseconds or by using an interrupt for handling state changes of the rotary encoder.
  void tick(void);

  // Returns the time in milliseconds between the current observed
  unsigned long getMillisBetweenRotations() const;

  // Returns the RPM
  unsigned long getRPM();

private:
  int _pin1, _pin2; // Arduino pins used for the encoder.
  
  LatchMode _mode; // Latch mode from initialization

  // Add a member to store the pin read function
  PinReadFn _pinReadFn;

  volatile int8_t _oldState;

  volatile long _position;        // Internal position (4 times _positionExt)
  volatile long _positionExt;     // External position
  volatile long _positionExtPrev; // External position (used only for direction checking)

  unsigned long _positionExtTime;     // The time the last position change was detected.
  unsigned long _positionExtTimePrev; // The time the previous position change was detected.
};

// Add a simple quadrature decoder for shift register encoders
class SimpleQuadratureDecoder {
public:
    SimpleQuadratureDecoder(uint8_t pinA, uint8_t pinB, RotaryEncoder::PinReadFn pinRead);
    
    // Returns -1 for CCW, 0 for no change, 1 for CW
    int8_t tick();
    
private:
    uint8_t _pinA, _pinB;
    uint8_t _lastStateA, _lastStateB;
    RotaryEncoder::PinReadFn _pinReadFn;
};

#endif

// End