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
- **Advanced Signal Processing**: Multi-stage filtering with noise reduction, velocity-adaptive smoothing or EWMA filtering, and intelligent deadband
- **Configuration**: Same format as Arduino/Teensy versions

---

## Quick Start

### 1. Hardware Setup

Connect your RP2040 board and configure the pin mapping in `src/ConfigDigital.h`:

```cpp
static const PinMapEntry hardwarePinMap[] = {
  // Direct button inputs (pins 4-13 in current config)
  {"4", BTN},   // Button 9 (MOMENTARY)
  {"5", BTN},   // Button 10 (MOMENTARY)
  {"6", BTN},   // Button 1 (NORMAL)
  {"7", BTN},   // Button 2 (NORMAL)
  {"8", BTN},   // Button 3 (MOMENTARY)
  {"9", BTN},   // Button 4 (MOMENTARY)
  {"10", BTN},  // Button 5 (NORMAL)
  {"11", BTN},  // Button 6 (NORMAL)
  {"12", BTN},  // Button 7 (NORMAL)
  {"13", BTN},  // Button 8 (NORMAL)
  
  // 74HC165 shift register control pins
  {"18", SHIFTREG_QH},   // Serial data out pin
  {"19", SHIFTREG_PL},   // Parallel load pin
  {"20", SHIFTREG_CLK}   // Clock pin
};
```

### 2. Configure Inputs

Define your logical inputs in `src/ConfigDigital.h`:

```cpp
constexpr LogicalInput logicalInputs[] = {
  // Direct pin buttons (pins 4-13 mapped to joystick buttons 1-10)
  { INPUT_PIN, { .pin = {4, 9, MOMENTARY, 0} } },    // Pin 4 -> Button 9
  { INPUT_PIN, { .pin = {5, 10, MOMENTARY, 0} } },   // Pin 5 -> Button 10
  { INPUT_PIN, { .pin = {6, 1, NORMAL, 0} } },       // Pin 6 -> Button 1
  { INPUT_PIN, { .pin = {7, 2, NORMAL, 0} } },       // Pin 7 -> Button 2
  { INPUT_PIN, { .pin = {8, 3, MOMENTARY, 0} } },    // Pin 8 -> Button 3
  
  // Shift register inputs (Register 0, bits 0-7)
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 11, NORMAL, 0} } },     // Reg 0, bit 0 -> Button 11
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 12, NORMAL, 0} } },     // Reg 0, bit 1 -> Button 12
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 13, ENC_A, 0} }, FOUR0 }, // Encoder A
  { INPUT_SHIFTREG, { .shiftreg = {0, 3, 14, ENC_B, 0} }, FOUR0 }, // Encoder B
};
```

### 3. Configure Analog Axes

Set up analog axes in `src/ConfigAxis.h`:

```cpp
// X-Axis with advanced processing
#define USE_AXIS_X
#ifdef USE_AXIS_X
    #define AXIS_X_PIN              A1              // Built-in analog pin
    #define AXIS_X_MIN              0               // Input range minimum
    #define AXIS_X_MAX              32767           // Input range maximum
    #define AXIS_X_FILTER_LEVEL     AXIS_FILTER_EWMA // Filter type
    #define AXIS_X_EWMA_ALPHA       200             // EWMA alpha (0-1000)
    #define AXIS_X_DEADBAND         250             // Deadband size
    #define AXIS_X_CURVE            CURVE_LINEAR    // Response curve
#endif

// Y-Axis with built-in analog input
#define USE_AXIS_Y
#ifdef USE_AXIS_Y
    #define AXIS_Y_PIN              A2              // Built-in analog pin
    #define AXIS_Y_MIN              0
    #define AXIS_Y_MAX              32767
    #define AXIS_Y_FILTER_LEVEL     AXIS_FILTER_EWMA
    #define AXIS_Y_EWMA_ALPHA       200
    #define AXIS_Y_DEADBAND         250
    #define AXIS_Y_CURVE            CURVE_LINEAR
#endif

// Z-Axis with ADS1115 high-resolution input (example)
// #define USE_AXIS_Z
#ifdef USE_AXIS_Z
    #define AXIS_Z_PIN              ADS1115_CH0     // 16-bit ADC channel
    #define AXIS_Z_MIN              0
    #define AXIS_Z_MAX              32767
    #define AXIS_Z_FILTER_LEVEL     AXIS_FILTER_MEDIUM
    #define AXIS_Z_EWMA_ALPHA       30
    #define AXIS_Z_DEADBAND         0
    #define AXIS_Z_CURVE            CURVE_LINEAR
#endif
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

| Feature | Arduino Pro Micro | Teensy 4.0 | RP2040 |
|---------|------------------|------------|--------|
| Digital I/O | 0-21 | 0-23, 24-33 | GPIO0-GPIO28 |
| Analog Input | A0-A3, A6-A10 | A0-A13 | GPIO26-GPIO28 |
| PWM | Limited | Many pins | Most pins |
| Interrupts | 2, 3, 7 | Most pins | All pins |
| I2C | 2, 3 | 18, 19 | GPIO4/5 (default) |

**RP2040 Analog Pin Limitations:**
- Only 3 built-in analog pins: GPIO26, GPIO27, GPIO28
- For more analog inputs, use external ADS1115 ADC
- 12-bit resolution (0-4095) vs 10-bit on other boards

### Shift Register Configuration

Set the number of chained 74HC165 chips:

```cpp
#define SHIFTREG_COUNT    2  // Number of 74HC165 chips (current config uses 2)
```

### Analog Axis Configuration

Configure analog axes in `src/ConfigAxis.h`. You can use built-in analog pins or ADS1115 channels:

**Built-in Analog Pins (RP2040):**
- `A1` (GPIO27) - Built-in ADC (12-bit resolution)
- `A2` (GPIO28) - Built-in ADC (12-bit resolution) 
- `A3` (GPIO29) - Built-in ADC (12-bit resolution)

**ADS1115 Channels:**
- `ADS1115_CH0` - Channel 0 (pins 0-1)
- `ADS1115_CH1` - Channel 1 (pins 2-3) 
- `ADS1115_CH2` - Channel 2 (pins 4-5)
- `ADS1115_CH3` - Channel 3 (pins 6-7)

**ADS1115 Wiring (RP2040):**
- SDA → GPIO 4 (Pin 6 on Pico) - Default I2C0 SDA
- SCL → GPIO 5 (Pin 7 on Pico) - Default I2C0 SCL
- VCC → 3.3V (3V3 pin)
- GND → GND
- ADDR → GND (for I2C address 0x48)

The ADS1115 is automatically initialized when any axis uses ADS1115 channels.

**Advanced Filter Options:**
- `AXIS_FILTER_OFF` - No filtering (raw values pass through)
- `AXIS_FILTER_LOW` - Light filtering for high-precision controls
- `AXIS_FILTER_MEDIUM` - Moderate filtering for general use (default)
- `AXIS_FILTER_HIGH` - Heavy filtering for noisy or low-quality sensors
- `AXIS_FILTER_EWMA` - EWMA (Exponentially Weighted Moving Average) filtering

**EWMA Filtering:**
- Memory-efficient smoothing algorithm
- Configurable alpha parameter (0-1000, scaled by 1000)
- Higher alpha = less smoothing, more responsive
- Lower alpha = more smoothing, less responsive
- Common values: 30 (0.03), 100 (0.1), 200 (0.2), 500 (0.5)

**Intelligent Deadband:**
- Prevents jitter when control is at rest
- Works around current axis position, not fixed center
- Uses statistical analysis to avoid interfering with slow movements
- Configurable settle duration (default: 150ms)
- Typical values: 0 (off), 500-1000 (light), 1000-2000 (medium), 2000-5000 (heavy)

**Response Curves:**
- `CURVE_LINEAR` - Linear response (1:1 mapping)
- `CURVE_S_CURVE` - S-curve (gentle at center, steeper at edges)
- `CURVE_EXPONENTIAL` - Exponential (gentle at start, steep at end)
- `CURVE_CUSTOM` - Custom curve (define your own lookup table)

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

# Upload to RP2040 (put board in bootloader mode first)
pio run --target upload

# Or upload with specific environment
pio run -e rp2040 --target upload
```

---

## 🔧 Technical Differences Between Branches

### USB Implementation
- **Arduino Leonardo (main branch)**: Uses custom DynamicHID implementation with PluggableUSB
- **Teensy 4.0 (teensy40 branch)**: Uses native Teensyduino joystick library
- **RP2040 (this branch)**: Uses rp2040-HID library with native TinyUSB stack

### Performance Comparison

| Metric | Arduino Pro Micro | Teensy 4.0 | RP2040 |
|--------|------------------|------------|--------|
| CPU Speed | 16MHz ATmega32U4 | 600MHz ARM Cortex-M7 | 133MHz ARM Cortex-M0+ |
| Memory | 32KB Flash + 2.5KB RAM | 1MB Flash + 512KB RAM | 2MB Flash + 264KB RAM |
| Loop Speed | ~1kHz typical | >10kHz possible | >5kHz typical |
| Button Response | <10ms | <1ms | <2ms |
| USB Latency | Variable | Consistent | Very consistent |
| Cost | ~$8-12 | ~$20-25 | ~$4-8 |

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
✅ **Enhanced axis processing**: Advanced multi-stage filtering with EWMA, intelligent deadband, and velocity-adaptive smoothing  
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
{"4", BTN},        // GPIO 4 (direct button, not matrix in current config)
```

### Configuration Utility Compatibility (**FUTURE**)

A configuration utility would work with all platforms because:
- Same configuration format and structure
- Same ADS1115 channel definitions (`ADS1115_CH0-3`)
- Same axis processing logic and filter options
- Same pin assignment system

---

## 🔧 Advanced Axis Processing

The RP2040 implementation includes sophisticated signal processing capabilities designed for professional-grade control systems:

### Multi-Stage Processing Chain

```
Raw Input → Deadband → Noise Filter → Velocity-Adaptive Smoothing → Response Curve → Output
```

**1. Intelligent Deadband**
- Prevents jitter when control is at rest
- Uses rolling average analysis to detect settled state
- Works around current position, not fixed center point
- Configurable settle duration (default: 150ms)

**2. Noise Filtering**
- Configurable noise threshold (0-10 typical)
- Ignores small changes below threshold
- Maintains responsiveness during active control

**3. Velocity-Adaptive Smoothing**
- Calculates movement velocity in real-time
- Reduces smoothing during fast movements for responsiveness
- Increases smoothing during slow movements for stability
- Emergency pass-through for very fast movements

**4. EWMA Filtering**
- Memory-efficient Exponentially Weighted Moving Average
- Integer-only arithmetic (no floating point)
- Configurable alpha parameter for fine-tuning
- Ideal for consistent timing applications

**5. Response Curves**
- Linear, S-curve, exponential, and custom curves
- Lookup table interpolation for smooth transitions
- Custom curves support up to 11 control points

### Configuration Examples

**High-Precision Control (Flight Stick):**
```cpp
#define AXIS_X_PIN              A1              // Built-in analog pin
#define AXIS_X_FILTER_LEVEL     AXIS_FILTER_EWMA
#define AXIS_X_EWMA_ALPHA       25              // Heavy smoothing
#define AXIS_X_DEADBAND         500             // Light deadband
#define AXIS_X_CURVE            CURVE_S_CURVE   // Gentle center
```

**Responsive Control (Racing Wheel):**
```cpp
#define AXIS_Y_PIN              A2              // Built-in analog pin
#define AXIS_Y_FILTER_LEVEL     AXIS_FILTER_LOW
#define AXIS_Y_EWMA_ALPHA       200             // Light smoothing
#define AXIS_Y_DEADBAND         0               // No deadband
#define AXIS_Y_CURVE            CURVE_LINEAR    // Direct response
```

**High-Resolution External ADC:**
```cpp
#define AXIS_Z_PIN              ADS1115_CH0     // 16-bit external ADC
#define AXIS_Z_FILTER_LEVEL     AXIS_FILTER_HIGH
#define AXIS_Z_EWMA_ALPHA       30              // Heavy smoothing
#define AXIS_Z_DEADBAND         1000            // Medium deadband
#define AXIS_Z_CURVE            CURVE_LINEAR    // Simple curve
```

---

## Dependencies

- **PlatformIO**: Development platform
- **RP2040**: Target hardware (Raspberry Pi Pico, Adafruit Feather RP2040, etc.)
- **Arduino-Pico**: Arduino-compatible framework for RP2040 (framework-arduino-mbed)
- **USBHID**: Native USB HID implementation for RP2040
- **Adafruit ADS1X15**: For high-resolution analog input support (16-bit ADC)

---

## Credit

- **[Raspberry Pi Foundation](https://www.raspberrypi.org/)** - RP2040 microcontroller
- **[rp2040-HID](https://github.com/earlephilhower/arduino-pico)** - Native HID implementation
- **[RotaryEncoder Library](https://github.com/mathertel/RotaryEncoder)** - Modified and integrated
- **[Keypad Library](https://playground.arduino.cc/Code/Keypad/)** - Replaced with built-in implementation
- **[EWMA Filter](https://github.com/jonnieZG/EWMA)** - Exponentially Weighted Moving Average algorithm
- **[Adafruit ADS1X15](https://github.com/adafruit/Adafruit_ADS1X15)** - High-resolution ADC support

---

## Troubleshooting

### Common Issues
1. **Pin numbering**: Use Arduino pin format (A1, A2, A3) for analog pins in configuration
2. **Analog pins**: Only 3 built-in analog pins available (A1-A3), use ADS1115 for more
3. **Voltage levels**: Ensure 3.3V compatibility of your components (RP2040 is NOT 5V tolerant)
4. **USB enumeration**: Uses USBHID library with automatic descriptor generation
5. **Upload mode**: Hold BOOTSEL button while connecting USB to enter bootloader mode

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
- Advanced signal processing includes EWMA filtering, intelligent deadband, and velocity-adaptive smoothing
- Configuration format is identical between branches - only hardware platform differs
- RP2040's limited analog pins (A1-A3) make ADS1115 valuable for complex setups with multiple axes
- Current configuration uses direct pin buttons (4-13) and shift register expansion for scalability
- New processing chain: Raw Input → Deadband → Noise Filter → Velocity-Adaptive Smoothing → Response Curve → Output