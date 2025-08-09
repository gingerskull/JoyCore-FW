#pragma once

#include "../../StorageInterface.h"
#include <stdint.h>

#if CONFIG_FEATURE_STORAGE_ENABLED
    #include <EEPROM.h>
#endif

// RP2040-specific EEPROM storage implementation
// Uses EEPROM emulation for direct byte-level configuration storage
// More reliable and efficient than LittleFS for small configuration data
class RP2040EEPROMStorage : public StorageInterface {
public:
    RP2040EEPROMStorage();
    virtual ~RP2040EEPROMStorage();
    
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
    
    // List files in storage
    uint8_t listFiles(char fileNames[][32], uint8_t maxFiles) override;
    
    // Debug method to dump file table
    void debugDumpFileTable();
    
private:
    // EEPROM Memory Layout
    static constexpr uint16_t EEPROM_SIZE = 4096;  // 4KB EEPROM
    static constexpr uint16_t FILE_TABLE_START = 0x0000;
    static constexpr uint16_t FILE_TABLE_SIZE = 64;  // 64 bytes for file table
    static constexpr uint16_t DATA_START = FILE_TABLE_SIZE;
    static constexpr uint16_t DATA_SIZE = EEPROM_SIZE - FILE_TABLE_SIZE;
    
    // File entry structure (8 bytes each, supports up to 8 files)
    struct FileEntry {
        char name[4];           // Short filename (4 chars max)
        uint16_t offset;        // Data offset from DATA_START
        uint16_t size;          // File size in bytes
    } __attribute__((packed));
    
    static constexpr uint8_t MAX_FILES = 8;
    static constexpr uint8_t FILE_ENTRY_SIZE = sizeof(FileEntry);
    
    // Well-known file offsets for main config files
    static constexpr uint16_t CONFIG_FILE_OFFSET = 0;
    static constexpr uint16_t BACKUP_FILE_OFFSET = 2048;
    static constexpr uint16_t VERSION_FILE_OFFSET = 3584;
    
    // Helper methods
    StorageResult initializeEEPROM();
    int findFile(const char* filename);
    int findFreeSlot();
    bool createFileEntry(const char* filename, uint16_t size);
    bool updateFileEntry(int index, uint16_t size);
    void removeFileEntry(int index);
    
    // Direct EEPROM access helpers
    void readEEPROM(uint16_t address, uint8_t* buffer, size_t size);
    void writeEEPROM(uint16_t address, const uint8_t* data, size_t size);
    
    // File table management
    void loadFileTable();
    void saveFileTable();
    bool validateFileTable();
    
    FileEntry m_fileTable[MAX_FILES];
    uint8_t m_fileCount;
    bool m_tableLoaded;
    
    // Convert long filenames to 4-char keys
    void filenameToKey(const char* filename, char* key);
};