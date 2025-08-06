#pragma once

#include "ConfigMode.h"
#include "ConfigStructs.h"

#if CONFIG_FEATURE_STORAGE_ENABLED
    #include "../../StorageInterface.h"
    #include "../../rp2040/storage/RP2040Storage.h"
#endif

#include <stdint.h>

// Configuration Manager - Handles loading, saving, and switching between configuration modes
// Provides a unified interface for configuration regardless of source (compile-time vs storage)
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Initialize the configuration system
    bool initialize();
    
    // Load configuration based on current CONFIG_MODE
    bool loadConfiguration();
    
    // Save current configuration to storage (if storage mode enabled)
    bool saveConfiguration();
    
    // Reset to factory defaults
    bool resetToDefaults();
    
    // Get current configuration status
    ConfigStatus getStatus() const;
    
    // Validate a configuration without applying it
    ConfigValidationResult validateConfiguration(const StoredConfig* config) const;
    
    // Configuration access methods
    const PinMapEntry* getPinMap() const { return m_currentPinMap; }
    uint8_t getPinMapCount() const { return m_currentPinMapCount; }
    
    const LogicalInput* getLogicalInputs() const { return m_currentLogicalInputs; }
    uint8_t getLogicalInputCount() const { return m_currentLogicalInputCount; }
    
    uint8_t getShiftRegisterCount() const { return m_currentShiftRegCount; }
    
    // Axis configuration access (returns nullptr if axis not enabled)
    const StoredAxisConfig* getAxisConfig(uint8_t axisIndex) const;
    bool isAxisEnabled(uint8_t axisIndex) const;
    
    // USB descriptor configuration access
    const StoredUSBDescriptor* getUSBDescriptor() const { return &m_currentUSBDescriptor; }
    
    // Hot-reload configuration (for runtime updates via USB)
    bool applyConfiguration(const StoredConfig* config, const uint8_t* variableData, size_t variableSize);
    
    // Get configuration in serialized format (for USB transmission)
    bool getSerializedConfig(uint8_t* buffer, size_t bufferSize, size_t* actualSize) const;
    
private:
    // Current active configuration
    PinMapEntry m_currentPinMap[MAX_PIN_MAP_ENTRIES];
    LogicalInput m_currentLogicalInputs[MAX_LOGICAL_INPUTS];
    StoredAxisConfig m_currentAxisConfigs[8];
    StoredUSBDescriptor m_currentUSBDescriptor;
    
    uint8_t m_currentPinMapCount;
    uint8_t m_currentLogicalInputCount;
    uint8_t m_currentShiftRegCount;
    
    bool m_initialized;
    bool m_configLoaded;
    bool m_usingDefaults;
    
#if CONFIG_FEATURE_STORAGE_ENABLED
    RP2040Storage m_storage;
    
    // Storage-based configuration methods
    bool loadFromStorage();
    bool saveToStorage();
    bool createBackup();
    bool restoreFromBackup();
#endif
    
    // Static configuration methods (compile-time config)
    bool loadStaticConfiguration();
    
    // Conversion helpers
    bool convertStaticToRuntime();
    bool convertStoredToRuntime(const StoredConfig* config, const uint8_t* variableData, size_t variableSize);
    bool convertRuntimeToStored(StoredConfig* config, uint8_t* variableData, size_t* variableSize, size_t maxVariableSize) const;
    
    // Validation helpers
    bool validatePinMap(const PinMapEntry* pinMap, uint8_t count) const;
    bool validateLogicalInputs(const LogicalInput* inputs, uint8_t count) const;
    bool validateAxisConfig(const StoredAxisConfig* config) const;
    
    // Default configuration generators
    void generateDefaultPinMap();
    void generateDefaultLogicalInputs();
    void generateDefaultAxisConfigs();
    void generateDefaultUSBDescriptor();
    
    // Utility methods
    bool isValidPin(const char* pinName) const;
    bool isValidJoystickButton(uint8_t buttonID) const;
    bool isValidAnalogPin(uint8_t pin) const;
};

// Global configuration manager instance
extern ConfigManager g_configManager;