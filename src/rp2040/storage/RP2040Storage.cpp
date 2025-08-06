#include "RP2040Storage.h"
#include "../../config/core/ConfigMode.h"
#include <string.h>
#include <stdio.h>

#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    #include <LittleFS.h>
    #include <VFS.h>
    // Use built-in LittleFS from arduino-pico
    static bool littleFS_initialized = false;
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
    // Initialize built-in LittleFS
    if (!littleFS_initialized) {
        if (!LittleFS.begin()) {
            return StorageResult::ERROR_NOT_INITIALIZED;
        }
        
        // Set VFS root to LittleFS for POSIX file operations
        VFS.root(LittleFS);
        littleFS_initialized = true;
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
        Serial.print("DEBUG: RP2040Storage::read - failed to open file: ");
        Serial.println(filename);
        return StorageResult::ERROR_FILE_NOT_FOUND;
    }
    
    Serial.print("DEBUG: RP2040Storage::read - reading from ");
    Serial.println(filename);
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    Serial.print("DEBUG: RP2040Storage::read - file size: ");
    Serial.println(fileSize);
    
    size_t readSize = (bufferSize < fileSize) ? bufferSize : fileSize;
    
    Serial.print("DEBUG: RP2040Storage::read - read size: ");
    Serial.println(readSize);
    
    size_t actualBytesRead = fread(buffer, 1, readSize, file);
    fclose(file);
    
    Serial.print("DEBUG: RP2040Storage::read - actual bytes read: ");
    Serial.println(actualBytesRead);
    
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
        Serial.print("DEBUG: RP2040Storage::write - failed to open file: ");
        Serial.println(filename);
        return StorageResult::ERROR_WRITE_FAILED;
    }
    
    Serial.print("DEBUG: RP2040Storage::write - writing ");
    Serial.print(dataSize);
    Serial.print(" bytes to ");
    Serial.println(filename);
    
    size_t bytesWritten = fwrite(data, 1, dataSize, file);
    
    Serial.print("DEBUG: RP2040Storage::write - fwrite returned ");
    Serial.println(bytesWritten);
    
    fflush(file);  // Explicit flush
    fclose(file);
    
    // Force filesystem sync
    #if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
        LittleFS.end();
        if (!LittleFS.begin()) {
            Serial.println("DEBUG: RP2040Storage::write - filesystem restart failed");
        }
        VFS.root(LittleFS);
    #endif
    
    Serial.print("DEBUG: RP2040Storage::write - file closed, result: ");
    Serial.println((bytesWritten == dataSize) ? "SUCCESS" : "FAILED");
    
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
    
#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    // Get actual filesystem info from built-in LittleFS
    FSInfo fsInfo;
    if (LittleFS.info(fsInfo)) {
        return fsInfo.totalBytes - fsInfo.usedBytes;
    }
#endif
    
    // Fallback estimate
    return 1024 * 1024; // 1MB estimate
}

size_t RP2040Storage::getUsedSpace() const {
    if (!m_initialized) {
        return 0;
    }
    
#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    // Get actual filesystem info from built-in LittleFS
    FSInfo fsInfo;
    if (LittleFS.info(fsInfo)) {
        return fsInfo.usedBytes;
    }
#endif
    
    return 0; // Fallback
}

StorageResult RP2040Storage::format() {
#if CONFIG_FEATURE_STORAGE_ENABLED && CONFIG_STORAGE_USE_LITTLEFS
    // Format the built-in LittleFS filesystem
    m_initialized = false;
    littleFS_initialized = false;
    
    // End current filesystem
    LittleFS.end();
    
    // Format and reinitialize
    if (!LittleFS.format()) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    // Reinitialize after format
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