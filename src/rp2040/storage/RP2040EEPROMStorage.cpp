#include "RP2040EEPROMStorage.h"
#include "../../config/core/ConfigMode.h"
#include <string.h>
#include <stdio.h>
#include <Arduino.h>

#if CONFIG_FEATURE_STORAGE_ENABLED
    #include <EEPROM.h>
#endif

RP2040EEPROMStorage::RP2040EEPROMStorage() : StorageInterface() {
    m_fileCount = 0;
    m_tableLoaded = false;
    memset(m_fileTable, 0, sizeof(m_fileTable));
}

RP2040EEPROMStorage::~RP2040EEPROMStorage() {
}

StorageResult RP2040EEPROMStorage::initialize() {
    if (m_initialized) {
        return StorageResult::SUCCESS;
    }
    
    return initializeEEPROM();
}

StorageResult RP2040EEPROMStorage::initializeEEPROM() {
#if CONFIG_FEATURE_STORAGE_ENABLED
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Load and validate file table
    loadFileTable();
    if (!validateFileTable()) {
        // File table corrupted or uninitialized, format EEPROM
        format();
    }
    
    m_initialized = true;
    return StorageResult::SUCCESS;
#else
    return StorageResult::ERROR_NOT_INITIALIZED;
#endif
}

void RP2040EEPROMStorage::loadFileTable() {
#if CONFIG_FEATURE_STORAGE_ENABLED
    readEEPROM(FILE_TABLE_START, (uint8_t*)m_fileTable, sizeof(m_fileTable));
    
    // Count valid files
    m_fileCount = 0;
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        if (m_fileTable[i].name[0] != 0 && m_fileTable[i].name[0] != 0xFF) {
            m_fileCount++;
        }
    }
    m_tableLoaded = true;
#endif
}

void RP2040EEPROMStorage::saveFileTable() {
#if CONFIG_FEATURE_STORAGE_ENABLED
    writeEEPROM(FILE_TABLE_START, (const uint8_t*)m_fileTable, sizeof(m_fileTable));
    EEPROM.commit();
#endif
}

bool RP2040EEPROMStorage::validateFileTable() {
    // Check for overlapping files or invalid offsets
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        if (m_fileTable[i].name[0] == 0 || m_fileTable[i].name[0] == 0xFF) {
            continue; // Empty slot
        }
        
        // Check if offset + size exceeds available space
        if (m_fileTable[i].offset + m_fileTable[i].size > DATA_SIZE) {
            return false;
        }
        
        // Check for overlaps with other files
        for (uint8_t j = i + 1; j < MAX_FILES; j++) {
            if (m_fileTable[j].name[0] == 0 || m_fileTable[j].name[0] == 0xFF) {
                continue;
            }
            
            uint16_t i_start = m_fileTable[i].offset;
            uint16_t i_end = i_start + m_fileTable[i].size;
            uint16_t j_start = m_fileTable[j].offset;
            uint16_t j_end = j_start + m_fileTable[j].size;
            
            // Check for overlap
            if ((i_start < j_end) && (j_start < i_end)) {
                return false;
            }
        }
    }
    return true;
}

StorageResult RP2040EEPROMStorage::read(const char* filename, uint8_t* buffer, size_t bufferSize, size_t* bytesRead) {
    if (!m_initialized) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    if (!filename || !buffer || bufferSize == 0) {
        return StorageResult::ERROR_INVALID_PARAMETER;
    }
    
    int fileIndex = findFile(filename);
    if (fileIndex < 0) {
        return StorageResult::ERROR_FILE_NOT_FOUND;
    }
    
    const FileEntry& file = m_fileTable[fileIndex];
    size_t readSize = (bufferSize < file.size) ? bufferSize : file.size;
    
    readEEPROM(DATA_START + file.offset, buffer, readSize);
    
    if (bytesRead) {
        *bytesRead = readSize;
    }
    
    return StorageResult::SUCCESS;
}

StorageResult RP2040EEPROMStorage::write(const char* filename, const uint8_t* data, size_t dataSize) {
    if (!m_initialized) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    if (!filename || !data || dataSize == 0) {
        return StorageResult::ERROR_INVALID_PARAMETER;
    }
    
    if (dataSize > DATA_SIZE) {
        return StorageResult::ERROR_INSUFFICIENT_SPACE;
    }
    
    int fileIndex = findFile(filename);
    if (fileIndex >= 0) {
        // File exists, check if we can reuse the space
        const FileEntry& existingFile = m_fileTable[fileIndex];
        if (existingFile.size >= dataSize) {
            // Reuse existing space
            writeEEPROM(DATA_START + existingFile.offset, data, dataSize);
            updateFileEntry(fileIndex, dataSize);
            saveFileTable();
            EEPROM.commit();
            return StorageResult::SUCCESS;
        } else {
            // Need more space, remove old file and create new one
            removeFileEntry(fileIndex);
        }
    }
    
    // Create new file entry
    if (!createFileEntry(filename, dataSize)) {
        return StorageResult::ERROR_INSUFFICIENT_SPACE;
    }
    
    // Find the file we just created
    fileIndex = findFile(filename);
    if (fileIndex < 0) {
        return StorageResult::ERROR_WRITE_FAILED;
    }
    
    // Write data
    const FileEntry& file = m_fileTable[fileIndex];
    writeEEPROM(DATA_START + file.offset, data, dataSize);
    saveFileTable();
    EEPROM.commit();
    
    return StorageResult::SUCCESS;
}

bool RP2040EEPROMStorage::exists(const char* filename) {
    if (!m_initialized || !filename) {
        return false;
    }
    
    return findFile(filename) >= 0;
}

StorageResult RP2040EEPROMStorage::remove(const char* filename) {
    if (!m_initialized) {
        return StorageResult::ERROR_NOT_INITIALIZED;
    }
    
    if (!filename) {
        return StorageResult::ERROR_INVALID_PARAMETER;
    }
    
    int fileIndex = findFile(filename);
    if (fileIndex < 0) {
        return StorageResult::ERROR_FILE_NOT_FOUND;
    }
    
    removeFileEntry(fileIndex);
    saveFileTable();
    return StorageResult::SUCCESS;
}

size_t RP2040EEPROMStorage::getAvailableSpace() const {
    if (!m_initialized) {
        return 0;
    }
    
    size_t used = getUsedSpace();
    return (DATA_SIZE > used) ? (DATA_SIZE - used) : 0;
}

size_t RP2040EEPROMStorage::getUsedSpace() const {
    if (!m_initialized) {
        return 0;
    }
    
    size_t total = 0;
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        if (m_fileTable[i].name[0] != 0 && m_fileTable[i].name[0] != 0xFF) {
            total += m_fileTable[i].size;
        }
    }
    return total;
}

StorageResult RP2040EEPROMStorage::format() {
#if CONFIG_FEATURE_STORAGE_ENABLED
    // Clear entire EEPROM
    for (uint16_t addr = 0; addr < EEPROM_SIZE; addr++) {
        EEPROM.write(addr, 0xFF);
    }
    
    // Initialize empty file table
    memset(m_fileTable, 0, sizeof(m_fileTable));
    m_fileCount = 0;
    
    // Save empty file table
    saveFileTable();
    m_tableLoaded = true;
    
    return StorageResult::SUCCESS;
#else
    return StorageResult::ERROR_NOT_INITIALIZED;
#endif
}

StorageResult RP2040EEPROMStorage::maintenance() {
    // EEPROM doesn't need maintenance like wear leveling
    // But we can validate file table integrity
    if (!validateFileTable()) {
        // Attempt to recover by reloading table
        loadFileTable();
        if (!validateFileTable()) {
            return StorageResult::ERROR_CORRUPTION_DETECTED;
        }
    }
    return StorageResult::SUCCESS;
}

int RP2040EEPROMStorage::findFile(const char* filename) {
    if (!m_tableLoaded) {
        loadFileTable();
    }
    
    char key[5];
    filenameToKey(filename, key);
    
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        if (memcmp(m_fileTable[i].name, key, 4) == 0) {
            return i;
        }
    }
    return -1;
}

int RP2040EEPROMStorage::findFreeSlot() {
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        if (m_fileTable[i].name[0] == 0 || m_fileTable[i].name[0] == 0xFF) {
            return i;
        }
    }
    return -1;
}

bool RP2040EEPROMStorage::createFileEntry(const char* filename, uint16_t size) {
    int slot = findFreeSlot();
    if (slot < 0) {
        return false; // No free slots
    }
    
    // Find free space for the file data
    uint16_t offset = 0;
    bool found = false;
    
    // Simple first-fit allocation
    for (offset = 0; offset <= DATA_SIZE - size; offset++) {
        bool conflict = false;
        
        // Check if this offset conflicts with any existing file
        for (uint8_t i = 0; i < MAX_FILES; i++) {
            if (m_fileTable[i].name[0] == 0 || m_fileTable[i].name[0] == 0xFF) {
                continue;
            }
            
            uint16_t file_start = m_fileTable[i].offset;
            uint16_t file_end = file_start + m_fileTable[i].size;
            uint16_t new_end = offset + size;
            
            if ((offset < file_end) && (file_start < new_end)) {
                conflict = true;
                offset = file_end - 1; // Jump past this file
                break;
            }
        }
        
        if (!conflict) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        return false; // No space available
    }
    
    // Create file entry
    filenameToKey(filename, m_fileTable[slot].name);
    m_fileTable[slot].offset = offset;
    m_fileTable[slot].size = size;
    m_fileCount++;
    
    return true;
}

bool RP2040EEPROMStorage::updateFileEntry(int index, uint16_t size) {
    if (index < 0 || index >= MAX_FILES) {
        return false;
    }
    
    m_fileTable[index].size = size;
    return true;
}

void RP2040EEPROMStorage::removeFileEntry(int index) {
    if (index < 0 || index >= MAX_FILES) {
        return;
    }
    
    memset(&m_fileTable[index], 0, sizeof(FileEntry));
    m_fileCount--;
}

void RP2040EEPROMStorage::readEEPROM(uint16_t address, uint8_t* buffer, size_t size) {
#if CONFIG_FEATURE_STORAGE_ENABLED
    for (size_t i = 0; i < size; i++) {
        buffer[i] = EEPROM.read(address + i);
    }
#endif
}

void RP2040EEPROMStorage::writeEEPROM(uint16_t address, const uint8_t* data, size_t size) {
#if CONFIG_FEATURE_STORAGE_ENABLED
    for (size_t i = 0; i < size; i++) {
        EEPROM.write(address + i, data[i]);
    }
#endif
}

void RP2040EEPROMStorage::filenameToKey(const char* filename, char* key) {
    // Convert filename to 4-character key
    memset(key, 0, 4);
    
    if (strcmp(filename, CONFIG_STORAGE_FILENAME) == 0) {
        memcpy(key, "CFG", 3);
    } else if (strcmp(filename, CONFIG_STORAGE_BACKUP_FILENAME) == 0) {
        memcpy(key, "BAK", 3);
    } else if (strcmp(filename, CONFIG_STORAGE_FIRMWARE_VERSION) == 0) {
        memcpy(key, "VER", 3);
    } else {
        // Generic conversion - take first 4 chars or hash
        size_t len = strlen(filename);
        if (len >= 4) {
            memcpy(key, filename, 4);
        } else {
            strncpy(key, filename, len);
        }
    }
}

uint8_t RP2040EEPROMStorage::listFiles(char fileNames[][32], uint8_t maxFiles) {
    if (!m_initialized || !fileNames || maxFiles == 0) {
        return 0;
    }
    
    if (!m_tableLoaded) {
        loadFileTable();
    }
    
    uint8_t count = 0;
    
    // Iterate through file table and build list of actual filenames
    for (uint8_t i = 0; i < MAX_FILES && count < maxFiles; i++) {
        if (m_fileTable[i].name[0] != 0 && m_fileTable[i].name[0] != 0xFF) {
            // Convert the 3-character key back to filename
            if (memcmp(m_fileTable[i].name, "CFG", 3) == 0) {
                strncpy(fileNames[count], CONFIG_STORAGE_FILENAME, 31);
                fileNames[count][31] = '\0';
                count++;
            } else if (memcmp(m_fileTable[i].name, "BAK", 3) == 0) {
                strncpy(fileNames[count], CONFIG_STORAGE_BACKUP_FILENAME, 31);
                fileNames[count][31] = '\0';
                count++;
            } else if (memcmp(m_fileTable[i].name, "VER", 3) == 0) {
                strncpy(fileNames[count], CONFIG_STORAGE_FIRMWARE_VERSION, 31);
                fileNames[count][31] = '\0';
                count++;
            } else {
                // Generic filename (use the key as the name)
                snprintf(fileNames[count], 32, "/%c%c%c%c",
                         m_fileTable[i].name[0],
                         m_fileTable[i].name[1],
                         m_fileTable[i].name[2],
                         m_fileTable[i].name[3]);
                count++;
            }
        }
    }
    
    return count;
}

void RP2040EEPROMStorage::debugDumpFileTable() {
#if CONFIG_FEATURE_STORAGE_ENABLED
    Serial.println("\n=== FILE TABLE DEBUG DUMP ===");
    Serial.print("Table loaded: ");
    Serial.println(m_tableLoaded ? "YES" : "NO");
    Serial.print("File count: ");
    Serial.println(m_fileCount);
    Serial.print("Initialized: ");
    Serial.println(m_initialized ? "YES" : "NO");
    
    if (!m_tableLoaded) {
        loadFileTable();
    }
    
    Serial.println("\nFile Table Entries:");
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] ");
        
        if (m_fileTable[i].name[0] == 0 || m_fileTable[i].name[0] == 0xFF) {
            Serial.println("(empty)");
        } else {
            Serial.print("Key: '");
            for (int j = 0; j < 4; j++) {
                if (m_fileTable[i].name[j] >= 32 && m_fileTable[i].name[j] <= 126) {
                    Serial.print((char)m_fileTable[i].name[j]);
                } else {
                    Serial.print("?");
                }
            }
            Serial.print("' (");
            for (int j = 0; j < 4; j++) {
                if (m_fileTable[i].name[j] < 0x10) Serial.print("0");
                Serial.print(m_fileTable[i].name[j], HEX);
                if (j < 3) Serial.print(" ");
            }
            Serial.print(") -> ");
            
            // Show which filename this maps to
            if (memcmp(m_fileTable[i].name, "CFG", 3) == 0) {
                Serial.print(CONFIG_STORAGE_FILENAME);
            } else if (memcmp(m_fileTable[i].name, "BAK", 3) == 0) {
                Serial.print(CONFIG_STORAGE_BACKUP_FILENAME);
            } else if (memcmp(m_fileTable[i].name, "VER", 3) == 0) {
                Serial.print(CONFIG_STORAGE_FIRMWARE_VERSION);
            } else {
                Serial.print("(unknown)");
            }
            
            Serial.print(", Offset: ");
            Serial.print(m_fileTable[i].offset);
            Serial.print(", Size: ");
            Serial.println(m_fileTable[i].size);
        }
    }
    
    Serial.print("\nTotal used space: ");
    Serial.print(getUsedSpace());
    Serial.print(" / ");
    Serial.print(DATA_SIZE);
    Serial.println(" bytes");
    Serial.println("=== END FILE TABLE DEBUG ===\n");
#endif
}