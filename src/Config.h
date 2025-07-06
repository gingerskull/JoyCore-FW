#pragma once
#include <Arduino.h>

// ===========================
// 🔧 Button Config
// ===========================

enum ButtonBehavior : uint8_t {
  MOM,    // Momentary: button held = HID pressed
  LATCH   // Latching: one-shot HID press when switched on
};

struct ButtonConfig {
  uint8_t pin;
  uint8_t joyButtonID;
  ButtonBehavior behavior;
};

// Example setup
constexpr ButtonConfig buttonConfigs[] = {
  {2, 0, MOM},
  {3, 1, MOM},
  {8, 2, LATCH},
};

constexpr uint8_t buttonCount = sizeof(buttonConfigs) / sizeof(buttonConfigs[0]);


// ============================
// 🎮 Logical Joystick Mappings
// ============================

// --- Joystick Button IDs for buttons ---
constexpr uint8_t joyButtonBase = 0;

// ===========================
// 🔧 Encoder Config
// ===========================

struct EncoderPins {
  uint8_t pinA;
  uint8_t pinB;
};

constexpr EncoderPins encoderPins[] = {
  {4, 5},
  {6, 7},
};

struct EncoderButtons {
  uint8_t cw;   // Clockwise button ID
  uint8_t ccw;  // Counter-clockwise button ID
};

constexpr EncoderButtons encoderButtons[] = {
  {20, 21},   // Encoder 0
  {22, 23},   // Encoder 1
};

constexpr uint8_t encoderCount = sizeof(encoderPins) / sizeof(encoderPins[0]);