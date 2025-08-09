#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../../Config.h"
#include "ConfigMode.h"

// Serializable configuration structures for storage and USB communication
// These structures pack the current configuration data into binary format
// for efficient storage and transmission

// Configuration file header with version and validation
struct ConfigHeader {
    uint32_t magic;          // Magic number for file validation (0x4A4F5943 = "JOYC")
    uint16_t version;        // Configuration format version
    uint16_t size;           // Total size of configuration data
    uint32_t checksum;       // CRC32 checksum for data integrity
    uint8_t reserved[4];     // Reserved for future use
} __attribute__((packed));

// Pin mapping entry for storage
struct StoredPinMapEntry {
    char name[8];            // Pin name (truncated for storage efficiency)
    uint8_t type;            // PinType enum value
    uint8_t reserved;        // Padding for alignment
} __attribute__((packed));

// Logical input definition for storage
struct StoredLogicalInput {
    uint8_t type;            // InputType enum value
    uint8_t behavior;        // ButtonBehavior enum value  
    uint8_t joyButtonID;     // Joystick button ID
    uint8_t reverse;         // Reverse flag
    uint8_t encoderLatchMode; // LatchMode enum value
    uint8_t reserved[3];     // Padding for alignment
    
    // Union for different input types
    union {
        struct {
            uint8_t pin;
        } pin;
        
        struct {
            uint8_t row;
            uint8_t col;
        } matrix;
        
        struct {
            uint8_t regIndex;
            uint8_t bitIndex;
        } shiftreg;
    } data;
} __attribute__((packed));

// Analog axis configuration for storage
struct StoredAxisConfig {
    uint8_t enabled;         // Axis enabled flag
    uint8_t pin;             // Analog pin number (or ADS1115 channel)
    uint16_t minValue;       // Minimum axis value
    uint16_t maxValue;       // Maximum axis value
    uint8_t filterLevel;     // Filter level (AXIS_FILTER_*)
    uint16_t ewmaAlpha;      // EWMA alpha value (0-1000)
    uint16_t deadband;       // Deadband size
    uint8_t curve;           // Response curve type
    uint8_t reserved[3];     // Padding for alignment
} __attribute__((packed));

// USB descriptor configuration for storage
struct StoredUSBDescriptor {
    uint16_t vendorID;       // USB Vendor ID (VID)
    uint16_t productID;      // USB Product ID (PID)
    char manufacturer[32];   // Manufacturer string
    char product[32];        // Product string
    uint8_t reserved[8];     // Padding for future expansion
} __attribute__((packed));

// Main configuration structure
struct StoredConfig {
    ConfigHeader header;
    
    // USB descriptor configuration
    StoredUSBDescriptor usbDescriptor;
    
    // Digital configuration
    uint8_t pinMapCount;
    uint8_t logicalInputCount;
    uint8_t shiftRegCount;
    uint8_t reserved1;       // Padding
    
    // Analog configuration - 8 axes (X, Y, Z, RX, RY, RZ, S1, S2)
    StoredAxisConfig axes[8];
    
    // Variable-length arrays (stored after this structure)
    // StoredPinMapEntry pinMap[pinMapCount];
    // StoredLogicalInput logicalInputs[logicalInputCount];
} __attribute__((packed));

// USB protocol message types (deprecated - using serial protocol instead)
// enum class ConfigMessageType : uint8_t {
//     GET_CONFIG = 0x01,       // Request current configuration
//     SET_CONFIG = 0x02,       // Update configuration
//     RESET_CONFIG = 0x03,     // Reset to factory defaults
//     VALIDATE_CONFIG = 0x04,  // Validate configuration without applying
//     GET_CONFIG_STATUS = 0x05, // Get configuration system status
//     SAVE_CONFIG = 0x06,      // Save current config to storage
//     LOAD_CONFIG = 0x07       // Load config from storage
// };

// USB protocol message header (deprecated - using serial protocol instead)
// struct ConfigMessage {
//     uint8_t reportID;        // HID Report ID (CONFIG_USB_FEATURE_REPORT_ID)
//     ConfigMessageType type;  // Message type
//     uint8_t sequence;        // Sequence number for multi-packet transfers
//     uint8_t totalPackets;    // Total number of packets in transfer
//     uint16_t dataLength;     // Length of data in this packet
//     uint8_t status;          // Status/error code
//     uint8_t reserved;        // Padding
//     uint8_t data[CONFIG_USB_MAX_PACKET_SIZE - 8]; // Payload data
// } __attribute__((packed));

// Configuration validation result
struct ConfigValidationResult {
    bool isValid;
    uint8_t errorCount;
    uint8_t warningCount;
    uint8_t reserved;
    char firstError[32];     // First error message (truncated)
} __attribute__((packed));

// Configuration system status
struct ConfigStatus {
    bool storageInitialized;
    bool configLoaded;
    bool usingDefaults;
    uint8_t currentMode;     // CONFIG_MODE_* value
    uint32_t storageUsed;
    uint32_t storageAvailable;
    uint16_t configVersion;
    uint8_t reserved[6];
} __attribute__((packed));

// Constants for configuration limits
static constexpr uint8_t MAX_PIN_MAP_ENTRIES = 32;
static constexpr uint8_t MAX_LOGICAL_INPUTS = 64;
static constexpr uint8_t MAX_SHIFT_REGISTERS = 8;
static constexpr uint32_t CONFIG_MAGIC = 0x4A4F5943; // "JOYC"

// Helper functions for conversion between runtime and stored formats
namespace ConfigConversion {
    
    // Convert runtime pin map to stored format
    bool packPinMap(const PinMapEntry* runtimeMap, uint8_t count, StoredPinMapEntry* storedMap);
    
    // Convert stored pin map to runtime format  
    bool unpackPinMap(const StoredPinMapEntry* storedMap, uint8_t count, PinMapEntry* runtimeMap);
    
    // Convert runtime logical inputs to stored format
    bool packLogicalInputs(const LogicalInput* runtimeInputs, uint8_t count, StoredLogicalInput* storedInputs);
    
    // Convert stored logical inputs to runtime format
    bool unpackLogicalInputs(const StoredLogicalInput* storedInputs, uint8_t count, LogicalInput* runtimeInputs);
    
    // Calculate configuration checksum
    uint32_t calculateChecksum(const StoredConfig* config, const uint8_t* variableData, size_t variableSize);
    
    // Validate configuration structure
    bool validateStoredConfig(const StoredConfig* config, size_t totalSize);
}