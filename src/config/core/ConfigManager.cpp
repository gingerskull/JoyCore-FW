#include "ConfigManager.h"
#include "../../Config.h"
#include "../ConfigDigital.h"
#include "../ConfigAxis.h"
#include <string.h>
#include <stdio.h>

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
    memset(m_currentLogicalInputs, 0, sizeof(m_currentLogicalInputs));
    memset(m_currentAxisConfigs, 0, sizeof(m_currentAxisConfigs));
    memset(&m_currentUSBDescriptor, 0, sizeof(m_currentUSBDescriptor));
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
#if CONFIG_FEATURE_STORAGE_ENABLED
    // Initialize storage system if enabled
    if (m_storage.initialize() != StorageResult::SUCCESS) {
        // Storage initialization failed, fall back to static mode
        return loadStaticConfiguration();
    }
    
    // Check firmware version and handle fresh uploads
    checkAndUpdateFirmwareVersion();
#endif
    
    m_initialized = true;
    return loadConfiguration();
}

bool ConfigManager::loadConfiguration() {
    if (!m_initialized) {
        return false;
    }
    
#if CONFIG_MODE == CONFIG_MODE_STATIC
    return loadStaticConfiguration();
    
#elif CONFIG_MODE == CONFIG_MODE_STORAGE
    #if CONFIG_FEATURE_STORAGE_ENABLED
        return loadFromStorage();
    #else
        // Storage not available, fall back to static
        return loadStaticConfiguration();
    #endif
    
#elif CONFIG_MODE == CONFIG_MODE_HYBRID
    #if CONFIG_FEATURE_STORAGE_ENABLED
        // Try storage first, fall back to static if failed
        if (loadFromStorage()) {
            return true;
        }
    #endif
    return loadStaticConfiguration();
    
#else
    #error "Invalid CONFIG_MODE specified"
#endif
}

bool ConfigManager::loadStaticConfiguration() {
    // Load compile-time configuration from ConfigDigital.h and ConfigAxis.h
    
    // Copy pin map
    m_currentPinMapCount = min((uint8_t)hardwarePinMapCount, (uint8_t)MAX_PIN_MAP_ENTRIES);
    for (uint8_t i = 0; i < m_currentPinMapCount; i++) {
        m_currentPinMap[i] = hardwarePinMap[i];
    }
    
    // Copy logical inputs  
    m_currentLogicalInputCount = min((uint8_t)logicalInputCount, (uint8_t)MAX_LOGICAL_INPUTS);
    for (uint8_t i = 0; i < m_currentLogicalInputCount; i++) {
        m_currentLogicalInputs[i] = logicalInputs[i];
    }
    
    // Set shift register count
    m_currentShiftRegCount = SHIFTREG_COUNT;
    
    // Load axis configurations from ConfigAxis.h
    generateDefaultAxisConfigs();
    
    // Load USB descriptor from static configuration
    m_currentUSBDescriptor.vendorID = staticUSBDescriptor.vendorID;
    m_currentUSBDescriptor.productID = staticUSBDescriptor.productID;
    strncpy(m_currentUSBDescriptor.manufacturer, staticUSBDescriptor.manufacturer, 
            sizeof(m_currentUSBDescriptor.manufacturer) - 1);
    m_currentUSBDescriptor.manufacturer[sizeof(m_currentUSBDescriptor.manufacturer) - 1] = '\0';
    strncpy(m_currentUSBDescriptor.product, staticUSBDescriptor.product,
            sizeof(m_currentUSBDescriptor.product) - 1);
    m_currentUSBDescriptor.product[sizeof(m_currentUSBDescriptor.product) - 1] = '\0';
    memset(m_currentUSBDescriptor.reserved, 0, sizeof(m_currentUSBDescriptor.reserved));
    
    m_configLoaded = true;
    m_usingDefaults = false;
    return true;
}

#if CONFIG_FEATURE_STORAGE_ENABLED

bool ConfigManager::loadFromStorage() {
    uint8_t buffer[2048]; // Buffer for configuration data
    size_t bytesRead;
    
    StorageResult result = m_storage.read(CONFIG_STORAGE_FILENAME, buffer, sizeof(buffer), &bytesRead);
    
    if (result == StorageResult::ERROR_FILE_NOT_FOUND) {
        // No configuration file exists, generate defaults
        generateDefaultPinMap();
        generateDefaultLogicalInputs(); 
        generateDefaultAxisConfigs();
        generateDefaultUSBDescriptor();
        m_configLoaded = true;
        m_usingDefaults = true;
        return true;
    }
    
    if (result != StorageResult::SUCCESS) {
        return false;
    }
    
    // Validate and parse stored configuration
    if (bytesRead < sizeof(StoredConfig)) {
        return false;
    }
    
    const StoredConfig* storedConfig = reinterpret_cast<const StoredConfig*>(buffer);
    const uint8_t* variableData = buffer + sizeof(StoredConfig);
    size_t variableSize = bytesRead - sizeof(StoredConfig);
    
    if (!ConfigConversion::validateStoredConfig(storedConfig, bytesRead)) {
        return false;
    }
    
    // Convert stored format to runtime format
    return convertStoredToRuntime(storedConfig, variableData, variableSize);
}

bool ConfigManager::saveToStorage() {
    if (!m_initialized || !m_configLoaded) {
        Serial.println("DEBUG: saveToStorage - not initialized or config not loaded");
        return false;
    }
    
    uint8_t buffer[2048];
    size_t totalSize = 0;
    
    if (!getSerializedConfig(buffer, sizeof(buffer), &totalSize)) {
        Serial.println("DEBUG: saveToStorage - getSerializedConfig failed");
        return false;
    }
    
    Serial.print("DEBUG: saveToStorage - about to write ");
    Serial.print(totalSize);
    Serial.println(" bytes");
    
    // Create backup before saving
    createBackup();
    
    StorageResult result = m_storage.write(CONFIG_STORAGE_FILENAME, buffer, totalSize);
    Serial.print("DEBUG: saveToStorage - write result: ");
    Serial.println((int)result);
    
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
    status.currentMode = CONFIG_MODE;
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
        Serial.println("DEBUG: getSerializedConfig - buffer too small");
        return false;
    }
    
    Serial.print("DEBUG: getSerializedConfig - buffer size: ");
    Serial.println(bufferSize);
    Serial.print("DEBUG: StoredConfig size: ");
    Serial.println(sizeof(StoredConfig));
    
    StoredConfig* config = reinterpret_cast<StoredConfig*>(buffer);
    uint8_t* variableData = buffer + sizeof(StoredConfig);
    size_t maxVariableSize = bufferSize - sizeof(StoredConfig);
    size_t variableSize;
    
    Serial.print("DEBUG: Max variable size: ");
    Serial.println(maxVariableSize);
    
    if (!convertRuntimeToStored(config, variableData, &variableSize, maxVariableSize)) {
        Serial.println("DEBUG: convertRuntimeToStored failed");
        return false;
    }
    
    Serial.print("DEBUG: Variable size: ");
    Serial.println(variableSize);
    Serial.print("DEBUG: Total size: ");
    Serial.println(sizeof(StoredConfig) + variableSize);
    
    if (actualSize) {
        *actualSize = sizeof(StoredConfig) + variableSize;
    }
    
    return true;
}

bool ConfigManager::convertStoredToRuntime(const StoredConfig* config, const uint8_t* variableData, size_t variableSize) {
    // Extract pin map
    const StoredPinMapEntry* storedPinMap = reinterpret_cast<const StoredPinMapEntry*>(variableData);
    if (!ConfigConversion::unpackPinMap(storedPinMap, config->pinMapCount, m_currentPinMap)) {
        return false;
    }
    m_currentPinMapCount = config->pinMapCount;
    
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
    
    // Add some basic pin mappings for testing
    if (m_currentPinMapCount < MAX_PIN_MAP_ENTRIES) {
        strncpy((char*)m_currentPinMap[m_currentPinMapCount].name, "6", sizeof(m_currentPinMap[0].name));
        m_currentPinMap[m_currentPinMapCount].type = BTN;
        m_currentPinMapCount++;
    }
}

void ConfigManager::generateDefaultLogicalInputs() {
    // Generate basic default logical inputs
    m_currentLogicalInputCount = 0;
    
    // Add a basic button input for testing
    if (m_currentLogicalInputCount < MAX_LOGICAL_INPUTS) {
        m_currentLogicalInputs[m_currentLogicalInputCount].type = INPUT_PIN;
        m_currentLogicalInputs[m_currentLogicalInputCount].u.pin.pin = 6;
        m_currentLogicalInputs[m_currentLogicalInputCount].u.pin.joyButtonID = 1;
        m_currentLogicalInputs[m_currentLogicalInputCount].u.pin.behavior = NORMAL;
        m_currentLogicalInputs[m_currentLogicalInputCount].u.pin.reverse = 0;
        m_currentLogicalInputCount++;
    }
}

void ConfigManager::generateDefaultAxisConfigs() {
    // Initialize all axes as disabled by default
    for (uint8_t i = 0; i < 8; i++) {
        m_currentAxisConfigs[i].enabled = 0;
        m_currentAxisConfigs[i].pin = 0;
        m_currentAxisConfigs[i].minValue = 0;
        m_currentAxisConfigs[i].maxValue = 32767;
        m_currentAxisConfigs[i].filterLevel = 0; // AXIS_FILTER_OFF
        m_currentAxisConfigs[i].ewmaAlpha = 200;
        m_currentAxisConfigs[i].deadband = 0;
        m_currentAxisConfigs[i].curve = 0; // CURVE_LINEAR
    }
    
    // Enable X and Y axes by default with basic configuration
    m_currentAxisConfigs[0].enabled = 1; // X axis
    m_currentAxisConfigs[0].pin = A1;
    
    m_currentAxisConfigs[1].enabled = 1; // Y axis  
    m_currentAxisConfigs[1].pin = A2;
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
    uint32_t storedVersion = readStoredFirmwareVersion();
    uint32_t currentVersion = FIRMWARE_VERSION;
    
    // If firmware version has changed, this is a fresh upload
    if (storedVersion != currentVersion) {
        // For ALL modes in STORAGE mode, generate minimal defaults on fresh upload
        #if CONFIG_MODE == CONFIG_MODE_STORAGE
            // Generate minimal defaults
            generateDefaultPinMap();
            generateDefaultLogicalInputs();
            generateDefaultAxisConfigs();
            generateDefaultUSBDescriptor();
            m_configLoaded = true;
            m_usingDefaults = true;
            
            // Save the minimal defaults to storage
            saveToStorage();
            
            // Update the stored firmware version
            writeStoredFirmwareVersion(currentVersion);
            
            return true;
        #endif
        
        // For STATIC and HYBRID modes, just update the version
        // They will load configuration normally through loadConfiguration()
        writeStoredFirmwareVersion(currentVersion);
    }
    
    return true; // Version unchanged or non-storage mode, proceed normally
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