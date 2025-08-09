#include "ConfigStructs.h"
#include <string.h>
#include <stddef.h>

namespace ConfigConversion {

bool packPinMap(const PinMapEntry* runtimeMap, uint8_t count, StoredPinMapEntry* storedMap) {
    if (!runtimeMap || !storedMap || count == 0) {
        return false;
    }
    
    for (uint8_t i = 0; i < count; i++) {
        // Clear the name field first
        memset(storedMap[i].name, 0, sizeof(storedMap[i].name));
        
        // Copy pin name if it exists (runtimeMap[i].name is a const char* pointer)
        if (runtimeMap[i].name) {
            strncpy(storedMap[i].name, runtimeMap[i].name, sizeof(storedMap[i].name) - 1);
            storedMap[i].name[sizeof(storedMap[i].name) - 1] = '\0';
        }
        
        storedMap[i].type = (uint8_t)runtimeMap[i].type;
        storedMap[i].reserved = 0;
    }
    
    return true;
}

bool unpackPinMap(const StoredPinMapEntry* storedMap, uint8_t count, PinMapEntry* runtimeMap) {
    if (!storedMap || !runtimeMap || count == 0) {
        return false;
    }
    
    for (uint8_t i = 0; i < count; i++) {
        // Note: This creates a potential issue as runtimeMap expects const char* 
        // but we're copying from stored format. In practice, you'd need a static
        // string pool or dynamic allocation here.
        runtimeMap[i].name = storedMap[i].name; // This is problematic - needs better solution
        runtimeMap[i].type = (PinType)storedMap[i].type;
    }
    
    return true;
}

bool packLogicalInputs(const LogicalInput* runtimeInputs, uint8_t count, StoredLogicalInput* storedInputs) {
    if (!runtimeInputs || !storedInputs || count == 0) {
        return false;
    }
    
    for (uint8_t i = 0; i < count; i++) {
        storedInputs[i].type = (uint8_t)runtimeInputs[i].type;
        storedInputs[i].encoderLatchMode = (uint8_t)runtimeInputs[i].encoderLatchMode;
        
        switch (runtimeInputs[i].type) {
            case INPUT_PIN:
                storedInputs[i].behavior = (uint8_t)runtimeInputs[i].u.pin.behavior;
                storedInputs[i].joyButtonID = runtimeInputs[i].u.pin.joyButtonID;
                storedInputs[i].reverse = runtimeInputs[i].u.pin.reverse;
                storedInputs[i].data.pin.pin = runtimeInputs[i].u.pin.pin;
                break;
                
            case INPUT_MATRIX:
                storedInputs[i].behavior = (uint8_t)runtimeInputs[i].u.matrix.behavior;
                storedInputs[i].joyButtonID = runtimeInputs[i].u.matrix.joyButtonID;
                storedInputs[i].reverse = runtimeInputs[i].u.matrix.reverse;
                storedInputs[i].data.matrix.row = runtimeInputs[i].u.matrix.row;
                storedInputs[i].data.matrix.col = runtimeInputs[i].u.matrix.col;
                break;
                
            case INPUT_SHIFTREG:
                storedInputs[i].behavior = (uint8_t)runtimeInputs[i].u.shiftreg.behavior;
                storedInputs[i].joyButtonID = runtimeInputs[i].u.shiftreg.joyButtonID;
                storedInputs[i].reverse = runtimeInputs[i].u.shiftreg.reverse;
                storedInputs[i].data.shiftreg.regIndex = runtimeInputs[i].u.shiftreg.regIndex;
                storedInputs[i].data.shiftreg.bitIndex = runtimeInputs[i].u.shiftreg.bitIndex;
                break;
        }
        
        // Clear reserved fields
        memset(storedInputs[i].reserved, 0, sizeof(storedInputs[i].reserved));
    }
    
    return true;
}

bool unpackLogicalInputs(const StoredLogicalInput* storedInputs, uint8_t count, LogicalInput* runtimeInputs) {
    if (!storedInputs || !runtimeInputs || count == 0) {
        return false;
    }
    
    for (uint8_t i = 0; i < count; i++) {
        runtimeInputs[i].type = (InputType)storedInputs[i].type;
        runtimeInputs[i].encoderLatchMode = (LatchMode)storedInputs[i].encoderLatchMode;
        
        switch (runtimeInputs[i].type) {
            case INPUT_PIN:
                runtimeInputs[i].u.pin.behavior = (ButtonBehavior)storedInputs[i].behavior;
                runtimeInputs[i].u.pin.joyButtonID = storedInputs[i].joyButtonID;
                runtimeInputs[i].u.pin.reverse = storedInputs[i].reverse;
                runtimeInputs[i].u.pin.pin = storedInputs[i].data.pin.pin;
                break;
                
            case INPUT_MATRIX:
                runtimeInputs[i].u.matrix.behavior = (ButtonBehavior)storedInputs[i].behavior;
                runtimeInputs[i].u.matrix.joyButtonID = storedInputs[i].joyButtonID;
                runtimeInputs[i].u.matrix.reverse = storedInputs[i].reverse;
                runtimeInputs[i].u.matrix.row = storedInputs[i].data.matrix.row;
                runtimeInputs[i].u.matrix.col = storedInputs[i].data.matrix.col;
                break;
                
            case INPUT_SHIFTREG:
                runtimeInputs[i].u.shiftreg.behavior = (ButtonBehavior)storedInputs[i].behavior;
                runtimeInputs[i].u.shiftreg.joyButtonID = storedInputs[i].joyButtonID;
                runtimeInputs[i].u.shiftreg.reverse = storedInputs[i].reverse;
                runtimeInputs[i].u.shiftreg.regIndex = storedInputs[i].data.shiftreg.regIndex;
                runtimeInputs[i].u.shiftreg.bitIndex = storedInputs[i].data.shiftreg.bitIndex;
                break;
        }
    }
    
    return true;
}

uint32_t calculateChecksum(const StoredConfig* config, const uint8_t* variableData, size_t variableSize) {
    if (!config) {
        return 0;
    }
    
    // Simple CRC32-like checksum (not cryptographically secure)
    uint32_t checksum = 0xFFFFFFFF;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(config);
    
    // Skip the checksum field itself
    size_t headerSize = offsetof(ConfigHeader, checksum);
    
    // Checksum the header up to the checksum field
    for (size_t i = 0; i < headerSize; i++) {
        checksum ^= data[i];
        for (int j = 0; j < 8; j++) {
            checksum = (checksum >> 1) ^ (0xEDB88320 & (-(checksum & 1)));
        }
    }
    
    // Skip checksum field and continue with rest of header
    size_t remainingHeaderSize = sizeof(ConfigHeader) - headerSize - sizeof(config->header.checksum);
    const uint8_t* afterChecksum = data + headerSize + sizeof(config->header.checksum);
    
    for (size_t i = 0; i < remainingHeaderSize; i++) {
        checksum ^= afterChecksum[i];
        for (int j = 0; j < 8; j++) {
            checksum = (checksum >> 1) ^ (0xEDB88320 & (-(checksum & 1)));
        }
    }
    
    // Checksum the rest of the config structure
    const uint8_t* configData = data + sizeof(ConfigHeader);
    size_t configDataSize = sizeof(StoredConfig) - sizeof(ConfigHeader);
    
    for (size_t i = 0; i < configDataSize; i++) {
        checksum ^= configData[i];
        for (int j = 0; j < 8; j++) {
            checksum = (checksum >> 1) ^ (0xEDB88320 & (-(checksum & 1)));
        }
    }
    
    // Checksum the variable data
    if (variableData && variableSize > 0) {
        for (size_t i = 0; i < variableSize; i++) {
            checksum ^= variableData[i];
            for (int j = 0; j < 8; j++) {
                checksum = (checksum >> 1) ^ (0xEDB88320 & (-(checksum & 1)));
            }
        }
    }
    
    return ~checksum;
}

bool validateStoredConfig(const StoredConfig* config, size_t totalSize) {
    if (!config || totalSize < sizeof(StoredConfig)) {
        return false;
    }
    
    // Check magic number
    if (config->header.magic != CONFIG_MAGIC) {
        return false;
    }
    
    // Check version compatibility
    if (config->header.version > CONFIG_VERSION) {
        return false; // Future version, can't handle
    }
    
    // Check size consistency
    if (config->header.size != totalSize) {
        return false;
    }
    
    // Check bounds
    if (config->pinMapCount > MAX_PIN_MAP_ENTRIES ||
        config->logicalInputCount > MAX_LOGICAL_INPUTS ||
        config->shiftRegCount > MAX_SHIFT_REGISTERS) {
        return false;
    }
    
    // Calculate expected variable data size
    size_t expectedVariableSize = config->pinMapCount * sizeof(StoredPinMapEntry) +
                                  config->logicalInputCount * sizeof(StoredLogicalInput);
    
    if (totalSize != sizeof(StoredConfig) + expectedVariableSize) {
        return false;
    }
    
    // Verify checksum
    const uint8_t* variableData = reinterpret_cast<const uint8_t*>(config) + sizeof(StoredConfig);
    uint32_t calculatedChecksum = calculateChecksum(config, variableData, expectedVariableSize);
    
    if (calculatedChecksum != config->header.checksum) {
        return false;
    }
    
    return true;
}

} // namespace ConfigConversion