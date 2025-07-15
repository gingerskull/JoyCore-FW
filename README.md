# JoyCore

**JoyCore** is an Arduino-based USB game controller firmware. It supports matrix buttons, rotary encoders, direct pin buttons, shift register inputs, and high-resolution analog axes via ADS1115, letting you build a highly customizable button box or control panel.

---

## 🖥️ Supported Boards

JoyCore is designed for basically any board based on the ATmega32U4 microcontroller, including:

**✅ Tested:**
- [SparkFun Pro Micro](https://www.sparkfun.com/products/12640)

**⚠️ Untested (should work, but not verified):**
- [Arduino Leonardo](https://store.arduino.cc/products/arduino-leonardo)
- [Arduino Micro](https://store.arduino.cc/products/arduino-micro)
- [Adafruit ItsyBitsy 32u4](https://www.adafruit.com/product/3677)
- [Adafruit Feather 32u4](https://www.adafruit.com/product/2771)

> **Note:** Only the SparkFun Pro Micro has been tested so far. The other boards are supported in theory but I have not tested them yet.

---

## ✨ Features

- **Matrix Button Scanning**: Efficient row/column multiplexing for large button arrays.
- **Rotary Encoders**: Handles both direct/matrix and shift register-based encoders.
- **Shift Registers**: Expand inputs with [74HC165](https://www.ti.com/lit/ds/symlink/sn74hc165.pdf) chips.
- **High-Resolution Analog Axes**: Up to 4 axes via ADS1115 16-bit I2C ADC.
- **USB HID Game Controller**: Native USB interface via the [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary).

---

## 🛠️ Hardware Setup

### 1. Pin Mapping

Edit your pin mapping in `src/UserConfig.h` (or `src/ConfigAxis.h` for axes):

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

Set the number of chained 74HC165 chips:
```cpp
#define SHIFTREG_COUNT    1  // Number of 74HC165 chips
```

---

### 3. Logical Inputs

Define your logical inputs in the `logicalInputs` array. Each entry describes a button or encoder and how it maps to the joystick.

```cpp
constexpr LogicalInput logicalInputs[] = {
  // Matrix buttons: row, col, joystick button ID, behavior
  { LOGICAL_MATRIX,  { .matrix = {0, 0, 24, NORMAL} }, SRC_MATRIX },
  // ...
  // Shift register button: regIndex, bitIndex, joystick button ID, behavior
  { LOGICAL_BTN, { .shiftreg = {0, 0, 5, NORMAL} }, SRC_SHIFTREG },
  // ...
};
```
**Behavior options:**  
- `NORMAL`: Standard button (press/release)  
- `MOMENTARY`: Sends a quick pulse on press  
- `ENC_A`, `ENC_B`: Used for rotary encoder channels (must be paired as A/B)

---

### 4. Example: Adding a New Encoder

To add a rotary encoder on pins 8 and 9, mapped to joystick buttons 10 (CW) and 11 (CCW):

```cpp
// In hardwarePinMap:
{"8", BTN}, {"9", BTN},

// In logicalInputs:
{ LOGICAL_BTN, { .btn = {8, 10, ENC_A} }, SRC_PIN },
{ LOGICAL_BTN, { .btn = {9, 11, ENC_B} }, SRC_PIN },
```

---

## 🎚️ High-Resolution Analog Axes with ADS1115

JoyCore supports using an ADS1115 16-bit I2C ADC for up to 4 high-resolution analog axes.

### Wiring (Arduino Pro Micro / Leonardo)
- Connect ADS1115 **SDA** to Arduino **SDA** (Pin 2)
- Connect ADS1115 **SCL** to Arduino **SCL** (Pin 3)
- Connect **VCC** and **GND** as usual

### Configuration
- In `ConfigAxis.h`, set the axis pin to one of the following to use an ADS1115 channel:
  - `ADS1115_CH0` (channel 0)
  - `ADS1115_CH1` (channel 1)
  - `ADS1115_CH2` (channel 2)
  - `ADS1115_CH3` (channel 3)
- Example:
  ```cpp
  #define AXIS_X_PIN ADS1115_CH0
  ```
- All other configuration (filtering, range, etc.) works as before.

### Notes
- The ADS1115 is automatically initialized on first use.
- You can mix onboard analog pins and ADS1115 channels for different axes.
- No code changes are needed for I2C pin selection; the standard Wire library uses the hardware I2C pins by default.

---

## 🚦 Build & Upload

1. **Clone the repo** and open in Arduino IDE or PlatformIO.
2. **Install dependencies**:
   - [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)
   - [RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)
   - [Adafruit ADS1X15 Library](https://github.com/adafruit/Adafruit_ADS1X15)
3. **Edit your config files** to match your hardware.
4. **Upload** to your Arduino board.

---

## 💡 Tips

- Use unique joystick button IDs for each input.
- For encoders, always define A and B channels as consecutive entries.
- You can mix matrix, direct, and shift register inputs freely.
- For axes, you can use both onboard analog pins and ADS1115 channels in any combination.

---

## 🙏 Credits

- **[Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)**
- **[RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)**
- **[Keypad Library](https://playground.arduino.cc/Code/Keypad/)**
- **[Adafruit ADS1X15 Library](https://github.com/adafruit/Adafruit_ADS1X15)**

Big thanks to the authors of these libraries for their incredible work. Many features here are based on, expanded from, and inspired by their approaches.

---