# Firmware Modifications for Raw Hardware State Reading

## Overview

This document provides detailed instructions for modifying the JoyCore firmware (RP2040-based) to support raw hardware state reading. These modifications will enable the dashboard to display actual electrical states of GPIO pins, button matrices, and shift registers.

## Prerequisites

- JoyCore firmware source code
- Understanding of the existing ConfigProtocol.h command structure
- RP2040 SDK knowledge
- Basic understanding of GPIO, button matrices, and shift registers

## New Serial Commands to Implement

### 1. READ_GPIO_STATES Command

**Command**: `READ_GPIO_STATES\n`

**Response Format**: `GPIO_STATES:0x[32-bit-hex]:[timestamp]\n`

**Implementation**:

```cpp
// In ConfigProtocol.h or similar command handler
void handleReadGpioStates() {
    uint32_t gpio_mask = 0;
    
    // Read all 30 GPIO pins (GPIO 0-29)
    for (int i = 0; i < 30; i++) {
        // Read the actual electrical state of each GPIO
        bool pin_state = gpio_get(i);
        if (pin_state) {
            gpio_mask |= (1 << i);
        }
    }
    
    // Get current timestamp in microseconds
    uint64_t timestamp = to_us_since_boot(get_absolute_time());
    
    // Send response
    printf("GPIO_STATES:0x%08X:%llu\n", gpio_mask, timestamp);
}
```

**Notes**:
- Must read raw GPIO state, not button logical state
- Include all 30 GPIO pins in the mask
- Timestamp helps with synchronization and debugging

### 2. READ_MATRIX_STATE Command

**Command**: `READ_MATRIX_STATE\n`

**Response Format**: Multiple lines of `MATRIX_STATE:[row]:[col]:[0/1]:[timestamp]\n`

**Implementation**:

```cpp
void handleReadMatrixState() {
    uint64_t timestamp = to_us_since_boot(get_absolute_time());
    
    // Assuming you have matrix configuration stored
    for (int row = 0; row < matrix_rows; row++) {
        // Drive this row LOW (active)
        gpio_put(matrix_row_pins[row], 0);
        gpio_set_dir(matrix_row_pins[row], GPIO_OUT);
        
        // Small delay for signal to settle
        sleep_us(10);
        
        // Read all columns
        for (int col = 0; col < matrix_cols; col++) {
            // Read column state (with pull-up, LOW means pressed)
            bool is_connected = !gpio_get(matrix_col_pins[col]);
            
            // Send state for this intersection
            printf("MATRIX_STATE:%d:%d:%d:%llu\n", 
                   row, col, is_connected ? 1 : 0, timestamp);
        }
        
        // Return row to high-impedance state
        gpio_set_dir(matrix_row_pins[row], GPIO_IN);
    }
}
```

**Important Considerations**:
- Must save and restore current matrix scanning state
- Use proper timing to avoid false readings
- Consider implementing ghosting detection
- May need to disable interrupts during scan

### 3. READ_SHIFT_REG Command

**Command**: `READ_SHIFT_REG\n`

**Response Format**: `SHIFT_REG:[reg_id]:[8-bit-hex]:[timestamp]\n`

**Implementation for 74HC165**:

```cpp
void handleReadShiftRegisters() {
    uint64_t timestamp = to_us_since_boot(get_absolute_time());
    
    // For each configured shift register
    for (int reg = 0; reg < num_shift_registers; reg++) {
        uint8_t shift_data = 0;
        
        // Latch the current state (PL pin LOW then HIGH)
        gpio_put(shift_reg_pl_pin, 0);
        sleep_us(1);
        gpio_put(shift_reg_pl_pin, 1);
        sleep_us(1);
        
        // Read 8 bits
        for (int bit = 7; bit >= 0; bit--) {
            // Read data bit
            if (gpio_get(shift_reg_data_pin)) {
                shift_data |= (1 << bit);
            }
            
            // Clock pulse (rising edge shifts data)
            gpio_put(shift_reg_clk_pin, 0);
            sleep_us(1);
            gpio_put(shift_reg_clk_pin, 1);
            sleep_us(1);
        }
        
        // Send the result
        printf("SHIFT_REG:%d:0x%02X:%llu\n", reg, shift_data, timestamp);
    }
}
```

**Notes**:
- Must not interfere with normal button reading
- Timing is critical for reliable shift register reading
- Support cascaded shift registers if used

### 4. START_RAW_MONITOR Command

**Command**: `START_RAW_MONITOR\n`

**Response**: `OK:RAW_MONITOR_STARTED\n` followed by continuous state updates

**Implementation**:

```cpp
// Global flag
volatile bool raw_monitoring_enabled = false;

void handleStartRawMonitor() {
    raw_monitoring_enabled = true;
    printf("OK:RAW_MONITOR_STARTED\n");
}

// In main loop or timer interrupt
void rawMonitoringTask() {
    static uint64_t last_update = 0;
    uint64_t now = to_us_since_boot(get_absolute_time());
    
    // Update every 50ms (configurable)
    if (raw_monitoring_enabled && (now - last_update) >= 50000) {
        last_update = now;
        
        // Send all states
        sendGpioStates();
        sendMatrixStates();
        sendShiftRegStates();
    }
}
```

### 5. STOP_RAW_MONITOR Command

**Command**: `STOP_RAW_MONITOR\n`

**Response**: `OK:RAW_MONITOR_STOPPED\n`

**Implementation**:

```cpp
void handleStopRawMonitor() {
    raw_monitoring_enabled = false;
    printf("OK:RAW_MONITOR_STOPPED\n");
}
```

## Integration with Existing Firmware

### 1. Command Parser Integration

Add to your existing command parser (typically in main.cpp or similar):

```cpp
// In your command parsing function
if (strcmp(command, "READ_GPIO_STATES") == 0) {
    handleReadGpioStates();
} else if (strcmp(command, "READ_MATRIX_STATE") == 0) {
    handleReadMatrixState();
} else if (strcmp(command, "READ_SHIFT_REG") == 0) {
    handleReadShiftRegisters();
} else if (strcmp(command, "START_RAW_MONITOR") == 0) {
    handleStartRawMonitor();
} else if (strcmp(command, "STOP_RAW_MONITOR") == 0) {
    handleStopRawMonitor();
}
```

### 2. State Management

Create a separate module for raw state reading to avoid conflicts:

```cpp
// raw_state.h
#ifndef RAW_STATE_H
#define RAW_STATE_H

void initRawStateReading();
void handleRawStateCommands(const char* command);
void updateRawStateMonitoring();

#endif

// raw_state.cpp
#include "raw_state.h"

// Store last known states for change detection
static uint32_t last_gpio_mask = 0;
static uint8_t last_shift_reg_states[MAX_SHIFT_REGS];
static bool last_matrix_states[MAX_MATRIX_ROWS][MAX_MATRIX_COLS];

// Implementation of raw state functions...
```

### 3. Thread Safety Considerations

If using FreeRTOS or similar:

```cpp
// Mutex for protecting shared resources
static mutex_t gpio_mutex;
static mutex_t matrix_mutex;
static mutex_t shift_reg_mutex;

void readGpioStatesSafe() {
    mutex_enter_blocking(&gpio_mutex);
    // Read GPIO states
    mutex_exit(&gpio_mutex);
}
```

### 4. Performance Optimization

```cpp
// Batch reading for efficiency
typedef struct {
    uint32_t gpio_mask;
    uint8_t matrix_states[8];  // Packed matrix states
    uint8_t shift_reg_states[MAX_SHIFT_REGS];
    uint64_t timestamp;
} RawHardwareSnapshot;

void captureRawSnapshot(RawHardwareSnapshot* snapshot) {
    snapshot->timestamp = to_us_since_boot(get_absolute_time());
    
    // Capture all states atomically
    // ... implementation ...
}
```

## Testing Protocol

### 1. Basic Functionality Test

```bash
# Test each command individually
echo "READ_GPIO_STATES" > /dev/ttyACM0
echo "READ_MATRIX_STATE" > /dev/ttyACM0
echo "READ_SHIFT_REG" > /dev/ttyACM0
```

### 2. Monitoring Test

```bash
# Start monitoring
echo "START_RAW_MONITOR" > /dev/ttyACM0
# Should see continuous updates
# Stop monitoring
echo "STOP_RAW_MONITOR" > /dev/ttyACM0
```

### 3. Hardware Verification

1. **GPIO Test**: Connect test pin to GND/3.3V and verify state changes
2. **Matrix Test**: Press matrix buttons and verify correct row/col detection
3. **Shift Register Test**: Set known patterns and verify reading

## Common Issues and Solutions

### Issue 1: Interference with Normal Operation

**Problem**: Raw state reading interferes with button detection

**Solution**:
```cpp
// Use state machine to separate operations
enum SystemState {
    STATE_NORMAL_OPERATION,
    STATE_RAW_READING,
    STATE_IDLE
};

// Only read raw states during idle periods
```

### Issue 2: Timing Conflicts

**Problem**: Matrix scanning timing affected by raw state reading

**Solution**:
```cpp
// Use DMA or PIO for non-blocking reads
// Or implement time-sliced reading
```

### Issue 3: Buffer Overflows

**Problem**: Too much serial data causes overflow

**Solution**:
```cpp
// Implement throttling
static uint32_t last_send_time = 0;
const uint32_t MIN_SEND_INTERVAL = 20000; // 20ms minimum

if (now - last_send_time < MIN_SEND_INTERVAL) {
    return; // Skip this update
}
```

## Configuration Options

Add to your config structure:

```cpp
typedef struct {
    bool raw_state_enabled;
    uint32_t raw_state_poll_interval_ms;
    bool raw_state_debug_mode;
    bool raw_state_change_events_only;
} RawStateConfig;

// Default configuration
const RawStateConfig DEFAULT_RAW_CONFIG = {
    .raw_state_enabled = false,
    .raw_state_poll_interval_ms = 50,
    .raw_state_debug_mode = false,
    .raw_state_change_events_only = false
};
```

## Memory Usage Estimation

- GPIO state tracking: 4 bytes
- Matrix state tracking: rows * cols bytes
- Shift register tracking: num_registers bytes
- Command buffers: ~256 bytes
- Total overhead: < 1KB for typical configuration

## Final Notes

1. **Backwards Compatibility**: These changes should not affect existing HID functionality
2. **Error Handling**: Always validate pin configurations before reading
3. **Documentation**: Update firmware documentation with new commands
4. **Version**: Consider incrementing firmware version to indicate raw state support
5. **Testing**: Thoroughly test with actual hardware before deployment

Remember to maintain clean separation between raw state reading and normal button processing to ensure reliability of both systems.