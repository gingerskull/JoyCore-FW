# RP2040 Port Summary

## Overview

This branch contains a complete port of GNGR-ButtonBox from Teensy 4.0 to RP2040. The port provides excellent performance, cost-effectiveness, and broad hardware compatibility.

## Key Benefits

### Performance
- **CPU**: Dual-core ARM Cortex-M0+ at 133MHz
- **Memory**: 264KB SRAM + 2MB Flash (typical)
- **PIO**: 8 state machines for hardware-accelerated I/O

### USB Reliability
- **TinyUSB Stack**: Mature and well-tested USB implementation
- **HID Gamepad**: Standard gamepad with 32 buttons, 6 axes, 1 hat
- **Hot-plug Support**: Reliable USB enumeration and reconnection

### Development
- **Cost-Effective**: RP2040 boards are very affordable
- **Wide Support**: Many board options (Pico, Feather, QtPy, etc.)
- **Active Community**: Large ecosystem and support

## Hardware Migration

### Pin Compatibility
Most hardware designs will work with minimal changes:

| Feature | Teensy 4.0 | RP2040 (Pico) | Notes |
|---------|------------|---------------|-------|
| Digital I/O | 0-39 | GP0-GP28 | 29 GPIO pins available |
| Analog Input | A0-A13 | GP26-GP28 | 3 ADC pins (12-bit) |
| PWM | All digital | All GPIO | Hardware PWM on all pins |
| I2C | Multiple | GP0/1, GP4/5, etc | Multiple I2C peripherals |
| SPI | Multiple | Multiple | Hardware SPI support |

### Voltage Levels
- **Teensy 4.0**: 3.3V logic (5V tolerant)
- **RP2040**: 3.3V logic (NOT 5V tolerant)

⚠️ **Important**: RP2040 pins are NOT 5V tolerant. Use level shifters if interfacing with 5V devices.

## Software Changes

### Build System
- **PlatformIO**: Configured in platformio.ini
- **Arduino IDE**: Supported with Arduino-Pico core

### Dependencies
- **Removed**: Teensy-specific joystick code
- **Added**: Adafruit TinyUSB Library

### USB Implementation
The RP2040 port uses TinyUSB for USB HID functionality:
- Standard HID gamepad descriptor
- 32 buttons (bitmask)
- 6 analog axes (8-bit signed)
- 1 hat switch (8-way)

### Key Differences from Teensy 4.0

1. **USB Stack**: TinyUSB instead of Teensy's built-in USB
2. **Analog Resolution**: 12-bit ADC (vs 10-bit default on Teensy)
3. **Pin Count**: Fewer pins but sufficient for most designs
4. **Cost**: Significantly more affordable
5. **Dual Core**: Can leverage second core for advanced features

## Configuration

The configuration files remain largely unchanged:
- `Config.h`: Main configuration
- `ConfigDigital.h`: Pin mappings (update for RP2040 GPIO numbers)
- `ConfigAxis.h`: Analog axis configuration

### Pin Naming
Use RP2040 GPIO numbers in configuration:
- `0` through `28` for GPIO pins
- `26`, `27`, `28` for analog-capable pins

## Performance Notes

- **Update Rate**: Maintains sub-millisecond USB update rates
- **Encoder Support**: Excellent rotary encoder performance
- **Matrix Scanning**: Fast GPIO switching for matrix operations
- **Shift Registers**: SPI hardware acceleration available

## Future Enhancements

The RP2040's unique features enable potential improvements:
- **PIO**: Hardware-accelerated matrix scanning or encoder reading
- **Dual Core**: Dedicated core for time-critical operations
- **DMA**: Efficient data transfers for shift registers
- **Interpolator**: Hardware acceleration for analog processing

## Troubleshooting

### Common Issues

1. **USB Not Recognized**
   - Ensure TinyUSB library is installed
   - Check USB cable (data capable, not charge-only)
   - Try BOOTSEL reset if needed

2. **5V Compatibility**
   - Use level shifters for 5V devices
   - Check all connected hardware voltage levels

3. **Analog Noise**
   - RP2040 ADC can be noisy, use filtering
   - Consider external ADC (ADS1115) for precision

4. **Pin Conflicts**
   - Some pins have special functions (USB, LED, etc.)
   - Refer to your specific board's pinout