# JoyCore

**JoyCore** is an Arduino-based USB game controller firmware. It supports matrix buttons, rotary encoders, direct pin buttons, shift register inputs, and analog axes, letting you build customizable button boxes or control panels.
## 🔄 Firmware Versions

JoyCore is being developed for multiple microcontroller platforms:

- **[Leonardo/Micro (ATmega32U4)](https://github.com/gingerskull/JoyCore-FW/tree/main)** - This branch
- **[Teensy 4.0](https://github.com/gingerskull/JoyCore-FW/tree/teensy40)** - High-performance ARM Cortex-M7
- **[RP2040 Pico](https://github.com/gingerskull/JoyCore-FW/tree/rp2040-native)** - Raspberry Pi Pico and compatible boards

Each version is optimized for its target platform while maintaining consistent functionality and configuration.

---

## 🖥️ Supported Boards

JoyCore is designed for basically any board based on the ATmega32U4 microcontroller, including:

**✅ Tested:**
- [SparkFun Pro Micro](https://www.sparkfun.com/products/12640)

**⚠️ Untested (should work, but not verified):**
- [Arduino Leonardo](https://docs.arduino.cc/hardware/leonardo/)
- [Arduino Micro](https://store.arduino.cc/products/arduino-micro)
- [Adafruit ItsyBitsy 32u4](https://www.adafruit.com/product/3677)
- [Adafruit Feather 32u4](https://www.adafruit.com/product/2771)

> **Note:** Only the SparkFun Pro Micro has been tested so far. The other boards are supported in theory but I have not tested them yet.

#### SparkFun Pro Micro Pinout Reference

![SparkFun Pro Micro Pinout](images/pro-micro-pinout.png)

Use this pinout reference when configuring your hardware connections in the config files.

---

## ✨ Features

- **Matrix Button Scanning**: Efficient row/column multiplexing for large button arrays.
- **Rotary Encoders**: Handles both direct/matrix and shift register-based encoders.
- **Shift Registers**: Expand inputs with [74HC165](https://www.ti.com/lit/ds/symlink/sn74hc165.pdf) chips.
- **Analog Axes**: Support for up to 8 analog axes with filtering and response curves.
- **USB HID Game Controller**: Native USB interface via the [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary).

---

## 🛠️ Hardware Setup

### 1. Pin Mapping

Edit your pin mapping in `src/ConfigDigital.h` (or `src/ConfigAxis.h` for axes):

```cpp
static const PinMapEntry hardwarePinMap[] = {
  {"2", BTN_ROW},         // Matrix row
  {"3", BTN_ROW},
  {"4", BTN_ROW},
  {"5", BTN_ROW},
  {"6", BTN_COL},         // Matrix column
  {"16", SHIFTREG_QH},    // 74HC165 QH (serial out)
  {"14", SHIFTREG_PL},    // 74HC165 PL (parallel load)
  {"15", SHIFTREG_CLK}    // 74HC165 CLK (clock)
  // Add more as needed...
};
```

**Pin Types:**
- `BTN_ROW`, `BTN_COL`: For matrix scanning
- `SHIFTREG_PL`, `SHIFTREG_CLK`, `SHIFTREG_QH`: For shift register chips
- `BTN`: For direct-wired buttons

---

### 2. Shift Register Configuration

Set the number of chained 74HC165 chips in `src/ConfigDigital.h`:

```cpp
#define SHIFTREG_COUNT    1  // Number of 74HC165 chips
```

---

### 3. Logical Inputs

Define your logical inputs in the `logicalInputs` array in `src/ConfigDigital.h`. Each entry describes a button or encoder and how it maps to the joystick.

```cpp
constexpr LogicalInput logicalInputs[] = {
  // Matrix buttons: row, col, joystick button ID, behavior
  { INPUT_MATRIX, { .matrix = {0, 0, 1, NORMAL} } },
  { INPUT_MATRIX, { .matrix = {1, 0, 2, NORMAL} } },

  // Matrix encoder: adjacent matrix positions, each with a unique joystick button
  { INPUT_MATRIX, { .matrix = {2, 0, 3, ENC_A} } },
  { INPUT_MATRIX, { .matrix = {3, 0, 4, ENC_B} } },

  // Shift register button: regIndex, bitIndex, joystick button ID, behavior
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 5, NORMAL} } },

  // Shift register encoder: two adjacent bits, each mapped to a joystick button
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 6, ENC_A} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 7, ENC_B} } }
};
```

**Behavior options:**
- `NORMAL`: Standard button (press/release)
- `MOMENTARY`: Sends a quick pulse on press
- `ENC_A`, `ENC_B`: Used for rotary encoder channels (must be paired as A/B)

---

### 4. Analog Axes Configuration

Configure analog axes in `src/ConfigAxis.h`. Uncomment and modify the axis definitions you want to use:

```cpp
// X-Axis (Main stick pitch)
#define USE_AXIS_X
#ifdef USE_AXIS_X
    #define AXIS_X_PIN              A0
    #define AXIS_X_MIN              0
    #define AXIS_X_MAX              1023
    #define AXIS_X_FILTER_LEVEL     AXIS_FILTER_MEDIUM
    #define AXIS_X_CURVE            CURVE_LINEAR
#endif
```

**Available axes:** X, Y, Z, RX, RY, RZ, S1, S2

---

### 5. Example: Adding a New Encoder

To add a rotary encoder on pins 8 and 9, mapped to joystick buttons 10 (CW) and 11 (CCW):

```cpp
// In hardwarePinMap:
{"8", BTN}, {"9", BTN},

// In logicalInputs:
{ INPUT_PIN, { .pin = {8, 10, ENC_A} } },
{ INPUT_PIN, { .pin = {9, 11, ENC_B} } },
```

---

## 🚦 Build & Upload

1. **Clone the repo** and open in Arduino IDE or PlatformIO.
2. **Install dependencies**:
   - [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)
   - [RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)
3. **Edit your config files** to match your hardware:
   - `src/ConfigDigital.h` for buttons and encoders
   - `src/ConfigAxis.h` for analog axes
4. **Upload** to your Arduino board.

---

## 💡 Tips

- Use unique joystick button IDs for each input.
- For encoders, always define A and B channels as consecutive entries.
- You can mix matrix, direct, and shift register inputs freely.
- For axes, you can use onboard analog pins for all 8 axes.
- Matrix encoders work by reading pin states during matrix scanning.

---

## 🙏 Credits

- **[Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)**
- **[RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)**
- **[Keypad Library](https://playground.arduino.cc/Code/Keypad/)**

Big thanks to the authors of these libraries for their incredible work. Many features here are based on, expanded from, and inspired by their approaches.
