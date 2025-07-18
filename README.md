# JoyCore-FW

## RP2040 Native Implementation

**⚠️ This is the RP2040 native implementation branch - for the original Arduino Pro Micro version, see the `main` branch. For the Teensy 4.0 version, see the `teensy40` branch.**

**RP2040-based** USB game controller firmware. This port supports matrix buttons, rotary encoders, direct pin buttons, shift register inputs, and analog axes.

![RP2040 Pico Pinout](images/rp2040-pico-pinout.png)

### Advantages of RP2040 Port

✅ **Dual-Core Processing**: Dual ARM Cortex-M0+ cores at 133MHz

✅ **PIO Support**: Programmable I/O for hardware-accelerated operations

✅ **Cost-Effective**: More affordable than Teensy 4.0 (~$4-8 vs ~$20-25)

✅ **Memory**: 264KB SRAM + 2MB Flash (typical)

✅ **Wide Board Support**: Works with Raspberry Pi Pico, Adafruit Feather RP2040, and many others

✅ **Native USB**: Uses rp2040-HID library for optimal performance

### Hardware Requirements

- **RP2040-based board** (Raspberry Pi Pico, Adafruit Feather RP2040, etc.)
- Compatible with all existing button box hardware designs
- Same pin mapping concepts apply

⚠️ **Important**: RP2040 pins are NOT 5V tolerant. Use level shifters if interfacing with 5V devices.

---

## Features

- **Matrix Button Scanning**: Row/column multiplexing for button arrays
- **Rotary Encoders**: Direct pin, matrix, and shift register-based encoders
- **Shift Registers**: Expand inputs with [74HC165](https://www.ti.com/lit/ds/symlink/sn74hc165.pdf) chips
- **Analog Inputs**: Built-in analog pins and ADS1115 16-bit ADC support
- **USB HID Game Controller**: Native USB interface via rp2040-HID library
- **Signal Processing**: Configurable noise filtering and response curves for analog axes
- **Configuration**: Same format as Arduino/Teensy versions

---

## Quick Start

### 1. Hardware Setup

Connect your RP2040 board and configure the pin mapping in `src/ConfigDigital.h`:

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
};
```

### 2. Configure Inputs

Define your logical inputs in `src/ConfigDigital.h`:

```cpp
constexpr LogicalInput logicalInputs[] = {
  // Matrix buttons
  { INPUT_MATRIX, { .matrix = {0, 0, 1, NORMAL} } },
  { INPUT_MATRIX, { .matrix = {1, 0, 2, NORMAL} } },
  
  // Matrix encoders
  { INPUT_MATRIX, { .matrix = {2, 0, 3, ENC_A} } },
  { INPUT_MATRIX, { .matrix = {3, 0, 4, ENC_B} } },
  
  // Shift register inputs
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 5, NORMAL} } }
};
```

### 3. Configure Analog Axes

Set up analog axes in `src/ConfigAxis.h`:

```cpp
// Built-in analog pin (RP2040: GPIO26-GPIO28)
#define AXIS_X_PIN 26

// ADS1115 channel
#define AXIS_X_PIN ADS1115_CH0  // 16-bit resolution
```

---

## 📋 Configuration Guide

### Hardware Pin Mapping

Edit `src/ConfigDigital.h` to match your hardware. Each entry defines a pin and its function:

**Pin Types:**
- `BTN_ROW`, `BTN_COL`: For matrix scanning
- `SHIFTREG_PL`, `SHIFTREG_CLK`, `SHIFTREG_QH`: For shift register chips
- `BTN`: For direct-wired buttons

### RP2040 Pin Considerations

RP2040 has different pin numbering and capabilities than other boards:

| Feature | Arduino Pro Micro | Teensy 4.0 | RP2040 | Notes |
|---------|------------------|------------|--------|-------|
| Digital I/O | 0-21 | 0-23, 24-33 | GPIO0-GPIO28 | GPIO numbering |
| Analog Input | A0-A3, A6-A10 | A0-A13 | GPIO26-GPIO28 | Limited analog pins |
| PWM | Limited | Many pins | Most pins | PWM support |
| Interrupts | 2, 3, 7 | Most pins | All pins | Interrupt options |
| I2C | 2, 3 | 18, 19 | GPIO4/5 (default) | For ADS1115 |

**RP2040 Analog Pin Limitations:**
- Only 3 built-in analog pins: GPIO26, GPIO27, GPIO28
- For more analog inputs, use external ADS1115 ADC
- 12-bit resolution (0-4095) vs 10-bit on other boards

### Shift Register Configuration

Set the number of chained 74HC165 chips:

```cpp
#define SHIFTREG_COUNT    1  // Number of 74HC165 chips
```

### Analog Axis Configuration

Configure analog axes in `src/ConfigAxis.h`. You can use built-in analog pins or ADS1115 channels:

**Built-in Analog Pins (RP2040):**
- `GPIO26` - Built-in ADC (12-bit resolution)
- `GPIO27` - Built-in ADC (12-bit resolution) 
- `GPIO28` - Built-in ADC (12-bit resolution)

**ADS1115 Channels:**
- `ADS1115_CH0` - Channel 0 (pins 0-1)
- `ADS1115_CH1` - Channel 1 (pins 2-3) 
- `ADS1115_CH2` - Channel 2 (pins 4-5)
- `ADS1115_CH3` - Channel 3 (pins 6-7)

**ADS1115 Wiring (RP2040):**
- SDA → GPIO 4 (Pin 6 on Pico)
- SCL → GPIO 5 (Pin 7 on Pico)
- VCC → 3.3V
- GND → GND

The ADS1115 is automatically initialized when any axis uses ADS1115 channels.

**Filter Options:**
- `AXIS_FILTER_OFF` - No filtering (raw values)
- `AXIS_FILTER_LOW` - Light filtering
- `AXIS_FILTER_MEDIUM` - Moderate filtering (default)
- `AXIS_FILTER_HIGH` - Heavy filtering

**Response Curves:**
- `CURVE_LINEAR` - Linear response (1:1 mapping)
- `CURVE_S_CURVE` - S-curve (gentle at center, steeper at edges)
- `CURVE_EXPONENTIAL` - Exponential (gentle at start, steep at end)
- `CURVE_CUSTOM` - Custom curve (define your own)

### Logical Inputs

Define your logical inputs in the `logicalInputs` array:

**Behavior options:**
- `NORMAL`: Standard button (press/release)
- `MOMENTARY`: Sends a quick pulse on press
- `ENC_A`, `ENC_B`: Used for rotary encoder channels (must be paired as A/B)

### 4. Build & Upload

```bash
# Build the project
pio run

# Upload to RP2040
pio run --target upload
```

---

## 🔧 Technical Differences Between Branches

### USB Implementation
- **Arduino Leonardo (main branch)**: Uses custom DynamicHID implementation with PluggableUSB
- **Teensy 4.0 (teensy40 branch)**: Uses native Teensyduino joystick library
- **RP2040 (this branch)**: Uses rp2040-HID library with native TinyUSB stack

### Performance Comparison

| Metric | Arduino Pro Micro | Teensy 4.0 | RP2040 | Notes |
|--------|------------------|------------|--------|-------|
| CPU Speed | 16MHz ATmega32U4 | 600MHz ARM Cortex-M7 | 133MHz ARM Cortex-M0+ | Dual-core on RP2040 |
| Memory | 32KB Flash + 2.5KB RAM | 1MB Flash + 512KB RAM | 2MB Flash + 264KB RAM | Generous memory on all |
| Loop Speed | ~1kHz typical | >10kHz possible | >5kHz typical | Fast enough for all use cases |
| Button Response | <10ms | <1ms | <2ms | All platforms responsive |
| USB Latency | Variable | Consistent | Very consistent | rp2040-HID is excellent |
| Cost | ~$8-12 | ~$20-25 | ~$4-8 | RP2040 is most cost-effective |

### USB Descriptors
- **Arduino Leonardo**: Custom USB HID descriptors required
- **Teensy 4.0**: Automatic USB descriptor generation
- **RP2040**: Standard HID gamepad with rp2040-HID

### Reliability
- **Arduino Leonardo**: Occasional USB enumeration issues
- **Teensy 4.0**: Stable USB implementation with better host compatibility
- **RP2040**: Excellent USB reliability with rp2040-HID library

---

## 🔄 Migration Between Branches

### Platform Selection Guide

**Choose RP2040 if:**
- Cost is a primary concern
- You need excellent USB reliability
- You want to leverage PIO for advanced features
- You're building multiple units

**Choose Teensy 4.0 if:**
- You need maximum processing power
- You have complex analog processing requirements
- You need the most I/O pins
- Performance is critical

**Choose Arduino Pro Micro if:**
- You have existing hardware designed for it
- You need 5V logic compatibility
- You're on a very tight budget with existing parts

### Configuration Compatibility

✅ **Same configuration format**: `ConfigAxis.h` and `ConfigDigital.h` work identically across platforms  
✅ **Same ADS1115 support**: Same channel definitions and automatic initialization  
✅ **Same axis processing**: Identical filtering and curve algorithms  
✅ **Same logical input structure**: Matrix, direct pin, and shift register configurations  

The configuration format remains the same - only the hardware platform and pin numbers change.

### Pin Migration

When migrating between platforms, update pin numbers in `ConfigDigital.h`:

```cpp
// Arduino Pro Micro
{"2", BTN_ROW},    // Pin 2

// Teensy 4.0  
{"2", BTN_ROW},    // Pin 2 (same number)

// RP2040
{"2", BTN_ROW},    // GPIO 2 (same number, different naming)
```

### Configuration Utility Compatibility (**FUTURE**)

A configuration utility would work with all platforms because:
- Same configuration format and structure
- Same ADS1115 channel definitions (`ADS1115_CH0-3`)
- Same axis processing logic and filter options
- Same pin assignment system

---

## Dependencies

- **PlatformIO**: Development platform
- **RP2040**: Target hardware (Raspberry Pi Pico, Adafruit Feather RP2040, etc.)
- **Arduino-Pico**: Arduino-compatible framework for RP2040
- **rp2040-HID**: Native HID implementation for RP2040
- **Adafruit ADS1X15**: For analog input support

---

## Credit

- **[Raspberry Pi Foundation](https://www.raspberrypi.org/)** - RP2040 microcontroller
- **[rp2040-HID](https://github.com/earlephilhower/arduino-pico)** - Native HID implementation
- **[RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)** - Modified and integrated
- **[Keypad Library](https://playground.arduino.cc/Code/Keypad/)** - Replaced with built-in implementation

---

## Troubleshooting

### Common Issues
1. **Pin numbering**: RP2040 uses GPIO numbering (0-28) instead of Arduino pin numbers
2. **Analog pins**: Only 3 built-in analog pins available (GPIO26-28)
3. **Voltage levels**: Ensure 3.3V compatibility of your components
4. **USB type**: Uses rp2040-HID with automatic HID descriptor generation

### Debug Options
- **Serial Monitor**: Available for debugging
- **USB Descriptor**: Automatically handled by rp2040-HID
- **LED Indicator**: Use built-in LED for status indication

---

## Tips

- Use unique joystick button IDs for each input
- For encoders, always define A and B channels as consecutive entries
- You can mix matrix, direct, and shift register inputs freely
- RP2040's 3.3V logic is compatible with most 5V devices (check your specific components)
- The ADS1115 provides 16-bit resolution vs 12-bit for built-in analog pins
- Configuration is identical between branches - only hardware platform differs
- RP2040's limited analog pins make ADS1115 more valuable for complex setups