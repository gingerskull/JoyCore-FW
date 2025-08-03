#pragma once

#include <stdint.h>
#include <stddef.h>

// Abstract interface for platform-agnostic persistent storage
// Provides a common API for configuration storage across different platforms
// (RP2040 LittleFS, ATmega32U4 EEPROM, Teensy 4.0 EEPROM emulation)

enum class StorageResult : uint8_t {
    SUCCESS = 0,
    ERROR_NOT_INITIALIZED,
    ERROR_FILE_NOT_FOUND,
    ERROR_READ_FAILED,
    ERROR_WRITE_FAILED,
    ERROR_INSUFFICIENT_SPACE,
    ERROR_INVALID_PARAMETER,
    ERROR_CORRUPTION_DETECTED
};

class StorageInterface {
public:
    virtual ~StorageInterface() = default;
    
    // Initialize the storage system
    virtual StorageResult initialize() = 0;
    
    // Check if storage system is ready
    virtual bool isInitialized() const = 0;
    
    // Read data from storage
    virtual StorageResult read(const char* filename, uint8_t* buffer, size_t bufferSize, size_t* bytesRead = nullptr) = 0;
    
    // Write data to storage
    virtual StorageResult write(const char* filename, const uint8_t* data, size_t dataSize) = 0;
    
    // Check if file exists
    virtual bool exists(const char* filename) = 0;
    
    // Delete file
    virtual StorageResult remove(const char* filename) = 0;
    
    // Get available storage space (in bytes)
    virtual size_t getAvailableSpace() const = 0;
    
    // Get used storage space (in bytes) 
    virtual size_t getUsedSpace() const = 0;
    
    // Format/clear storage (use with caution)
    virtual StorageResult format() = 0;
    
    // Perform maintenance operations (e.g., wear leveling, garbage collection)
    virtual StorageResult maintenance() = 0;
    
protected:
    bool m_initialized = false;
};