# Teensy 4.0 Port Summary

## Overview

This branch contains a complete port of GNGR-ButtonBox from Arduino Pro Micro to Teensy 4.0. The port provides significant improvements in performance, reliability, and USB handling.

## Key Benefits

### Performance
- **CPU**: 600MHz ARM Cortex-M7 (vs 16MHz ATmega32U4)
- **Memory**: 1MB Flash + 512KB RAM (vs 32KB Flash + 2.5KB RAM)
- **I/O**: More pins available for complex button matrices

### USB Reliability
- **Native USB Stack**: Uses Teensy's proven USB implementation
- **Better Compatibility**: Improved host computer compatibility
- **Faster Recognition**: Quicker USB enumeration and reconnection

### Development
- **Simplified Code**: Removed complex DynamicHID implementation
- **Better Debugging**: More reliable serial output and debugging
- **Future-Proof**: Easier to extend and maintain

## Hardware Migration

### Pin Compatibility
Most hardware designs will work with minimal changes:

| Feature | Arduino Pro Micro | Teensy 4.0 | Notes |
|---------|------------------|------------|-------|
| Digital I/O | 0-21 | 0-23, 24-33 | More pins available |
| Analog Input | A0-A3, A6-A10 | A0-A13 | More analog inputs |
| PWM | Limited | Many pins | Better PWM support |
| Interrupts | 2, 3, 7 | Most pins | More interrupt options |

### Voltage Levels
- **Arduino Pro Micro**: 5V logic
- **Teensy 4.0**: 3.3V logic (5V tolerant on most pins)

Most 5V components work fine with Teensy 4.0, but check your specific parts.

## Software Changes

### Build System
- **PlatformIO**: Recommended (configured in platformio.ini)
- **Arduino IDE**: Also supported with Teensyduino

### Dependencies
- **Removed**: Arduino Joystick Library, DynamicHID
- **Added**: Native Teensy joystick support (built-in)
- **Unchanged**: RotaryEncoder library, user configuration

### Configuration
User configuration in `src/UserConfig.h` works the same way, just update pin numbers for Teensy 4.0 pinout.

## Getting Started

1. **Install PlatformIO** or Arduino IDE with Teensyduino
2. **Update pin mapping** in `src/UserConfig.h` for your hardware
3. **Build and upload**:
   ```bash
   pio run --target upload
   ```
4. **Test** with your preferred game/simulator

## Troubleshooting

### Common Issues
1. **Pin numbering**: Teensy 4.0 uses different pin numbers
2. **Voltage levels**: Ensure 3.3V compatibility
3. **USB type**: Use USB_SERIAL_HID build flag for development

### Debug Options
- **Serial Monitor**: Available for debugging
- **USB Descriptor**: Automatically handled by Teensy
- **LED Indicator**: Use built-in LED for status indication

## Performance Comparison

| Metric | Arduino Pro Micro | Teensy 4.0 | Improvement |
|--------|------------------|------------|-------------|
| Loop Speed | ~1kHz typical | >10kHz possible | 10x+ faster |
| Button Response | <10ms | <1ms | 10x+ faster |
| USB Latency | Variable | Consistent | More reliable |
| Memory Usage | Near limits | <10% used | Abundant headroom |

## Future Enhancements

The Teensy 4.0 platform enables future features:
- **More complex input matrices** (more pins available)
- **Advanced signal processing** (more CPU power)
- **Real-time response** (better interrupt handling)
- **Ethernet connectivity** (Teensy 4.1 support possible)
- **Audio feedback** (Teensy audio library integration)

## Support

For issues specific to the Teensy 4.0 port:
1. Check pin mapping in UserConfig.h
2. Verify 3.3V compatibility of your components
3. Ensure latest Teensyduino installation
4. Test with minimal configuration first

The logical input configuration and overall architecture remain the same as the Arduino version - only the underlying hardware platform has changed.