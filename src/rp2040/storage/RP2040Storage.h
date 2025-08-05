#pragma once

#include "../../StorageInterface.h"
#include <stdio.h>

#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    #include "LittleFS_Mbed_RP2040.h"
#endif

// RP2040-specific storage implementation
// Uses LittleFS for superior wear leveling with mbed framework
class RP2040Storage : public StorageInterface {
public:
    RP2040Storage();
    virtual ~RP2040Storage();
    
    // StorageInterface implementation
    StorageResult initialize() override;
    bool isInitialized() const override { return m_initialized; }
    
    StorageResult read(const char* filename, uint8_t* buffer, size_t bufferSize, size_t* bytesRead = nullptr) override;
    StorageResult write(const char* filename, const uint8_t* data, size_t dataSize) override;
    
    bool exists(const char* filename) override;
    StorageResult remove(const char* filename) override;
    
    size_t getAvailableSpace() const override;
    size_t getUsedSpace() const override;
    
    StorageResult format() override;
    StorageResult maintenance() override;
    
private:
    // LittleFS implementation
    StorageResult initializeLittleFS();
    
    FILE* openFile(const char* filename, const char* mode);
    
    static constexpr size_t MAX_FILENAME_LENGTH = 32;
};