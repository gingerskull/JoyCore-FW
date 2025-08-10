

<div align="center">

# 🎮 JoyCore-FW
**Next-Generation USB Game Controller Firmware for RP2040**

![RP2040 Badge](https://img.shields.io/badge/RP2040-Native-brightgreen?style=flat&logo=raspberrypi)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange?style=flat&logo=platformio)
![TinyUSB](https://img.shields.io/badge/TinyUSB-HID-blue?style=flat&logo=adafruit)
![License](https://img.shields.io/badge/License-GPL--3.0-red?style=flat)

**⚠️ RP2040 Branch - For Arduino Pro Micro see [`main`](../../tree/main) | For Teensy 4.0 see [`teensy40`](../../tree/teensy40)**

![RP2040 Pico Pinout](images/rp2040-pico-pinout.png)

</div>

---

## 🚀 **Key Advantages & Features**

### **⚡Performance**  
- **Dual ARM Cortex-M0+** processors @ 133MHz
- **264KB SRAM** + 2MB Flash memory
- **Hardware PIO** for accelerated I/O operations
- **<2ms button response** time with excellent USB reliability

### **🎮 Input Support**
- **Direct Pins**: Up to ~26 individual button/encoder connections
- **Matrix Scanning**: 8×8 row/column multiplexing (64 buttons max)
- **Shift Registers**: 74HC165 expansion for 128+ inputs
- **Analog Axes**: Built-in 12-bit ADC + 16-bit ADS1115 external ADC (8 axes total)
- **Rotary Encoders**: Supported on all input types with configurable latch modes

### **🧠 Signal Processing**
- **EWMA filtering** with configurable responsiveness
- **Intelligent deadband** with statistical movement analysis  
- **Custom response curves** with up to 11 interpolation points
- **Multi-stage processing** pipeline for robust controls

---

## ⚙️ **Digital Input Configuration**

### ⚡ **Hardware Pin Mapping**

> **⚠️ CRITICAL:** RP2040 pins are **3.3V only** - use level shifters for 5V devices!

Configure your pin mapping in `src/config/ConfigDigital.h`:

```cpp
static const PinMapEntry hardwarePinMap[] = {
  // 🎯 Direct inputs
  {"2", BTN},           // Simple button
  {"3", BTN},           // Another button  
  
  // 🔄 Direct encoder
  {"9", BTN},           // Encoder A channel
  {"10", BTN},          // Encoder B channel
  
  // 🔘 3×2 Matrix
  {"4", BTN_ROW},       // Matrix row 0
  {"5", BTN_ROW},       // Matrix row 1  
  {"6", BTN_ROW},       // Matrix row 2
  {"7", BTN_COL},       // Matrix column 0
  {"8", BTN_COL},       // Matrix column 1
  
  // ⚡ 74HC165 Shift Register
  {"18", SHIFTREG_CLK}, // Clock pin
  {"19", SHIFTREG_PL},  // Parallel load pin
  {"20", SHIFTREG_QH}   // Serial data out pin
};
```

### 🎮 **Logical Input Configuration**

Map your hardware to joystick buttons in the same file:

```cpp
constexpr LogicalInput logicalInputs[] = {
  // 🎯 Direct pin buttons
  { INPUT_PIN, { .pin = {2, 1, NORMAL, 0} } },      // Pin 2 → Button 1
  { INPUT_PIN, { .pin = {3, 2, MOMENTARY, 0} } },   // Pin 3 → Button 2 (momentary)
  
  // 🔄 Direct pin encoder (consecutive ENC_A → ENC_B)
  { INPUT_PIN, { .pin = {9, 3, ENC_A, 0} }, FOUR3 }, // Pin 9 → Encoder A
  { INPUT_PIN, { .pin = {10, 4, ENC_B, 0} }, FOUR3 }, // Pin 10 → Encoder B
  
  // 🔘 Matrix buttons (3×2 = 6 buttons)
  { INPUT_MATRIX, { .matrix = {0, 0, 5, NORMAL, 0} } },    // Row 0, Col 0 → Button 5
  { INPUT_MATRIX, { .matrix = {0, 1, 6, NORMAL, 0} } },    // Row 0, Col 1 → Button 6
  { INPUT_MATRIX, { .matrix = {1, 0, 7, MOMENTARY, 0} } }, // Row 1, Col 0 → Button 7
  { INPUT_MATRIX, { .matrix = {1, 1, 8, NORMAL, 0} } },    // Row 1, Col 1 → Button 8
  
  // 🔄 Matrix encoder (uses positions 2,0 and 2,1)
  { INPUT_MATRIX, { .matrix = {2, 0, 9, ENC_A, 0} }, FOUR0 }, // Row 2, Col 0 → Encoder A
  { INPUT_MATRIX, { .matrix = {2, 1, 10, ENC_B, 0} }, FOUR0 }, // Row 2, Col 1 → Encoder B
  
  // ⚡ Shift register inputs
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 11, NORMAL, 0} } },     // Reg 0, bit 0 → Button 11
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 12, MOMENTARY, 0} } }, // Reg 0, bit 1 → Button 12
  
  // 🔄 Shift register encoder (consecutive bits 2 & 3)
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 13, ENC_A, 0} }, FOUR0 }, // Reg 0, bit 2 → Encoder A
  { INPUT_SHIFTREG, { .shiftreg = {0, 3, 14, ENC_B, 0} }, FOUR0 }  // Reg 0, bit 3 → Encoder B
};
```

### 🎛️ **Button Behaviors**

| Type | Description |
|------|-------------|
| `NORMAL` | Standard press/release | 
| `MOMENTARY` | Quick pulse on press | 
| `ENC_A/ENC_B` | Encoder channels (paired) |

### 🔄 **Encoder Setup & Advanced Tips**

> **💡 Pro Tip:** Encoders require consecutive ENC_A → ENC_B entries with matching latch modes

```cpp
// ✅ Correct encoder pairing
{ INPUT_PIN, { .pin = {9, 1, ENC_A, 0} }, FOUR3 },  // A channel
{ INPUT_PIN, { .pin = {10, 2, ENC_B, 0} }, FOUR3 }, // B channel (same latch mode)

// ❌ Wrong - different latch modes
{ INPUT_PIN, { .pin = {9, 1, ENC_A, 0} }, FOUR3 },  // A channel  
{ INPUT_PIN, { .pin = {10, 2, ENC_B, 0} }, FOUR0 }, // B channel (different mode!)
```

**Latch Modes:**
- `FOUR3` - 4 steps/detent, both edges (most common)
- `FOUR0` - 4 steps/detent, single edge  
- `TWO03` - 2 steps/detent (fast response)

**Encoder Placement Strategies:**
- **Direct Pins**: Best performance, use for primary controls
- **Matrix**: Good for secondary encoders, shares pins efficiently  
- **Shift Register**: Lowest cost, slight latency increase

---

## 📊 **Analog Axis Configuration & Processing**

### 🎮 **Basic Axis Setup**

Configure analog inputs in `src/config/ConfigAxis.h`:

```cpp
// 📊 X-Axis (Built-in ADC)
#define USE_AXIS_X
#ifdef USE_AXIS_X
    #define AXIS_X_PIN              A0              // Built-in 12-bit ADC
    #define AXIS_X_MIN              0               
    #define AXIS_X_MAX              32767           
    #define AXIS_X_FILTER_LEVEL     AXIS_FILTER_EWMA // Smart filtering
    #define AXIS_X_EWMA_ALPHA       200             // Responsiveness (0-1000)
    #define AXIS_X_DEADBAND         250             // Anti-jitter
    #define AXIS_X_CURVE            CURVE_CUSTOM    // Response curve
#endif

// 📊 Y-Axis (High-Resolution with ADS1115)
#define USE_AXIS_Y  
#ifdef USE_AXIS_Y
    #define AXIS_Y_PIN              ADS1115_CH0     // 16-bit external ADC
    #define AXIS_Y_MIN              0
    #define AXIS_Y_MAX              32767
    #define AXIS_Y_FILTER_LEVEL     AXIS_FILTER_EWMA
    #define AXIS_Y_EWMA_ALPHA       150
    #define AXIS_Y_CURVE            CURVE_CUSTOM    // Custom response curve
#endif
```

**Hardware Options:**
- **Built-in ADC**: A0, A1, A2 (GPIO26-28) - 12-bit resolution
- **ADS1115**: ADS1115_CH0 to ADS1115_CH3 - 16-bit resolution, I2C interface

### 📈 **Signal Processing Pipeline**

```
🔄 Raw Input → 🎯 Deadband → 🔬 Noise Filter → ⚡ EWMA Smoothing → 📊 Response Curve → 🎮 USB Output
```

### 🔬 **Advanced Filtering Options**

| Filter | Description | Best For |
|--------|-------------|----------|
| `AXIS_FILTER_OFF` | Raw values | High-precision, low-noise sensors |
| `AXIS_FILTER_EWMA` | Exponential smoothing | General use, configurable response |

**EWMA Alpha Values:**
- `30-50` - Heavy smoothing (slow, stable)
- `100-200` - Balanced (recommended) 
- `300-500` - Light smoothing (fast, responsive)

**Response Curves:**
- `CURVE_CUSTOM` - Define your own custom lookup table with up to 11 points

The default custom curve is linear (1:1 response). You can define your own curves by modifying the lookup table in the configuration system. Custom curves support smooth interpolation between points for any desired response characteristic.

**Intelligent Deadband:**
- Dynamic around current position (not fixed center)
- Prevents jitter when control is at rest
- Statistical analysis avoids interfering with slow movements
- Typical values: 0 (off), 250-500 (light), 500-1000 (medium)

### 🧱 Unified Axis Descriptor Model (RP2040 Branch)

Axes are defined once via a compile‑time descriptor list in `src/config/ConfigAxis.h`:

```cpp
struct AxisDescriptor {
    uint8_t idx;          // Internal axis enum (X,Y,Z,RX,RY,RZ,S1,S2)
    int     pin;          // A0/A1/A2 or ADS1115_CH0..CH3
    int     minv, maxv;   // Logical range
    AxisFilterLevel filter; 
    uint32_t alpha;       // EWMA alpha (0..1000)
    int     deadband;     // Deadband size
    ResponseCurveType curve; // Response curve id
};

static const AxisDescriptor axisDescriptors[] = {
#ifdef USE_AXIS_X
    { AnalogAxisManager::AXIS_X, AXIS_X_PIN, AXIS_X_MIN, AXIS_X_MAX, AXIS_X_FILTER_LEVEL, AXIS_X_EWMA_ALPHA, AXIS_X_DEADBAND, AXIS_X_CURVE },
#endif
  // ...additional conditional entries
};
```

Lifecycle:
1. `setupUserAxes()` scans descriptors: if any `pin` is an ADS1115 channel (`isAdsPin()` helper) it initializes the external ADC.
2. First call to `readUserAxes()` applies each descriptor (pin/range/filter/alpha/deadband/curve/enable).
3. Subsequent calls just read all axes and publish them (`joystick.setAxis`).

Advantages:
- Single source of truth (no duplicated #ifdef blocks)
- Adding an axis: define `USE_AXIS_*` block + one descriptor entry
- Unused axes fully compiled out
- Easy to extend (e.g. add invert flag later)

Removed Legacy: Older macro patterns (`AXIS_CFG`, `AXIS_CONFIG_LIST`) were eliminated to avoid preprocessor complexity.

Potential Extensions:
- Optional `invert` boolean in descriptor
- Storage overlay to override static defaults at runtime
- Small LUT index for response curves instead of enum

---

## 🧮 Deterministic Digital Input Memory Model

All digital input subsystems (direct buttons, shift‑register buttons, matrix, encoders) now use fixed compile‑time
memory pools (see `src/inputs/PoolConfig.h`). No dynamic allocation occurs in the scan/update path.

Pool limits:

```
MAX_BUTTON_PIN_GROUPS
MAX_LOGICAL_PER_PIN
MAX_SHIFTREG_GROUPS
MAX_LOGICAL_PER_SHIFT_BIT
MAX_MATRIX_ROWS / MAX_MATRIX_COLS
MAX_LOGICAL_PER_MATRIX_POS
MAX_ENCODERS
```

If configuration exceeds a limit, extra logical inputs are ignored (clamped). During development you can add
`static_assert`s or debug prints to catch this early.

### ShiftRegisterManager
Centralizes timed reads of chained 74HC165 parts (1 ms single, 5 ms multi). All consumers read the shared
buffer instead of hitting hardware directly.

### InputManager
Provides `g_inputManager.update()` which sequences: shift → buttons → matrix → encoders → axes → HID send.

### Matrix Refactor
`ButtonMatrix` internal arrays are now static (size = `MAX_MATRIX_ROWS * MAX_MATRIX_COLS`); destructor removed;
constructed with placement new after matrix size discovery.

### Tuning
Lower limits to save RAM; raise to support bigger control panels. Cost is linear static BSS; no runtime penalty.

Future ideas: clamp warning counters, SMALL/DEFAULT/LARGE presets, utilization reporting.

---

## 🔌 **USB & System Configuration**

### 📱 **USB Descriptor Configuration**

Customize your device identity in `src/config/ConfigDigital.h`:

```cpp
static const struct {
    uint16_t vendorID;
    uint16_t productID; 
    const char* manufacturer;
    const char* product;
} staticUSBDescriptor = {
    .vendorID = 0x2E8A,          // Raspberry Pi Foundation
    .productID = 0xA02F,         // Custom PID
    .manufacturer = "YourName",   // 🏷️ Your brand
    .product = "Epic Controller"  // 🎮 Your product
};
```

### 🎛️ **Configuration Modes**

| Mode | Description | Use Case |
|------|-------------|----------|
| `CONFIG_MODE_STATIC` | Compile-time config | Development, simple setups |
| `CONFIG_MODE_STORAGE` | Runtime config via HID/Serial | Production, end-user config |
| `CONFIG_MODE_HYBRID` | Static fallback + storage | Best of both worlds |

---

## 🔧 **Build & Flash**

```bash
# 🏗️ Build firmware
pio run

# 📱 Flash to RP2040 (hold BOOTSEL button while plugging USB)
pio run --target upload
```

---

## 🔧 **Hardware Compatibility**

<table width="100%">
<tr width="100%">
<td width="33%">

#### 🍓 **Raspberry Pi Pico**
- GPIO0-28 available
- Built-in LED on GPIO25
- USB-C connector
- **$4-6**

</td>
<td width="33%">

#### 🦅 **Adafruit Feather RP2040**  
- GPIO0-29 available
- NeoPixel + user button
- USB-C + JST battery
- **$10-12**

</td>
<td width="33%">

#### ⚡ **SparkFun Pro Micro RP2040**
- Arduino Pro Micro footprint
- GPIO0-29 available  
- USB-C connector
- **$8-10**

</td>
</tr>
</table>

---

## 🏆 **Performance Comparison**

| Metric | Arduino Pro Micro | Teensy 4.0 | **RP2040** |
|--------|------------------|------------|-------------|
| **CPU** | 16MHz ATmega32U4 | 600MHz ARM M7 | **133MHz ARM M0+** |
| **RAM** | 2.5KB | 512KB | **264KB** |  
| **Flash** | 32KB | 1MB | **2MB** |
| **Response** | ~10ms | <1ms | **<2ms** |
| **Cost** | $8-12 | $20-25 | **$4-8** |
| **USB** | Standard | Excellent | **Excellent** |

---

## 🆘 **Troubleshooting**

### **Upload Issues**
- Hold **BOOTSEL** button while connecting USB
- Check cable supports data (not just power)
- Verify correct COM port in device manager

### **Pin Problems** 
- RP2040 is **3.3V only** - use level shifters for 5V
- Analog pins: Only A0, A1, A2 (GPIO26-28) available on RP2040
- Use ADS1115 for additional analog inputs

### **Configuration Errors**
- Encoder pairs must be consecutive ENC_A → ENC_B entries
- Matrix dimensions must match row/col pin counts
- Shift register count must match chained 74HC165 chips

---

## 🌟 **Credits**

- **[Raspberry Pi Foundation](https://github.com/raspberrypi)** - RP2040 MCU
- **[Arduino-Pico](https://github.com/earlephilhower/arduino-pico)** - Arduino-compatible RP2040 core by Earle Philhower
- **[TinyUSB](https://github.com/hathach/tinyusb)** - USB stack by Ha Thach
- **[Adafruit ADS1X15](https://github.com/adafruit/Adafruit_ADS1X15)** - 16-bit ADC library