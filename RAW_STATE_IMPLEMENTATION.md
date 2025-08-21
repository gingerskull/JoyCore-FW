# Raw State Monitoring Implementation Guide

This document provides a comprehensive overview of the raw state monitoring functionality added to JoyCore-FW and how configuration programs can integrate with it.

## Table of Contents
1. [Overview](#overview)
2. [Architecture Changes](#architecture-changes)
3. [Serial Commands API](#serial-commands-api)
4. [Response Formats](#response-formats)
5. [Integration Guide](#integration-guide)
6. [Usage Examples](#usage-examples)
7. [Performance Considerations](#performance-considerations)
8. [Troubleshooting](#troubleshooting)

## Overview

The raw state monitoring system provides non-intrusive access to the actual electrical states of:
- **GPIO pins** (all 30 pins on RP2040)
- **Button matrix intersections** (row/column scanning results)
- **Shift register inputs** (74HC165 chain states)

This functionality enables configuration programs to display live hardware states, verify pin mappings, and debug input configurations without interfering with normal joystick operation.

## Architecture Changes

### New Files Added

#### `src/comm/RawStateReader.h`
```cpp
class RawStateReader {
public:
    // Individual state reading commands
    static void readGpioStates();
    static void readMatrixState();
    static void readShiftRegState();
    
    // Continuous monitoring
    static void startRawMonitor();
    static void stopRawMonitor();
    static void updateRawMonitoring();  // Call from main loop
    
private:
    static bool s_rawMonitoringEnabled;
    static uint32_t s_lastMonitorUpdate;
    static constexpr uint32_t MONITOR_INTERVAL_MS = 50;
};
```

#### `src/comm/RawStateReader.cpp`
- **GPIO Reading**: Uses `gpio_get()` to read raw pin states directly
- **Matrix Scanning**: Temporarily drives matrix rows and reads columns
- **Shift Register Access**: Reads from existing buffered data
- **Monitoring Loop**: 50ms interval updates when enabled

### Modified Files

#### `src/comm/SerialCommands.cpp`
Added command handlers:
```cpp
// New command handlers
{"READ_GPIO_STATES", cmdReadGpioStates},
{"READ_MATRIX_STATE", cmdReadMatrixState},
{"READ_SHIFT_REG", cmdReadShiftReg},
{"START_RAW_MONITOR", cmdStartRawMonitor},
{"STOP_RAW_MONITOR", cmdStopRawMonitor},
```

#### `src/inputs/buttons/MatrixInput.h/.cpp`
Added accessor functions:
```cpp
namespace MatrixRawAccess {
    uint8_t* getRowPins();
    uint8_t* getColPins();
}
```

#### `src/main.cpp`
Added monitoring update to main loop:
```cpp
void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        processSerialLine(line);
    }
    g_inputManager.update(MyJoystick);
    RawStateReader::updateRawMonitoring();  // New line
}
```

## Serial Commands API

### Individual State Reading Commands

#### `READ_GPIO_STATES`
**Purpose**: Read all GPIO pin states as a single operation  
**Parameters**: None  
**Response**: Single line with GPIO state mask and timestamp

#### `READ_MATRIX_STATE`
**Purpose**: Scan button matrix and return all intersection states  
**Parameters**: None  
**Response**: Multiple lines, one per matrix intersection

#### `READ_SHIFT_REG`
**Purpose**: Read current shift register buffer contents  
**Parameters**: None  
**Response**: Multiple lines, one per shift register

### Continuous Monitoring Commands

#### `START_RAW_MONITOR`
**Purpose**: Begin continuous state updates every 50ms  
**Parameters**: None  
**Response**: Confirmation message, then continuous state updates

#### `STOP_RAW_MONITOR`  
**Purpose**: Stop continuous monitoring  
**Parameters**: None  
**Response**: Confirmation message

## Response Formats

### GPIO States Response
```
GPIO_STATES:0x[32-bit-hex]:[timestamp]
```

**Example**: `GPIO_STATES:0x00001090:1234567890`
- **Bit Position**: Corresponds to GPIO pin number (bit 0 = GPIO0, etc.)
- **Bit Value**: 1 = HIGH (3.3V), 0 = LOW (0V)
- **Timestamp**: Microseconds since boot (`to_us_since_boot()`)

**Parsing Example**:
```python
import re

response = "GPIO_STATES:0x00001090:1234567890"
match = re.match(r'GPIO_STATES:0x([0-9A-F]+):(\d+)', response)
if match:
    gpio_mask = int(match.group(1), 16)
    timestamp = int(match.group(2))
    
    # Check individual pins
    for pin in range(30):
        is_high = bool(gpio_mask & (1 << pin))
        print(f"GPIO{pin}: {'HIGH' if is_high else 'LOW'}")
```

### Matrix State Response
```
MATRIX_STATE:[row]:[col]:[state]:[timestamp]
```

**Example**: `MATRIX_STATE:2:1:1:1234567890`
- **Row**: Matrix row number (0-based)
- **Column**: Matrix column number (0-based)  
- **State**: 1 = pressed/connected, 0 = released/open
- **Timestamp**: Microseconds since boot

**Special Responses**:
- `MATRIX_STATE:NO_MATRIX_CONFIGURED` - No matrix defined in configuration
- `MATRIX_STATE:NO_MATRIX_PINS_CONFIGURED` - Matrix size defined but pins not configured

**Parsing Example**:
```python
matrix_data = {}
responses = read_multiple_lines()

for response in responses:
    match = re.match(r'MATRIX_STATE:(\d+):(\d+):([01]):(\d+)', response)
    if match:
        row = int(match.group(1))
        col = int(match.group(2))
        state = int(match.group(3))
        
        if row not in matrix_data:
            matrix_data[row] = {}
        matrix_data[row][col] = state

# matrix_data now contains complete matrix state
```

### Shift Register Response
```
SHIFT_REG:[reg_id]:[8-bit-hex]:[timestamp]
```

**Example**: `SHIFT_REG:0:0xFF:1234567890`
- **Register ID**: Chain position (0 = first register)
- **Value**: 8-bit register contents (0x00-0xFF)
- **Timestamp**: Microseconds since boot

**Special Response**:
- `SHIFT_REG:NO_SHIFT_REG_CONFIGURED` - No shift registers configured

**Parsing Example**:
```python
shift_data = {}
responses = read_multiple_lines()

for response in responses:
    match = re.match(r'SHIFT_REG:(\d+):0x([0-9A-F]+):(\d+)', response)
    if match:
        reg_id = int(match.group(1))
        reg_value = int(match.group(2), 16)
        timestamp = int(match.group(3))
        
        shift_data[reg_id] = reg_value
        
        # Check individual bits
        for bit in range(8):
            is_set = bool(reg_value & (1 << bit))
            print(f"Register {reg_id}, Bit {bit}: {is_set}")
```

### Monitoring Responses
When monitoring is active, the device sends all three types of responses continuously:

```
OK:RAW_MONITOR_STARTED
GPIO_STATES:0x00001090:1234567890
MATRIX_STATE:0:0:0:1234567891
MATRIX_STATE:0:1:1:1234567891
MATRIX_STATE:1:0:0:1234567891
SHIFT_REG:0:0xFF:1234567892
SHIFT_REG:1:0x80:1234567892
GPIO_STATES:0x00001094:1234617890
...
OK:RAW_MONITOR_STOPPED
```

## Integration Guide

### Basic Integration Steps

#### 1. Establish Serial Connection
```python
import serial
import time

# Connect to device
ser = serial.Serial('COM3', 115200, timeout=5)
time.sleep(2)  # Wait for device ready

# Verify connection
ser.write(b'IDENTIFY\n')
response = ser.readline().decode().strip()
if 'JoyCore' not in response:
    raise Exception("Device not responding")
```

#### 2. Read Individual States
```python
def read_gpio_states(ser):
    ser.write(b'READ_GPIO_STATES\n')
    response = ser.readline().decode().strip()
    
    match = re.match(r'GPIO_STATES:0x([0-9A-F]+):(\d+)', response)
    if match:
        return int(match.group(1), 16), int(match.group(2))
    return None, None

def read_matrix_states(ser):
    ser.write(b'READ_MATRIX_STATE\n')
    
    matrix_data = {}
    timeout = time.time() + 3
    
    while time.time() < timeout:
        if ser.in_waiting > 0:
            line = ser.readline().decode().strip()
            if not line:
                continue
                
            if "NO_MATRIX" in line:
                return None
                
            match = re.match(r'MATRIX_STATE:(\d+):(\d+):([01]):(\d+)', line)
            if match:
                row, col, state = int(match.group(1)), int(match.group(2)), int(match.group(3))
                if row not in matrix_data:
                    matrix_data[row] = {}
                matrix_data[row][col] = state
                timeout = time.time() + 1  # Reset timeout on data
                
    return matrix_data
```

#### 3. Start Continuous Monitoring
```python
import threading
import queue

class RawStateMonitor:
    def __init__(self, serial_port):
        self.ser = serial_port
        self.running = False
        self.data_queue = queue.Queue()
        self.monitor_thread = None
        
    def start_monitoring(self):
        self.ser.write(b'START_RAW_MONITOR\n')
        response = self.ser.readline().decode().strip()
        
        if response == "OK:RAW_MONITOR_STARTED":
            self.running = True
            self.monitor_thread = threading.Thread(target=self._monitor_loop)
            self.monitor_thread.start()
            return True
        return False
        
    def stop_monitoring(self):
        self.running = False
        self.ser.write(b'STOP_RAW_MONITOR\n')
        if self.monitor_thread:
            self.monitor_thread.join()
            
    def _monitor_loop(self):
        while self.running:
            if self.ser.in_waiting > 0:
                line = self.ser.readline().decode().strip()
                if line:
                    self.data_queue.put(line)
                    
    def get_latest_data(self):
        data = []
        while not self.data_queue.empty():
            data.append(self.data_queue.get())
        return data
```

### UI Integration Examples

#### Real-time GPIO Display
```python
class GPIODisplay:
    def __init__(self, parent):
        self.parent = parent
        self.gpio_indicators = []
        
        # Create 30 LED-style indicators
        for pin in range(30):
            indicator = LEDIndicator(parent, text=f"GPIO{pin}")
            self.gpio_indicators.append(indicator)
            
    def update_states(self, gpio_mask):
        for pin in range(30):
            is_high = bool(gpio_mask & (1 << pin))
            self.gpio_indicators[pin].set_state(is_high)
```

#### Matrix Button Grid
```python
class MatrixDisplay:
    def __init__(self, parent, rows, cols):
        self.buttons = []
        
        for row in range(rows):
            button_row = []
            for col in range(cols):
                btn = ButtonIndicator(parent, text=f"R{row}C{col}")
                button_row.append(btn)
            self.buttons.append(button_row)
            
    def update_states(self, matrix_data):
        for row, cols in matrix_data.items():
            for col, state in cols.items():
                if row < len(self.buttons) and col < len(self.buttons[row]):
                    self.buttons[row][col].set_pressed(bool(state))
```

#### Shift Register Bit Display
```python
class ShiftRegisterDisplay:
    def __init__(self, parent, register_count):
        self.bit_indicators = []
        
        for reg in range(register_count):
            reg_bits = []
            for bit in range(8):
                indicator = BitIndicator(parent, text=f"R{reg}B{bit}")
                reg_bits.append(indicator)
            self.bit_indicators.append(reg_bits)
            
    def update_states(self, shift_data):
        for reg_id, value in shift_data.items():
            if reg_id < len(self.bit_indicators):
                for bit in range(8):
                    is_set = bool(value & (1 << bit))
                    self.bit_indicators[reg_id][bit].set_state(is_set)
```

## Usage Examples

### Configuration Verification Tool
```python
class ConfigurationVerifier:
    def __init__(self, serial_port):
        self.monitor = RawStateMonitor(serial_port)
        
    def verify_pin_mapping(self, expected_pin, instruction):
        """Verify a pin mapping by asking user to trigger it."""
        print(f"Please {instruction}")
        print("Press Enter when ready...")
        input()
        
        # Read initial state
        initial_gpio, _ = read_gpio_states(self.monitor.ser)
        initial_state = bool(initial_gpio & (1 << expected_pin))
        
        print(f"Trigger the input now...")
        time.sleep(1)
        
        # Read new state
        new_gpio, _ = read_gpio_states(self.monitor.ser)
        new_state = bool(new_gpio & (1 << expected_pin))
        
        if initial_state != new_state:
            print(f"✓ Pin GPIO{expected_pin} changed as expected")
            return True
        else:
            print(f"✗ Pin GPIO{expected_pin} did not change")
            return False
            
    def verify_matrix_button(self, expected_row, expected_col, instruction):
        """Verify a matrix button by asking user to press it."""
        print(f"Please {instruction}")
        
        # Read initial matrix state
        initial_matrix = read_matrix_states(self.monitor.ser)
        initial_state = initial_matrix.get(expected_row, {}).get(expected_col, 0)
        
        print("Press the button now...")
        time.sleep(1)
        
        # Read new matrix state
        new_matrix = read_matrix_states(self.monitor.ser)
        new_state = new_matrix.get(expected_row, {}).get(expected_col, 0)
        
        if initial_state != new_state:
            print(f"✓ Matrix R{expected_row}C{expected_col} changed as expected")
            return True
        else:
            print(f"✗ Matrix R{expected_row}C{expected_col} did not change")
            return False
```

### Live Debugging Dashboard
```python
class DebugDashboard:
    def __init__(self):
        self.monitor = None
        self.update_timer = None
        
    def start_live_monitoring(self, serial_port):
        self.monitor = RawStateMonitor(serial_port)
        
        if self.monitor.start_monitoring():
            # Update UI every 100ms
            self.update_timer = self.root.after(100, self.update_display)
            
    def update_display(self):
        if not self.monitor:
            return
            
        # Get latest data
        new_data = self.monitor.get_latest_data()
        
        # Parse and update displays
        for line in new_data:
            if line.startswith('GPIO_STATES:'):
                gpio_mask, timestamp = self.parse_gpio_line(line)
                self.gpio_display.update_states(gpio_mask)
                
            elif line.startswith('MATRIX_STATE:'):
                row, col, state, timestamp = self.parse_matrix_line(line)
                self.matrix_display.update_button(row, col, state)
                
            elif line.startswith('SHIFT_REG:'):
                reg_id, value, timestamp = self.parse_shift_line(line)
                self.shift_display.update_register(reg_id, value)
                
        # Schedule next update
        self.update_timer = self.root.after(100, self.update_display)
```

### Pin Configuration Wizard
```python
class PinConfigurationWizard:
    def __init__(self, serial_port):
        self.ser = serial_port
        self.discovered_pins = {}
        
    def auto_discover_pins(self):
        """Auto-discover pin mappings by monitoring changes."""
        print("Auto-discovering pin mappings...")
        print("Please press each button/switch one at a time")
        
        # Start monitoring
        self.ser.write(b'START_RAW_MONITOR\n')
        
        button_count = 0
        last_gpio_state = 0
        
        try:
            while button_count < 20:  # Discover up to 20 buttons
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode().strip()
                    
                    if line.startswith('GPIO_STATES:'):
                        match = re.match(r'GPIO_STATES:0x([0-9A-F]+):(\d+)', line)
                        if match:
                            current_gpio = int(match.group(1), 16)
                            
                            # Find changed pins
                            changed_pins = current_gpio ^ last_gpio_state
                            
                            if changed_pins:
                                for pin in range(30):
                                    if changed_pins & (1 << pin):
                                        pin_state = bool(current_gpio & (1 << pin))
                                        
                                        if pin not in self.discovered_pins:
                                            self.discovered_pins[pin] = []
                                            
                                        self.discovered_pins[pin].append({
                                            'timestamp': int(match.group(2)),
                                            'state': pin_state,
                                            'button_id': button_count
                                        })
                                        
                                        print(f"Detected input on GPIO{pin} (Button {button_count})")
                                        button_count += 1
                                        
                            last_gpio_state = current_gpio
                            
        except KeyboardInterrupt:
            print("Discovery stopped by user")
            
        finally:
            self.ser.write(b'STOP_RAW_MONITOR\n')
            
        return self.discovered_pins
```

## Performance Considerations

### Timing Characteristics
- **Individual Commands**: ~1-5ms response time
- **Monitoring Interval**: 50ms (20Hz update rate)
- **GPIO Reading**: <1ms (direct register access)
- **Matrix Scanning**: 1-3ms (depends on matrix size)
- **Shift Register Reading**: <1ms (uses buffered data)

### Resource Usage
- **Memory Overhead**: <1KB static allocation
- **CPU Impact**: Minimal (<1% additional load)
- **Serial Bandwidth**: ~200-500 bytes per monitoring cycle

### Optimization Tips

#### For High-Frequency Monitoring
```python
# Use monitoring mode instead of individual commands
monitor.start_monitoring()  # 50ms updates

# Instead of:
while True:
    read_gpio_states()  # Multiple serial round-trips
    read_matrix_states()
    time.sleep(0.05)
```

#### For Batch Operations
```python
# Group related operations
def read_all_states():
    gpio_mask, _ = read_gpio_states(ser)
    matrix_data = read_matrix_states(ser)
    shift_data = read_shift_reg_states(ser)
    return gpio_mask, matrix_data, shift_data
```

#### For UI Responsiveness
```python
# Use background thread for monitoring
class NonBlockingMonitor:
    def __init__(self):
        self.latest_states = {}
        self.update_callbacks = []
        
    def add_callback(self, callback):
        self.update_callbacks.append(callback)
        
    def _background_monitor(self):
        while self.running:
            data = self.monitor.get_latest_data()
            
            # Parse and cache latest states
            for line in data:
                self.parse_and_cache(line)
                
            # Notify UI callbacks
            for callback in self.update_callbacks:
                callback(self.latest_states)
                
            time.sleep(0.1)  # UI update rate
```

## Troubleshooting

### Common Issues and Solutions

#### No Response to Commands
**Symptoms**: Commands send but no response received
**Causes**: 
- Device not running updated firmware
- Serial connection issues
- Wrong baud rate

**Solutions**:
```python
# Verify basic communication first
ser.write(b'IDENTIFY\n')
response = ser.readline().decode().strip()
if 'JoyCore' not in response:
    print("Device not responding or wrong firmware")

# Check raw state command availability
ser.write(b'READ_GPIO_STATES\n')
response = ser.readline().decode().strip()
if response == "ERROR:UNKNOWN_COMMAND":
    print("Firmware does not support raw state commands")
```

#### Matrix Shows "Not Configured"
**Symptoms**: `MATRIX_STATE:NO_MATRIX_CONFIGURED`
**Causes**: No matrix defined in ConfigDigital.h

**Solutions**:
- Check if matrix is actually configured in firmware
- This is normal if no matrix hardware is connected

#### Shift Register Shows "Not Configured"  
**Symptoms**: `SHIFT_REG:NO_SHIFT_REG_CONFIGURED`
**Causes**: No 74HC165 chips configured or connected

**Solutions**:
- Normal if no shift registers are used
- Check hardware connections if registers should be present

#### Monitoring Stops Unexpectedly
**Symptoms**: No more monitoring data received
**Causes**: 
- Device reset
- USB disconnection
- Serial buffer overflow

**Solutions**:
```python
def robust_monitoring():
    retry_count = 0
    max_retries = 3
    
    while retry_count < max_retries:
        try:
            # Start monitoring
            if monitor.start_monitoring():
                # Monitor for data
                timeout = time.time() + 5
                data_received = False
                
                while time.time() < timeout:
                    data = monitor.get_latest_data()
                    if data:
                        data_received = True
                        timeout = time.time() + 5  # Reset timeout
                        process_data(data)
                        
                if not data_received:
                    raise Exception("No data received")
                    
        except Exception as e:
            print(f"Monitoring error: {e}")
            retry_count += 1
            time.sleep(1)
            
            # Try to reconnect
            monitor.stop_monitoring()
            reconnect_device()
```

#### GPIO States Don't Change
**Symptoms**: GPIO mask always the same value
**Causes**:
- Pins configured as outputs
- No input stimulus
- Pull-up/pull-down resistors

**Diagnostic**:
```python
def diagnose_gpio_pins():
    # Read multiple samples
    samples = []
    for i in range(10):
        gpio_mask, _ = read_gpio_states(ser)
        samples.append(gpio_mask)
        time.sleep(0.1)
        
    # Check for any variation
    if len(set(samples)) == 1:
        print("No GPIO changes detected")
        print(f"Constant value: 0x{samples[0]:08X}")
        
        # Show pin states
        for pin in range(30):
            state = "HIGH" if (samples[0] & (1 << pin)) else "LOW"
            print(f"  GPIO{pin}: {state}")
    else:
        print("GPIO changes detected - pins are working")
```

### Integration Checklist

#### Before Integration
- [ ] Firmware updated with raw state functionality
- [ ] Device responds to `IDENTIFY` command  
- [ ] Basic serial communication working
- [ ] Test scripts run successfully

#### During Development
- [ ] Handle "not configured" responses gracefully
- [ ] Implement timeout handling for serial reads
- [ ] Use monitoring mode for real-time updates
- [ ] Cache latest states for UI responsiveness

#### Testing
- [ ] Verify GPIO changes with known inputs
- [ ] Test matrix scanning if hardware present
- [ ] Check shift register data if configured
- [ ] Validate monitoring start/stop functionality
- [ ] Test error recovery and reconnection

This implementation provides a robust foundation for configuration programs to access real-time hardware states while maintaining the performance and reliability of the core joystick functionality.