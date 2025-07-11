# GNGR-ButtonBox

GNGR-ButtonBox is an Arduino-based USB game controller for flight simulator enthusiasts. It supports matrix buttons, rotary encoders, direct pin buttons, and shift register inputs, letting you build a highly customizable button box for your cockpit.

---

## ✨ Features

- **Matrix Button Scanning**: Efficient row/column multiplexing for large button arrays.
- **Rotary Encoders**: Handles both direct/matrix and shift register-based encoders.
- **Shift Registers**: Expand inputs with [74HC165](https://www.ti.com/lit/ds/symlink/sn74hc165.pdf) chips.
- **USB HID Game Controller**: Native USB interface via the [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary).


---

## 🛠️ Libraries


- **[Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)**
- **[Keypad Library](https://playground.arduino.cc/Code/Keypad/)**
- **[RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)** (customized for shift register support)


---

## 🚀 How to Use

### 1. Hardware Pin Mapping

Edit [`src/UserConfig.h`](src/UserConfig.h) to match your hardware. Each entry in `hardwarePinMap` defines a pin and its function:

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
  { LOGICAL_MATRIX,  { .matrix = {1, 0, 25, NORMAL} }, SRC_MATRIX },

  // Matrix encoder: adjacent matrix positions, each with a unique joystick button
  { LOGICAL_MATRIX,  { .matrix = {2, 0, 26, ENC_A} }, SRC_MATRIX },
  { LOGICAL_MATRIX,  { .matrix = {3, 0, 27, ENC_B} }, SRC_MATRIX },

  // Shift register button: regIndex, bitIndex, joystick button ID, behavior
  { LOGICAL_BTN, { .shiftreg = {0, 0, 5, NORMAL} }, SRC_SHIFTREG },

  // Shift register encoder: two adjacent bits, each mapped to a joystick button
  { LOGICAL_BTN, { .shiftreg = {0, 1, 6, ENC_A} }, SRC_SHIFTREG },
  { LOGICAL_BTN, { .shiftreg = {0, 2, 7, ENC_B} }, SRC_SHIFTREG }
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

### 5. Build & Upload

1. **Clone the repo** and open in Arduino IDE.
2. **Install dependencies**:
   - [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)
   - [Keypad Library](https://playground.arduino.cc/Code/Keypad/)
   - [RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)
3. **Edit `UserConfig.h`** to match your hardware.
4. **Upload** to your Arduino board.

---

## 🧩 Tips

- Use unique joystick button IDs for each input.
- For encoders, always define A and B channels as consecutive entries.
- You can mix matrix, direct, and shift register inputs freely.
