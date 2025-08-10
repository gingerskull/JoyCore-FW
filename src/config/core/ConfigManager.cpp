#include "ConfigManager.h"
#include "../../Config.h"
#include "../ConfigDigital.h"
#include "../ConfigAxis.h"
#include <string.h>
#include <stdio.h>
#include "../../utils/Debug.h"

// Global instance
ConfigManager g_configManager;

ConfigManager::ConfigManager() 
    : m_currentPinMapCount(0)
    , m_currentLogicalInputCount(0) 
    , m_currentShiftRegCount(0)
    , m_initialized(false)
    , m_configLoaded(false)
    , m_usingDefaults(false)
{
    memset(m_currentPinMap, 0, sizeof(m_currentPinMap));
    memset(m_pinNamePool, 0, sizeof(m_pinNamePool));
    memset(m_currentLogicalInputs, 0, sizeof(m_currentLogicalInputs));
    memset(m_currentAxisConfigs, 0, sizeof(m_currentAxisConfigs));
    memset(&m_currentUSBDescriptor, 0, sizeof(m_currentUSBDescriptor));
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::initialize() {
    if (m_initialized) {
        DEBUG_PRINTLN("DEBUG: ConfigManager already initialized, returning true");
        return true;
    }
    DEBUG_PRINTLN("DEBUG: ConfigManager::initialize() called - ENTRY POINT");
    
    // Initialize storage system (mandatory)
    DEBUG_PRINTLN("DEBUG: Initializing storage system...");
    StorageResult storageResult = m_storage.initialize();
    DEBUG_PRINT("DEBUG: Storage initialization result: "); DEBUG_PRINTLN((int)storageResult);
    if (storageResult != StorageResult::SUCCESS) {
        DEBUG_PRINTLN("ERROR: Storage initialization failed - cannot proceed (no static fallback)");
        return false; // Hard failure now that static mode removed
    }
    DEBUG_PRINTLN("DEBUG: Storage initialized, dumping file table:");
    m_storage.debugDumpFileTable();
    m_initialized = true; // set before version check
    DEBUG_PRINTLN("DEBUG: About to call checkAndUpdateFirmwareVersion...");
    bool versionResult = checkAndUpdateFirmwareVersion();
    DEBUG_PRINT("DEBUG: checkAndUpdateFirmwareVersion returned: "); DEBUG_PRINTLN(versionResult ? "true" : "false");
    
    // If config was already loaded during firmware version check, don't load again
    if (m_configLoaded) {
    DEBUG_PRINTLN("DEBUG: Config already loaded during version check, skipping loadConfiguration");
        return true;
    }
    
    return loadConfiguration();
}

bool ConfigManager::loadConfiguration() {
    if (!m_initialized) return false;
    // Attempt primary load
    if (loadFromStorage()) return true;
    Serial.println("WARN: Primary config load failed, attempting backup restore");
    if (restoreFromBackup()) {
        Serial.println("INFO: Backup restored, re-attempting load");
        if (loadFromStorage()) return true;
    }
    Serial.println("WARN: No valid config found, generating defaults");
    generateDefaultPinMap();
    generateDefaultLogicalInputs();
    generateDefaultAxisConfigs();
    generateDefaultUSBDescriptor();
    m_configLoaded = true;
    m_usingDefaults = true;
    saveToStorage();
    return true;
}

// loadStaticConfiguration removed (always storage-based). Defaults are generated via generateDefault* helpers.

#if CONFIG_FEATURE_STORAGE_ENABLED

bool ConfigManager::loadFromStorage() {
    uint8_t buffer[2048]; // Buffer for configuration data
    size_t bytesRead;
    
    DEBUG_PRINTLN("DEBUG: loadFromStorage() called");
    
    StorageResult result = m_storage.read(CONFIG_STORAGE_FILENAME, buffer, sizeof(buffer), &bytesRead);
    DEBUG_PRINT("DEBUG: Read result for "); DEBUG_PRINT(CONFIG_STORAGE_FILENAME); DEBUG_PRINT(": "); DEBUG_PRINTLN((int)result);
    
    if (result == StorageResult::ERROR_FILE_NOT_FOUND) {
    DEBUG_PRINTLN("DEBUG: Config file not found, generating defaults and saving...");
        // No configuration file exists, generate defaults
        generateDefaultPinMap();
        generateDefaultLogicalInputs();
        generateDefaultAxisConfigs();
        generateDefaultUSBDescriptor();
        m_configLoaded = true;
        m_usingDefaults = true;
        
        // Save the defaults to storage so they exist for next time
    DEBUG_PRINTLN("DEBUG: Saving generated defaults to storage...");
        bool saveResult = saveToStorage();
    DEBUG_PRINT("DEBUG: Save defaults result: "); DEBUG_PRINTLN(saveResult ? "SUCCESS" : "FAILED");
        
        // Also create firmware version file
    DEBUG_PRINTLN("DEBUG: Writing firmware version file...");
        writeStoredFirmwareVersion(FIRMWARE_VERSION);
        
        return true;
    }
    
    if (result != StorageResult::SUCCESS) {
    DEBUG_PRINT("DEBUG: Failed to read config, error: "); DEBUG_PRINTLN((int)result);
        return false; // Higher-level will attempt fallback chain
    }
    
    // Validate and parse stored configuration
    if (bytesRead < sizeof(StoredConfig)) {
        return false;
    }
    
    const StoredConfig* storedConfig = reinterpret_cast<const StoredConfig*>(buffer);
    const uint8_t* variableData = buffer + sizeof(StoredConfig);
    size_t variableSize = bytesRead - sizeof(StoredConfig);
    
    if (!ConfigConversion::validateStoredConfig(storedConfig, bytesRead)) {
    DEBUG_PRINTLN("DEBUG: Stored config failed validation (possibly corrupt)");
        return false; // Trigger fallback chain
    }

    bool ok = convertStoredToRuntime(storedConfig, variableData, variableSize);
    if(!ok) {
    DEBUG_PRINTLN("DEBUG: convertStoredToRuntime failed");
    }
    return ok;
}

bool ConfigManager::saveToStorage() {
    if (!m_initialized || !m_configLoaded) {
        DEBUG_PRINTLN("DEBUG: saveToStorage - not initialized or config not loaded");
        return false;
    }
    
    uint8_t buffer[2048];
    size_t totalSize = 0;
    
    if (!getSerializedConfig(buffer, sizeof(buffer), &totalSize)) {
        Serial.println("DEBUG: saveToStorage - getSerializedConfig failed");
        return false;
    }
    
    DEBUG_PRINT("DEBUG: saveToStorage - about to write "); DEBUG_PRINT(totalSize); DEBUG_PRINTLN(" bytes");
    
    // Create backup before saving
    createBackup();
    
    StorageResult result = m_storage.write(CONFIG_STORAGE_FILENAME, buffer, totalSize);
    DEBUG_PRINT("DEBUG: saveToStorage - write result: "); DEBUG_PRINTLN((int)result);
    
    return result == StorageResult::SUCCESS;
}

bool ConfigManager::createBackup() {
    // Copy current config file to backup
    uint8_t buffer[2048];
    size_t bytesRead;
    
    StorageResult result = m_storage.read(CONFIG_STORAGE_FILENAME, buffer, sizeof(buffer), &bytesRead);
    if (result != StorageResult::SUCCESS) {
        return false; // No existing config to backup
    }
    
    result = m_storage.write(CONFIG_STORAGE_BACKUP_FILENAME, buffer, bytesRead);
    return result == StorageResult::SUCCESS;
}

bool ConfigManager::restoreFromBackup() {
    uint8_t buffer[2048];
    size_t bytesRead;
    
    StorageResult result = m_storage.read(CONFIG_STORAGE_BACKUP_FILENAME, buffer, sizeof(buffer), &bytesRead);
    if (result != StorageResult::SUCCESS) {
        return false;
    }
    
    result = m_storage.write(CONFIG_STORAGE_FILENAME, buffer, bytesRead);
    return result == StorageResult::SUCCESS;
}

#endif // CONFIG_FEATURE_STORAGE_ENABLED

bool ConfigManager::saveConfiguration() {
#if CONFIG_FEATURE_STORAGE_ENABLED
    return saveToStorage();
#else
    return false; // Storage not available
#endif
}

bool ConfigManager::resetToDefaults() {
    generateDefaultPinMap();
    generateDefaultLogicalInputs();
    generateDefaultAxisConfigs();
    
    m_configLoaded = true;
    m_usingDefaults = true;
    
#if CONFIG_FEATURE_STORAGE_ENABLED
    // Save defaults to storage
    return saveToStorage();
#else
    return true;
#endif
}

ConfigStatus ConfigManager::getStatus() const {
    ConfigStatus status;
    memset(&status, 0, sizeof(status));
    
    status.configLoaded = m_configLoaded;
    status.usingDefaults = m_usingDefaults;
    status.configVersion = CONFIG_VERSION;
    
#if CONFIG_FEATURE_STORAGE_ENABLED
    status.storageInitialized = m_storage.isInitialized();
    status.storageUsed = m_storage.getUsedSpace();
    status.storageAvailable = m_storage.getAvailableSpace();
#endif
    
    return status;
}

ConfigValidationResult ConfigManager::validateConfiguration(const StoredConfig* config) const {
    ConfigValidationResult result;
    memset(&result, 0, sizeof(result));
    
    // Basic structure validation
    if (!config || !ConfigConversion::validateStoredConfig(config, sizeof(StoredConfig))) {
        result.isValid = false;
        result.errorCount = 1;
        strncpy(result.firstError, "Invalid config structure", sizeof(result.firstError) - 1);
        return result;
    }
    
    // Validate pin map and logical inputs
    // (Implementation would include detailed validation logic)
    
    result.isValid = true;
    return result;
}

const StoredAxisConfig* ConfigManager::getAxisConfig(uint8_t axisIndex) const {
    if (axisIndex >= 8 || !m_currentAxisConfigs[axisIndex].enabled) {
        return nullptr;
    }
    return &m_currentAxisConfigs[axisIndex];
}

bool ConfigManager::isAxisEnabled(uint8_t axisIndex) const {
    return axisIndex < 8 && m_currentAxisConfigs[axisIndex].enabled;
}

bool ConfigManager::applyConfiguration(const StoredConfig* config, const uint8_t* variableData, size_t variableSize) {
    if (!ConfigConversion::validateStoredConfig(config, sizeof(StoredConfig) + variableSize)) {
        return false;
    }
    
    return convertStoredToRuntime(config, variableData, variableSize);
}

bool ConfigManager::getSerializedConfig(uint8_t* buffer, size_t bufferSize, size_t* actualSize) const {
    if (!buffer || bufferSize < sizeof(StoredConfig)) {
        DEBUG_PRINTLN("DEBUG: getSerializedConfig - buffer too small");
        return false;
    }
    
    DEBUG_PRINT("DEBUG: getSerializedConfig - buffer size: "); DEBUG_PRINTLN(bufferSize);
    DEBUG_PRINT("DEBUG: StoredConfig size: "); DEBUG_PRINTLN(sizeof(StoredConfig));
    
    StoredConfig* config = reinterpret_cast<StoredConfig*>(buffer);
    uint8_t* variableData = buffer + sizeof(StoredConfig);
    size_t maxVariableSize = bufferSize - sizeof(StoredConfig);
    size_t variableSize;
    
    DEBUG_PRINT("DEBUG: Max variable size: "); DEBUG_PRINTLN(maxVariableSize);
    
    if (!convertRuntimeToStored(config, variableData, &variableSize, maxVariableSize)) {
    DEBUG_PRINTLN("DEBUG: convertRuntimeToStored failed");
        return false;
    }
    
    DEBUG_PRINT("DEBUG: Variable size: "); DEBUG_PRINTLN(variableSize);
    DEBUG_PRINT("DEBUG: Total size: "); DEBUG_PRINTLN(sizeof(StoredConfig) + variableSize);
    
    if (actualSize) {
        *actualSize = sizeof(StoredConfig) + variableSize;
    }
    
    return true;
}

bool ConfigManager::convertStoredToRuntime(const StoredConfig* config, const uint8_t* variableData, size_t variableSize) {
    // Extract pin map
    const StoredPinMapEntry* storedPinMap = reinterpret_cast<const StoredPinMapEntry*>(variableData);
    m_currentPinMapCount = min(config->pinMapCount, (uint8_t)MAX_PIN_MAP_ENTRIES);
    for(uint8_t i=0;i<m_currentPinMapCount;i++) {
        // Copy name into stable pool then point runtime entry to it
        strncpy(m_pinNamePool[i], storedPinMap[i].name, sizeof(m_pinNamePool[i]) - 1);
        m_pinNamePool[i][sizeof(m_pinNamePool[i]) - 1] = '\0';
        m_currentPinMap[i].name = m_pinNamePool[i];
        m_currentPinMap[i].type = (PinType)storedPinMap[i].type;
    }
    
    // Extract logical inputs
    const StoredLogicalInput* storedInputs = reinterpret_cast<const StoredLogicalInput*>(
        variableData + config->pinMapCount * sizeof(StoredPinMapEntry));
    if (!ConfigConversion::unpackLogicalInputs(storedInputs, config->logicalInputCount, m_currentLogicalInputs)) {
        return false;
    }
    m_currentLogicalInputCount = config->logicalInputCount;
    
    // Copy axis configurations
    memcpy(m_currentAxisConfigs, config->axes, sizeof(config->axes));
    
    // Copy USB descriptor
    memcpy(&m_currentUSBDescriptor, &config->usbDescriptor, sizeof(config->usbDescriptor));
    
    m_currentShiftRegCount = config->shiftRegCount;
    m_configLoaded = true;
    m_usingDefaults = false;
    
    return true;
}

bool ConfigManager::convertRuntimeToStored(StoredConfig* config, uint8_t* variableData, size_t* variableSize, size_t maxVariableSize) const {
    if (!config || !variableData || !variableSize) {
        return false;
    }
    
    // Initialize header
    config->header.magic = CONFIG_MAGIC;
    config->header.version = CONFIG_VERSION;
    config->header.size = sizeof(StoredConfig);
    
    // Set counts
    config->pinMapCount = m_currentPinMapCount;
    config->logicalInputCount = m_currentLogicalInputCount;
    config->shiftRegCount = m_currentShiftRegCount;
    
    // Copy axis configurations
    memcpy(config->axes, m_currentAxisConfigs, sizeof(config->axes));
    
    // Copy USB descriptor
    memcpy(&config->usbDescriptor, &m_currentUSBDescriptor, sizeof(config->usbDescriptor));
    
    // Pack variable data
    size_t offset = 0;
    
    // Pack pin map
    StoredPinMapEntry* storedPinMap = reinterpret_cast<StoredPinMapEntry*>(variableData + offset);
    if (!ConfigConversion::packPinMap(m_currentPinMap, m_currentPinMapCount, storedPinMap)) {
        return false;
    }
    offset += m_currentPinMapCount * sizeof(StoredPinMapEntry);
    
    // Pack logical inputs
    StoredLogicalInput* storedInputs = reinterpret_cast<StoredLogicalInput*>(variableData + offset);
    if (!ConfigConversion::packLogicalInputs(m_currentLogicalInputs, m_currentLogicalInputCount, storedInputs)) {
        return false;
    }
    offset += m_currentLogicalInputCount * sizeof(StoredLogicalInput);
    
    *variableSize = offset;
    config->header.size += offset;
    
    // Calculate checksum
    config->header.checksum = ConfigConversion::calculateChecksum(config, variableData, *variableSize);
    
    return true;
}

void ConfigManager::generateDefaultPinMap() {
    // Generate basic default pin map - this would typically match ConfigDigital.h defaults
    m_currentPinMapCount = 0;
    // Minimal sane defaults: copy any static hardwarePinMap entries up to limits
    uint8_t copyCount = min((uint8_t)hardwarePinMapCount, (uint8_t)MAX_PIN_MAP_ENTRIES);
    for(uint8_t i=0;i<copyCount;i++) {
        strncpy(m_pinNamePool[i], hardwarePinMap[i].name, sizeof(m_pinNamePool[i]) - 1);
        m_pinNamePool[i][sizeof(m_pinNamePool[i]) - 1] = '\0';
        m_currentPinMap[i].name = m_pinNamePool[i];
        m_currentPinMap[i].type = hardwarePinMap[i].type;
        m_currentPinMapCount++;
    }
}

void ConfigManager::generateDefaultLogicalInputs() {
    // Mirror static logicalInputs[] from ConfigDigital.h exactly
    m_currentLogicalInputCount = min((uint8_t)logicalInputCount, (uint8_t)MAX_LOGICAL_INPUTS);
    for(uint8_t i=0;i<m_currentLogicalInputCount;i++) {
        m_currentLogicalInputs[i] = logicalInputs[i]; // struct copy (includes union + encoderLatchMode)
    }
}

void ConfigManager::generateDefaultAxisConfigs() {
    // Start with all axes disabled
    for(uint8_t i=0;i<8;i++) {
        m_currentAxisConfigs[i].enabled = 0;
        m_currentAxisConfigs[i].pin = 0;
        m_currentAxisConfigs[i].minValue = 0;
        m_currentAxisConfigs[i].maxValue = 0;
        m_currentAxisConfigs[i].filterLevel = 0;
        m_currentAxisConfigs[i].ewmaAlpha = 0;
        m_currentAxisConfigs[i].deadband = 0;
        m_currentAxisConfigs[i].curve = 0;
        memset(m_currentAxisConfigs[i].reserved, 0, sizeof(m_currentAxisConfigs[i].reserved));
    }

    // Populate from axisDescriptors[] defined in ConfigAxis.h (reflecting user/static config)
    for (auto &d : axisDescriptors) {
        if (d.idx >= 8) continue; // safety
        m_currentAxisConfigs[d.idx].enabled = 1;
        m_currentAxisConfigs[d.idx].pin = (uint8_t)d.pin; // assumes pin fits in uint8_t for built-in / ADS proxy values
        m_currentAxisConfigs[d.idx].minValue = (uint16_t)d.minv;
        m_currentAxisConfigs[d.idx].maxValue = (uint16_t)d.maxv;
        m_currentAxisConfigs[d.idx].filterLevel = (uint8_t)d.filter;
        m_currentAxisConfigs[d.idx].ewmaAlpha = (uint16_t)d.alpha;
        m_currentAxisConfigs[d.idx].deadband = (uint16_t)d.deadband;
        m_currentAxisConfigs[d.idx].curve = (uint8_t)d.curve;
    }
}

void ConfigManager::generateDefaultUSBDescriptor() {
    // Use the USB descriptor from static configuration (ConfigDigital.h)
    // This ensures consistent USB identity regardless of config mode
    m_currentUSBDescriptor.vendorID = staticUSBDescriptor.vendorID;
    m_currentUSBDescriptor.productID = staticUSBDescriptor.productID;
    
    strncpy(m_currentUSBDescriptor.manufacturer, staticUSBDescriptor.manufacturer, 
            sizeof(m_currentUSBDescriptor.manufacturer) - 1);
    m_currentUSBDescriptor.manufacturer[sizeof(m_currentUSBDescriptor.manufacturer) - 1] = '\0';
    
    strncpy(m_currentUSBDescriptor.product, staticUSBDescriptor.product,
            sizeof(m_currentUSBDescriptor.product) - 1);
    m_currentUSBDescriptor.product[sizeof(m_currentUSBDescriptor.product) - 1] = '\0';
    
    // Clear reserved bytes
    memset(m_currentUSBDescriptor.reserved, 0, sizeof(m_currentUSBDescriptor.reserved));
}

#if CONFIG_FEATURE_STORAGE_ENABLED

bool ConfigManager::checkAndUpdateFirmwareVersion() {
    DEBUG_PRINTLN("DEBUG: checkAndUpdateFirmwareVersion() - ENTRY");
    uint32_t storedVersion = readStoredFirmwareVersion();
    uint32_t currentVersion = FIRMWARE_VERSION;
    DEBUG_PRINT("DEBUG: checkAndUpdateFirmwareVersion - stored: "); DEBUG_PRINT(storedVersion); DEBUG_PRINT(", current: "); DEBUG_PRINTLN(currentVersion);
    if (storedVersion != currentVersion) {
        DEBUG_PRINTLN("DEBUG: Firmware version changed -> regenerating defaults and saving");
        generateDefaultPinMap();
        generateDefaultLogicalInputs();
        generateDefaultAxisConfigs();
        generateDefaultUSBDescriptor();
        m_configLoaded = true;
        m_usingDefaults = true;
        bool saveResult = saveConfiguration();
        DEBUG_PRINT("DEBUG: saveConfiguration() returned: "); DEBUG_PRINTLN(saveResult ? "SUCCESS" : "FAILED");
        if (saveResult) {
            if (!writeStoredFirmwareVersion(currentVersion)) {
                DEBUG_PRINTLN("ERROR: Failed to update firmware version file");
                return false;
            }
        } else {
            DEBUG_PRINTLN("ERROR: Config save failed after firmware version change");
            return false;
        }
    } else {
        DEBUG_PRINTLN("DEBUG: Firmware version unchanged");
    }
    return true;
}

uint32_t ConfigManager::readStoredFirmwareVersion() {
    uint8_t buffer[16];
    size_t bytesRead;
    
    StorageResult result = m_storage.read(CONFIG_STORAGE_FIRMWARE_VERSION, buffer, sizeof(buffer), &bytesRead);
    
    if (result == StorageResult::SUCCESS && bytesRead >= 4) {
        // Parse version number from file
        uint32_t version = 0;
        for (size_t i = 0; i < bytesRead && i < 10; i++) {
            if (buffer[i] >= '0' && buffer[i] <= '9') {
                version = version * 10 + (buffer[i] - '0');
            } else {
                break;
            }
        }
        return version;
    }
    
    return 0; // No version file or read failed
}

bool ConfigManager::writeStoredFirmwareVersion(uint32_t version) {
    char buffer[16];
    int len = snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)version);
    
    if (len > 0 && len < sizeof(buffer)) {
        StorageResult result = m_storage.write(CONFIG_STORAGE_FIRMWARE_VERSION, 
                                              (const uint8_t*)buffer, len);
        return result == StorageResult::SUCCESS;
    }
    
    return false;
}

#endif // CONFIG_FEATURE_STORAGE_ENABLED