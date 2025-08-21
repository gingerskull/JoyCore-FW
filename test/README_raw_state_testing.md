# Raw State Testing Scripts

This directory contains Python scripts to test the new raw state monitoring functionality in JoyCore-FW firmware.

## Scripts Overview

### 1. `test_raw_state_commands.py` - Automated Test Suite
Comprehensive automated testing of all raw state commands.

**Usage:**
```bash
python test_raw_state_commands.py [port]
```

**Features:**
- Auto-detects JoyCore device if no port specified
- Tests all raw state commands individually
- Tests continuous monitoring for 5 seconds
- Provides detailed test results and pass/fail status
- Collects performance data and statistics

**Example Output:**
```
=== Testing GPIO States ===
Response: GPIO_STATES:0x12345678:1234567890
GPIO Mask: 0x12345678
Pin states:
  GPIO 0: LOW
  GPIO 1: HIGH
  ...

TEST RESULTS SUMMARY
GPIO            : PASS
MATRIX          : PASS
SHIFT_REG       : PASS
MONITORING      : PASS
```

### 2. `interactive_raw_state_test.py` - Interactive Menu
Interactive tool for manual testing and exploration.

**Usage:**
```bash
python interactive_raw_state_test.py [port]
```

**Features:**
- Menu-driven interface
- Test individual commands on demand
- Start/stop monitoring manually
- Send custom commands
- Real-time response display

**Menu Options:**
1. Test GPIO States
2. Test Matrix State  
3. Test Shift Register State
4. Start Continuous Monitoring
5. Send Custom Command
0. Quit

### 3. `raw_state_monitor.py` - Real-time Dashboard
Continuous real-time monitoring with clean dashboard display.

**Usage:**
```bash
python raw_state_monitor.py [port]
```

**Features:**
- Full-screen real-time dashboard
- Auto-refreshing display
- Organized layout showing all states
- Visual indicators for pressed buttons
- Update statistics and timestamps

**Dashboard Layout:**
```
======================================================================
JoyCore Raw State Monitor
======================================================================
Device: COM3
Last Update: 14:23:45.123
Updates Received: 1337

GPIO STATES (showing HIGH pins only):
----------------------------------------
  GPIO 4, GPIO 7, GPIO12
  Raw mask: 0x00001090

MATRIX STATES:
----------------------------------------
     0  1  2  3
R0:  ○  ●  ○  ○
R1:  ○  ○  ●  ○
  Pressed: R0C1, R1C2

SHIFT REGISTERS:
----------------------------------------
  Register 0: 0xFF (11111111)
    Set bits: B0, B1, B2, B3, B4, B5, B6, B7
```

## New Serial Commands Tested

### Individual Commands
- `READ_GPIO_STATES` - Returns 32-bit GPIO state mask
- `READ_MATRIX_STATE` - Returns matrix button intersection states
- `READ_SHIFT_REG` - Returns shift register buffer contents

### Monitoring Commands  
- `START_RAW_MONITOR` - Begin continuous 50ms state updates
- `STOP_RAW_MONITOR` - Stop continuous monitoring

## Command Response Formats

### GPIO States
```
GPIO_STATES:0x12345678:1234567890
```
- `0x12345678` - 32-bit mask (GPIO 0-29)
- `1234567890` - Timestamp in microseconds

### Matrix States
```
MATRIX_STATE:2:1:1:1234567890
```
- `2` - Row number
- `1` - Column number  
- `1` - State (1=pressed, 0=released)
- `1234567890` - Timestamp

### Shift Register States
```
SHIFT_REG:0:0xFF:1234567890
```
- `0` - Register ID
- `0xFF` - 8-bit register value
- `1234567890` - Timestamp

## Prerequisites

**Python Requirements:**
- Python 3.6 or later
- `pyserial` library

**Install pyserial:**
```bash
pip install pyserial
```

**Hardware Requirements:**
- JoyCore device with updated firmware
- USB connection to computer
- Optional: Buttons/switches connected to test actual state changes

## Usage Tips

1. **Auto-detection:** If no port is specified, scripts will scan all COM ports for JoyCore devices

2. **Manual port specification:** Use if auto-detection fails
   ```bash
   python test_raw_state_commands.py COM3      # Windows
   python test_raw_state_commands.py /dev/ttyACM0  # Linux
   ```

3. **Testing GPIO states:** Connect test wires to GPIO pins and observe state changes

4. **Testing matrix:** If matrix is configured, press buttons to see matrix states update

5. **Testing shift registers:** If 74HC165 chips are connected, observe their input states

6. **Monitoring performance:** Use automated test to verify 50ms update intervals

## Troubleshooting

**Device not found:**
- Check USB connection
- Verify firmware is updated with raw state commands
- Try specifying port manually
- Check device manager (Windows) or `ls /dev/tty*` (Linux)

**No response to commands:**
- Verify firmware includes raw state functionality
- Check serial port permissions (Linux: add user to dialout group)
- Try different baud rate (though 115200 is standard)

**Monitoring stops unexpectedly:**
- Device may have reset or disconnected
- Check USB cable connection
- Restart device and try again

**Matrix/Shift register shows "not configured":**
- Normal if hardware is not connected
- Check hardware configuration in firmware
- Verify pin mappings in ConfigDigital.h

## Integration with Configuration Tools

These scripts demonstrate the protocol that configuration tools can use to:

1. **Display live hardware states** during configuration
2. **Verify pin mappings** by observing GPIO changes
3. **Test matrix scanning** by showing button presses in real-time
4. **Debug shift register chains** by monitoring input states
5. **Validate timing** by checking update intervals

The response formats are designed to be easily parsed by configuration software for integration into user interfaces.