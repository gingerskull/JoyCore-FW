# JoyCore-FW HID Mapping Implementation Report

## Executive Summary

This report documents the comprehensive HID mapping features added to JoyCore-FW firmware to enable 3rd party configuration programs to automatically detect and correctly parse button configurations without manual setup. The implementation provides standardized methods for querying device layout, button mappings, and includes diagnostic capabilities for validation.

## 1. Architecture Overview

### 1.1 Core Components Added

```
src/rp2040/hid/
├── HIDMapping.h          # Data structures and constants
├── HIDMapping.cpp        # Feature report handlers and logic
└── TinyUSBGamepad.cpp    # Updated HID descriptor and report structure
```

### 1.2 Integration Points

- **ConfigManager**: Automatic updates when configuration changes
- **TinyUSBGamepad**: Extended HID descriptor and report structure
- **Serial Commands**: Debug interface for testing and validation
- **Main Loop**: Initialization and callback setup

## 2. HID Report Structure Modifications

### 2.1 Input Report Structure (Report ID 1)

The standard gamepad input report has been extended with a frame counter:

```c
typedef struct __attribute__((packed)) {
    uint8_t buttons[16];      // 128 buttons, 1 bit each (16 bytes)
    int16_t axes[16];         // 16 axes, 16-bit signed values (32 bytes)
    uint16_t frameCounter;    // Frame counter for report tracking (2 bytes)
} joycore_gamepad_report_t;
```

**Total Report Size**: 50 bytes (was 48 bytes)

### 2.2 Byte Layout for Parsing

| Byte Range | Content | Description |
|------------|---------|-------------|
| 0-15 | Button Data | 128 buttons, LSB-first per byte |
| 16-47 | Axis Data | 16 axes, int16_t little-endian |
| 48-49 | Frame Counter | uint16_t little-endian, increments per report |

### 2.3 Button Bit Mapping

Buttons are packed with **LSB-first bit order within each byte**:

```
Byte 0: [Btn7][Btn6][Btn5][Btn4][Btn3][Btn2][Btn1][Btn0]
Byte 1: [Btn15][Btn14][Btn13][Btn12][Btn11][Btn10][Btn9][Btn8]
...
Byte 15: [Btn127][Btn126][Btn125][Btn124][Btn123][Btn122][Btn121][Btn120]
```

**Bit extraction formula**:
```c
bool isPressed = (report.buttons[buttonID / 8] & (1 << (buttonID % 8))) != 0;
```

## 3. HID Feature Reports

### 3.1 Feature Report IDs

| Report ID | Purpose | Size | Access |
|-----------|---------|------|--------|
| 3 | HID_MAPPING_INFO | 16 bytes | GET only |
| 4 | BUTTON_MAP | Variable (0-128 bytes) | GET only |
| 5 | SELFTEST_CONTROL | 8 bytes | GET/SET |

### 3.2 HID_MAPPING_INFO Structure (Report ID 3)

```c
typedef struct __attribute__((packed)) {
    uint8_t protocol_version;       // Protocol version (currently 1)
    uint8_t report_id;              // Input report ID (1 for gamepad)
    uint8_t button_count;           // Number of configured buttons
    uint8_t axis_count;             // Number of configured axes
    uint8_t button_byte_offset;     // Byte offset where buttons start (0)
    uint8_t button_bit_order;       // 0 = LSB-first-per-byte, 1 = MSB-first
    uint16_t mapping_crc;           // CRC16 of mapping (0x0000 = sequential)
    uint8_t frame_counter_offset;   // Byte offset of frame counter (48)
    uint8_t reserved[7];            // Reserved for future use
} HIDMappingInfo;
```

**Parsing Example**:
```c
HIDMappingInfo info;
if (get_feature_report(3, &info, sizeof(info))) {
    printf("Device has %d buttons, %d axes\n", info.button_count, info.axis_count);
    printf("Frame counter at byte %d\n", info.frame_counter_offset);
    printf("Mapping is %s\n", info.mapping_crc == 0 ? "sequential" : "custom");
}
```

### 3.3 BUTTON_MAP Structure (Report ID 4)

Returns the mapping array only if mapping is non-sequential (CRC ≠ 0x0000).

**Format**: Array of uint8_t values, length = button_count from HID_MAPPING_INFO

```c
uint8_t mapping[128];
if (info.mapping_crc != 0x0000) {
    int size = get_feature_report(4, mapping, info.button_count);
    // mapping[bit_index] = joy_button_id
}
```

**Usage**:
- If `mapping_crc == 0x0000`: Use identity mapping (bit_index == joy_button_id)
- If `mapping_crc != 0x0000`: Use custom mapping array

### 3.4 SELFTEST_CONTROL Structure (Report ID 5)

```c
typedef struct __attribute__((packed)) {
    uint8_t command;                // 0 = stop, 1 = start button walk
    uint8_t status;                 // 0 = idle, 1 = running, 2 = complete
    uint8_t current_button;         // Current button being tested (0-127)
    uint8_t interval_ms;            // Test interval in milliseconds (30-50)
    uint8_t reserved[4];            // Reserved for future use
} SelfTestControl;
```

## 4. Programming Interface

### 4.1 USB HID Access Method

**Windows (using Windows API)**:
```c
// Open HID device
HANDLE device = CreateFile(devicePath, GENERIC_READ | GENERIC_WRITE, ...);

// Get mapping info
HIDMappingInfo info;
BOOL result = HidD_GetFeature(device, &info, sizeof(info));
info.report_id = 3; // Set report ID before call
```

**Linux (using hidraw)**:
```c
// Open hidraw device
int fd = open("/dev/hidraw0", O_RDWR);

// Get mapping info via ioctl
struct hidraw_report_descriptor desc;
ioctl(fd, HIDIOCGFEATURE(sizeof(HIDMappingInfo)), &info);
```

**Cross-platform (using libusb/hidapi)**:
```c
hid_device* device = hid_open(0x2E8A, 0x0003, NULL);
uint8_t buffer[17] = {3}; // Report ID 3
int result = hid_get_feature_report(device, buffer, sizeof(buffer));
HIDMappingInfo* info = (HIDMappingInfo*)(buffer + 1); // Skip report ID
```

### 4.2 Serial Interface Access

For easier access without HID libraries:

```c
// Send command via serial port
send_command("HID_MAPPING_INFO\n");
// Response: "HID_MAPPING_INFO:ver=1,rid=1,btn=32,axis=8,btn_offset=0,bit_order=0,crc=0x0000,fc_offset=48"

send_command("HID_BUTTON_MAP\n");  
// Response: "HID_BUTTON_MAP:SEQUENTIAL" or "HID_BUTTON_MAP:0,1,2,3,4,5,..."
```

## 5. Complete Parsing Algorithm

### 5.1 Device Configuration Detection

```c
typedef struct {
    uint8_t button_count;
    uint8_t axis_count;
    uint8_t frame_counter_offset;
    bool is_sequential_mapping;
    uint8_t button_mapping[128];
} DeviceConfig;

bool detect_device_configuration(hid_device* device, DeviceConfig* config) {
    // Step 1: Get mapping info
    uint8_t buffer[17] = {3}; // Report ID 3
    if (hid_get_feature_report(device, buffer, 17) != 17) {
        return false;
    }
    
    HIDMappingInfo* info = (HIDMappingInfo*)(buffer + 1);
    config->button_count = info->button_count;
    config->axis_count = info->axis_count;
    config->frame_counter_offset = info->frame_counter_offset;
    config->is_sequential_mapping = (info->mapping_crc == 0x0000);
    
    // Step 2: Get button mapping if needed
    if (config->is_sequential_mapping) {
        // Use identity mapping
        for (int i = 0; i < config->button_count; i++) {
            config->button_mapping[i] = i;
        }
    } else {
        // Get custom mapping
        uint8_t map_buffer[129] = {4}; // Report ID 4
        int result = hid_get_feature_report(device, map_buffer, config->button_count + 1);
        if (result != config->button_count + 1) {
            return false;
        }
        memcpy(config->button_mapping, map_buffer + 1, config->button_count);
    }
    
    return true;
}
```

### 5.2 Button State Parsing

```c
typedef struct {
    bool buttons[128];
    int16_t axes[16];
    uint16_t frame_counter;
} GamepadState;

bool parse_input_report(uint8_t* report, size_t size, 
                       const DeviceConfig* config, GamepadState* state) {
    if (size < 50) return false; // Minimum report size
    
    // Clear state
    memset(state, 0, sizeof(GamepadState));
    
    // Parse buttons using device mapping
    for (int bit_index = 0; bit_index < config->button_count; bit_index++) {
        uint8_t byte_index = bit_index / 8;
        uint8_t bit_mask = 1 << (bit_index % 8);
        bool is_pressed = (report[byte_index] & bit_mask) != 0;
        
        // Map to logical button ID
        uint8_t joy_button_id = config->button_mapping[bit_index];
        if (joy_button_id < 128) {
            state->buttons[joy_button_id] = is_pressed;
        }
    }
    
    // Parse axes (little-endian int16_t)
    for (int i = 0; i < config->axis_count && i < 16; i++) {
        int16_t* axis_ptr = (int16_t*)(report + 16 + i * 2);
        state->axes[i] = *axis_ptr;
    }
    
    // Parse frame counter
    uint16_t* counter_ptr = (uint16_t*)(report + config->frame_counter_offset);
    state->frame_counter = *counter_ptr;
    
    return true;
}
```

### 5.3 Complete Integration Example

```c
#include <stdio.h>
#include <hidapi.h>

int main() {
    // Connect to device
    hid_device* device = hid_open(0x2E8A, 0x0003, NULL);
    if (!device) {
        printf("Device not found\n");
        return 1;
    }
    
    // Detect configuration
    DeviceConfig config;
    if (!detect_device_configuration(device, &config)) {
        printf("Failed to detect device configuration\n");
        hid_close(device);
        return 1;
    }
    
    printf("Device detected: %d buttons, %d axes\n", 
           config.button_count, config.axis_count);
    printf("Mapping type: %s\n", 
           config.is_sequential_mapping ? "Sequential" : "Custom");
    
    // Monitor input reports
    uint8_t report[50];
    GamepadState state;
    uint16_t last_frame = 0;
    
    while (1) {
        int result = hid_read_timeout(device, report, sizeof(report), 1000);
        if (result > 0) {
            if (parse_input_report(report, result, &config, &state)) {
                // Check for dropped frames
                if (last_frame != 0) {
                    uint16_t frame_delta = state.frame_counter - last_frame;
                    if (frame_delta > 1) {
                        printf("Warning: %d frames dropped\n", frame_delta - 1);
                    }
                }
                last_frame = state.frame_counter;
                
                // Process button states
                for (int i = 0; i < config.button_count; i++) {
                    if (state.buttons[i]) {
                        printf("Button %d pressed\n", i);
                    }
                }
            }
        }
    }
    
    hid_close(device);
    return 0;
}
```

## 6. Configuration Change Handling

### 6.1 Dynamic Updates

The HID mapping information updates automatically when the device configuration changes (via config.bin updates). Applications should:

1. **Re-query mapping info** after any configuration change
2. **Monitor frame counter gaps** to detect communication issues
3. **Use self-test** to validate new configurations

### 6.2 Configuration Validation

```c
bool validate_device_mapping(hid_device* device) {
    // Start self-test
    uint8_t test_cmd[9] = {5, 1, 0, 0, 40, 0, 0, 0, 0}; // Report ID 5, start command
    hid_send_feature_report(device, test_cmd, 9);
    
    // Monitor for completion
    for (int i = 0; i < 100; i++) { // 10 second timeout
        uint8_t status_buffer[9] = {5}; // Report ID 5
        if (hid_get_feature_report(device, status_buffer, 9) == 9) {
            if (status_buffer[2] == 2) { // COMPLETE status
                printf("Self-test completed successfully\n");
                return true;
            }
        }
        usleep(100000); // 100ms delay
    }
    
    // Stop test if timeout
    uint8_t stop_cmd[9] = {5, 0, 0, 0, 0, 0, 0, 0, 0}; // Report ID 5, stop command
    hid_send_feature_report(device, stop_cmd, 9);
    return false;
}
```

## 7. Error Handling and Edge Cases

### 7.1 Common Error Conditions

- **Feature report not supported**: Older firmware versions
- **Invalid mapping CRC**: Corrupted configuration
- **Button count mismatch**: Configuration/hardware mismatch
- **Frame counter stalls**: Communication or firmware issues

### 7.2 Robust Implementation

```c
#define MAX_RETRY_COUNT 3
#define FEATURE_TIMEOUT_MS 1000

bool robust_get_mapping_info(hid_device* device, HIDMappingInfo* info) {
    for (int retry = 0; retry < MAX_RETRY_COUNT; retry++) {
        uint8_t buffer[17] = {3}; // Report ID 3
        
        if (hid_get_feature_report(device, buffer, 17) == 17) {
            memcpy(info, buffer + 1, sizeof(HIDMappingInfo));
            
            // Validate response
            if (info->protocol_version == 1 && 
                info->button_count <= 128 && 
                info->axis_count <= 16) {
                return true;
            }
        }
        
        usleep(100000); // 100ms delay before retry
    }
    
    return false;
}
```

## 8. Performance Considerations

### 8.1 Polling Recommendations

- **Feature reports**: Query once at startup, re-query on configuration change
- **Input reports**: Poll at application frame rate (30-120 Hz typical)
- **Frame counter**: Monitor for dropped frames, don't rely on exact timing

### 8.2 Memory Usage

- **HID descriptor**: +48 bytes for new feature report collections
- **RAM usage**: ~150 bytes for mapping state and buffers
- **Flash usage**: ~2KB for HID mapping implementation

## 9. Compatibility and Migration

### 9.1 Backward Compatibility

- **Existing applications**: Continue to work unchanged
- **Input report format**: Extended but maintains existing layout
- **USB descriptor**: Maintains existing gamepad functionality

### 9.2 Version Detection

```c
bool supports_hid_mapping(hid_device* device) {
    uint8_t buffer[17] = {3}; // Report ID 3
    return (hid_get_feature_report(device, buffer, 17) == 17);
}
```

## 10. Summary

The HID mapping implementation provides a standardized method for 3rd party applications to automatically detect and correctly parse JoyCore device configurations. Key benefits:

- **Zero-configuration**: Automatic button mapping detection
- **Robust validation**: Frame counter and self-test capabilities  
- **Future-proof**: Extensible protocol design
- **Multiple interfaces**: Both HID and serial access methods
- **Real-time updates**: Dynamic configuration change support

This implementation eliminates the need for manual configuration in 3rd party tools while providing comprehensive diagnostic capabilities for validation and troubleshooting.