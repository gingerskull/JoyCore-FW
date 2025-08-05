#include "RP2040Storage.h"
#include "../../config/core/ConfigMode.h"
#include <string.h>
#include <stdio.h>

#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    #include "LittleFS_Mbed_RP2040.h"
    // Create static instance of LittleFS_MBED
    static LittleFS_MBED* littleFS_instance = nullptr;
#endif

RP2040Storage::RP2040Storage() : StorageInterface() {
}

RP2040Storage::~RP2040Storage() {
}

StorageResult RP2040Storage::initialize() {
    if (m_initialized) {
        return StorageResult::SUCCESS;
    }
    
    return initializeLittleFS();
}

StorageResult RP2040Storage::initializeLittleFS() {
#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    // Create LittleFS instance if needed
    if (!littleFS_instance) {
        littleFS_instance = new LittleFS_MBED();
    }
    
    // Initialize LittleFS - it will auto-format if no filesystem is detected
    if (!littleFS_instance->init()) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    m_initialized = true;
    return StorageResult::SUCCESS;
#else
    // Storage disabled, return error
    return StorageResult::ERROR_NOT_INITIALIZED;
#endif
}

StorageResult RP2040Storage::read(const char* filename, uint8_t* buffer, size_t bufferSize, size_t* bytesRead) {
    if (!m_initialized) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    if (!filename || !buffer || bufferSize == 0) {
        return StorageResult::ERROR_INVALID_PARAMETER;
    }
    
    FILE* file = this->openFile(filename, "r");
    if (!file) {
        return StorageResult::ERROR_FILE_NOT_FOUND;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    size_t readSize = (bufferSize < fileSize) ? bufferSize : fileSize;
    
    size_t actualBytesRead = fread(buffer, 1, readSize, file);
    fclose(file);
    
    if (bytesRead) {
        *bytesRead = actualBytesRead;
    }
    
    return (actualBytesRead == readSize) ? StorageResult::SUCCESS : StorageResult::ERROR_READ_FAILED;
}

StorageResult RP2040Storage::write(const char* filename, const uint8_t* data, size_t dataSize) {
    if (!m_initialized) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    if (!filename || !data || dataSize == 0) {
        return StorageResult::ERROR_INVALID_PARAMETER;
    }
    
    // Check available space
    if (dataSize > getAvailableSpace()) {
        return StorageResult::ERROR_INSUFFICIENT_SPACE;
    }
    
    FILE* file = this->openFile(filename, "w");
    if (!file) {
        return StorageResult::ERROR_WRITE_FAILED;
    }
    
    size_t bytesWritten = fwrite(data, 1, dataSize, file);
    fclose(file);
    
    return (bytesWritten == dataSize) ? StorageResult::SUCCESS : StorageResult::ERROR_WRITE_FAILED;
}

bool RP2040Storage::exists(const char* filename) {
    if (!m_initialized || !filename) {
        return false;
    }
    
    // Check if file exists by trying to open it
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

StorageResult RP2040Storage::remove(const char* filename) {
    if (!m_initialized) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    if (!filename) {
        return StorageResult::ERROR_INVALID_PARAMETER;
    }
    
    if (!exists(filename)) {
        return StorageResult::ERROR_FILE_NOT_FOUND;
    }
    
    return (::remove(filename) == 0) ? StorageResult::SUCCESS : StorageResult::ERROR_WRITE_FAILED;
}

size_t RP2040Storage::getAvailableSpace() const {
    if (!m_initialized) {
        return 0;
    }
    
    // For mbed RP2040, return a reasonable estimate
    // The actual implementation would need to use filesystem-specific methods
    return 1024 * 1024; // 1MB estimate
}

size_t RP2040Storage::getUsedSpace() const {
    if (!m_initialized) {
        return 0;
    }
    
    // For mbed RP2040, return a reasonable estimate
    // The actual implementation would need to use filesystem-specific methods
    return 0; // Placeholder
}

StorageResult RP2040Storage::format() {
#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    // Note: LittleFS auto-formats, so this is mainly for explicit clearing
    m_initialized = false;
    if (littleFS_instance) {
        littleFS_instance->unmount();
    }
    
    // Reinitialize (which will auto-format if needed)
    return initialize();
#else
    return StorageResult::ERROR_NOT_INITIALIZED;
#endif
}

StorageResult RP2040Storage::maintenance() {
    // LittleFS handles wear leveling automatically, no manual maintenance needed
    return StorageResult::SUCCESS;
}

FILE* RP2040Storage::openFile(const char* filename, const char* mode) {
#if CONFIG_FEATURE_STORAGE_ENABLED
    return fopen(filename, mode);
#else
    return nullptr;
#endif
}