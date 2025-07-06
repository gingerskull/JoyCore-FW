#pragma once
#include <Arduino.h>  // for uint8_t

// ===========================
// 🛠️ Hardware Pin Definitions
// ===========================

// --- Button Input Pins ---
constexpr uint8_t buttonPins[] = {2, 3};  // Add more as needed
constexpr uint8_t buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);

// --- Encoder Pin Pairs (A, B) ---
struct EncoderPins {
  uint8_t pinA;
  uint8_t pinB;
};

constexpr EncoderPins encoderPins[] = {
  {4, 5},   // Encoder 0
  {6, 7},   // Encoder 1
};
constexpr uint8_t encoderCount = sizeof(encoderPins) / sizeof(EncoderPins);

// ============================
// 🎮 Logical Joystick Mappings
// ============================

// --- Joystick Button IDs for buttons ---
constexpr uint8_t joyButtonBase = 0;

// --- Joystick Button IDs for encoder rotations ---
struct EncoderButtons {
  uint8_t cw;   // Clockwise
  uint8_t ccw;  // Counter-clockwise
};

constexpr EncoderButtons encoderButtons[] = {
  {20, 21},   // Encoder 0
  {22, 23},   // Encoder 1
};
