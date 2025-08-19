# HID Mapping Features Test Scripts

This directory contains test scripts that demonstrate and validate the new HID mapping features added to JoyCore-FW. These features allow 3rd party configuration programs to automatically detect button mappings and device configuration.

## 🎯 Features Tested

- **HID Mapping Info**: Protocol version, button/axis counts, offsets, bit order
- **Button Mapping**: Automatic detection of sequential vs custom mappings
- **Frame Counter**: Monitoring HID report frame increments
- **Self-Test**: Button walk validation for testing physical connections

## 📁 Test Scripts

### 1. `test_hid_mapping.py` - USB HID Interface Test

**Direct USB HID feature report testing**

**Requirements:**
```bash
pip install pyusb
```

**Usage:**
```bash
python test_hid_mapping.py
```

**Features:**
- Direct HID feature report access
- Real-time frame counter monitoring
- Complete self-test functionality
- Interactive menu system

**Note:** May require administrator/sudo privileges for USB access.

### 2. `test_hid_mapping_serial.py` - Serial Interface Test

**Serial command interface testing (recommended)**

**Requirements:**
```bash
pip install pyserial
```

**Usage:**
```bash
# Auto-detect port
python test_hid_mapping_serial.py

# Specify port (Windows)
python test_hid_mapping_serial.py COM3

# Specify port (Linux/Mac)
python test_hid_mapping_serial.py /dev/ttyACM0
```

**Features:**
- Easy to use, no special USB permissions needed
- Auto-detection of JoyCore device
- All HID mapping features via serial commands
- Interactive testing menu

## 🚀 Quick Start

1. **Flash the updated firmware** to your JoyCore device
2. **Connect the device** via USB
3. **Run the serial test script** (easier):
   ```bash
   python test_hid_mapping_serial.py
   ```

The script will automatically detect your device and run a comprehensive demo of all features.

## 📊 Test Output Examples

### HID Mapping Information
```
🔍 HID Mapping Information
============================================================
Protocol Version:     1
Input Report ID:      1
Button Count:         32
Axis Count:           8
Button Byte Offset:   0
Button Bit Order:     LSB-first
Frame Counter Offset: 48
Mapping CRC:          0x0000
Sequential Mapping:   ✅ Yes
```

### Button Mapping (Custom)
```
🔍 Button Mapping
============================================================
📋 Custom button mapping:
   Bit Index → Joy Button ID
   -------------------------
     0       →    0   ✅
     1       →    1   ✅
     2       →    5   ⚠️
     3       →    3   ✅
```

### Self-Test Output
```
🔍 Self-Test Button Walk (10s)
============================================================
🧪 Starting self-test button walk...
   Each button will be activated for 40ms in sequence
   This helps validate button mapping and detect stuck buttons

✅ Self-test started
   Testing button   0...
   Testing button   1...
   Testing button   2...
   ...
✅ Self-test completed at button 31
🛑 Self-test stopped
```

## 🔧 Serial Commands Reference

The firmware now supports these serial commands for HID mapping:

| Command | Description | Example Response |
|---------|-------------|------------------|
| `HID_MAPPING_INFO` | Get mapping information | `HID_MAPPING_INFO:ver=1,rid=1,btn=32,...` |
| `HID_BUTTON_MAP` | Get button mapping | `HID_BUTTON_MAP:0,1,2,3,4,5,...` or `HID_BUTTON_MAP:SEQUENTIAL` |
| `HID_SELFTEST start` | Start button walk test | `HID_SELFTEST:STARTED` |
| `HID_SELFTEST stop` | Stop button walk test | `HID_SELFTEST:STOPPED` |
| `HID_SELFTEST status` | Get test status | `HID_SELFTEST:status=RUNNING,btn=5,interval=40` |

## 📡 USB HID Feature Reports

For 3rd party applications, the following HID feature reports are available:

| Report ID | Purpose | Size | Description |
|-----------|---------|------|-------------|
| 3 | HID_MAPPING_INFO | 16 bytes | Device configuration and mapping info |
| 4 | BUTTON_MAP | Variable | Button mapping array (if not sequential) |
| 5 | SELFTEST_CONTROL | 8 bytes | Self-test control and status |

## 🎮 Frame Counter

The HID input report now includes a 16-bit frame counter at byte offset 48:

- **Purpose**: Detect dropped or stuck HID reports
- **Type**: `uint16_t` (little-endian)
- **Location**: Bytes 48-49 in the 50-byte HID report
- **Behavior**: Increments with each HID report sent

## 🧪 Self-Test Features

The self-test functionality provides:

- **Button Walk**: Activates one button at a time in sequence
- **Timing Control**: Configurable interval (30-50ms recommended)
- **Status Monitoring**: Real-time progress feedback
- **Validation**: Helps detect stuck buttons and mapping issues

## 🐛 Troubleshooting

### Device Not Found
- Ensure JoyCore device is connected via USB
- Try different USB ports
- On Linux, check device permissions: `sudo chmod 666 /dev/ttyACM*`
- Update device drivers if needed

### Permission Errors (USB HID)
- Run script as administrator (Windows) or sudo (Linux)
- Install appropriate USB drivers
- Use the serial version instead (easier)

### Serial Connection Issues
- Check correct COM port (Windows) or device file (Linux)
- Ensure no other programs are using the serial port
- Try auto-detection mode (don't specify port)

## 💡 Integration Guide

For 3rd party applications:

1. **Query HID_MAPPING_INFO** to get device configuration
2. **Check sequential mapping** (CRC = 0x0000 means sequential)
3. **Get custom mapping** if needed using BUTTON_MAP
4. **Monitor frame counter** to detect communication issues
5. **Use self-test** to validate physical button connections

The frame counter and mapping info enable automatic configuration without manual setup!

## 📞 Support

If you encounter issues:

1. Ensure you're using the latest firmware with HID mapping features
2. Check that the device enumerates correctly in Device Manager (Windows) or lsusb (Linux)
3. Try the serial version first as it's more reliable
4. Check the main JoyCore documentation for configuration help

## 🎉 Success Indicators

✅ **Working correctly if you see:**
- Device auto-detection succeeds
- HID mapping info returns valid data
- Button mapping shows expected layout
- Self-test activates buttons in sequence
- Frame counter increments with activity

This validates that all HID mapping features are functioning properly and ready for use by 3rd party applications!