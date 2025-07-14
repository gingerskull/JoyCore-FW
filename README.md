# GNGR-ButtonBox

## Teensy 4.0 Port

**⚠️ This is the Teensy 4.0 port branch - for the original Arduino Pro Micro version, see the `main` branch.**

GNGR-ButtonBox is a **Teensy 4.0-based** USB game controller for flight simulator enthusiasts. This port takes advantage of Teensy 4.0's superior USB HID capabilities for enhanced performance and reliability. It supports matrix buttons, rotary encoders, direct pin buttons, and shift register inputs, letting you build a highly customizable button box for your cockpit.

### Advantages of Teensy 4.0 Port

✅ **Superior USB Performance**: Teensy 4.0's native USB HID implementation is more robust and faster than Arduino Leonardo's PluggableUSB system

✅ **Simplified Architecture**: No need for complex DynamicHID implementation - uses Teensy's built-in joystick library

✅ **Better Reliability**: Teensy's USB stack is more stable and handles reconnections better

✅ **More Processing Power**: 600MHz ARM Cortex-M7 vs 16MHz ATmega32U4

✅ **More Memory**: 1MB Flash + 512KB RAM vs 32KB Flash + 2.5KB RAM

✅ **More I/O Pins**: More pins available for complex button matrices and encoders

### Hardware Requirements

- **Teensy 4.0** (not compatible with Arduino Pro Micro/Leonardo)
- Compatible with all existing button box hardware designs
- Same pin mapping concepts apply

---

## ✨ Features

- **Matrix Button Scanning**: Efficient row/column multiplexing for large button arrays.
- **Rotary Encoders**: Handles both direct/matrix and shift register-based encoders.
- **Shift Registers**: Expand inputs with [74HC165](https://www.ti.com/lit/ds/symlink/sn74hc165.pdf) chips.
- **USB HID Game Controller**: Native USB interface via Teensy's built-in joystick library.

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

### 2. Teensy 4.0 Pin Considerations

Teensy 4.0 has different pin numbering than Arduino Pro Micro. Key differences:
- Digital pins: 0-23, plus additional pins 24-33
- Analog pins: A0-A9 (pins 14-23), A10-A13 (pins 24-27)
- More interrupt-capable pins available
- 3.3V logic levels (5V tolerant on most pins)

---

### 3. Shift Register Configuration

Set the number of chained 74HC165 chips:

```cpp
#define SHIFTREG_COUNT    1  // Number of 74HC165 chips
```

---

### 4. Logical Inputs

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

### 5. Example: Adding a New Encoder

To add a rotary encoder on pins 8 and 9, mapped to joystick buttons 10 (CW) and 11 (CCW):

```cpp
// In hardwarePinMap:
{"8", BTN}, {"9", BTN},

// In logicalInputs:
{ LOGICAL_BTN, { .btn = {8, 10, ENC_A} }, SRC_PIN },
{ LOGICAL_BTN, { .btn = {9, 11, ENC_B} }, SRC_PIN },
```

---

### 6. Build & Upload (Teensy 4.0)

1. **Install PlatformIO** or use Arduino IDE with Teensyduino
2. **Hardware setup**: Connect your Teensy 4.0
3. **Build configuration**: This project uses PlatformIO with Teensy 4.0 target
4. **Edit `UserConfig.h`** to match your hardware
5. **Upload** to your Teensy 4.0

#### PlatformIO Commands:
```bash
# Build the project
pio run

# Upload to Teensy 4.0
pio run --target upload

# Monitor serial output
pio device monitor
```

---

## 🔧 Technical Differences from Arduino Version

### USB Implementation
- **Arduino Leonardo**: Uses custom DynamicHID implementation with PluggableUSB
- **Teensy 4.0**: Uses native Teensyduino joystick library (much simpler and more reliable)

### Performance
- **Arduino Leonardo**: 16MHz ATmega32U4, limited memory
- **Teensy 4.0**: 600MHz ARM Cortex-M7, abundant memory and processing power

### USB Descriptors
- **Arduino Leonardo**: Custom USB HID descriptors required
- **Teensy 4.0**: Automatic USB descriptor generation

### Reliability
- **Arduino Leonardo**: Occasional USB enumeration issues
- **Teensy 4.0**: Rock-solid USB implementation with better host compatibility

---

## 🧩 Tips

- Use unique joystick button IDs for each input.
- For encoders, always define A and B channels as consecutive entries.
- You can mix matrix, direct, and shift register inputs freely.
- Teensy 4.0's 3.3V logic is compatible with most 5V devices (check your specific components)

---

## 🛠️ Dependencies

- **PlatformIO**: Development platform
- **Teensy 4.0**: Target hardware
- **Teensyduino**: Arduino-compatible framework for Teensy (included with PlatformIO Teensy platform)

---

## 🛠️ Credit

- **[Teensy](https://www.pjrc.com/)** - Excellent microcontroller with superior USB support
- **[RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)** 
- **[Keypad Library](https://playground.arduino.cc/Code/Keypad/)** 

Big thanks to PJRC for creating Teensy and its outstanding USB implementation. The original Arduino Leonardo version was based on the Arduino Joystick Library and related components - their work made this project possible.

---

## 📋 Migration from Arduino Leonardo

If you're upgrading from the Arduino Leonardo version:

1. **Hardware**: Replace Arduino Pro Micro/Leonardo with Teensy 4.0
2. **Pin mapping**: Update pin numbers in `UserConfig.h` for Teensy 4.0 layout
3. **Voltage levels**: Ensure 3.3V compatibility (most components work fine)
4. **Upload**: Use Teensy Loader or PlatformIO Teensy platform
5. **Performance**: Enjoy much better USB stability and performance!

The logical configuration remains the same - only the hardware platform changes.