#include "SerialCommands.h"
#include "../config/core/ConfigManager.h"
#include "../config/core/DeviceIdentifier.h"
#include "../utils/Debug.h"
#include "../Config.h"
#if CONFIG_FEATURE_STORAGE_ENABLED
#include "../rp2040/storage/RP2040EEPROMStorage.h"
#endif

// Command handler signature
using CommandHandler = void (*)(const String&);
struct SerialCommand { const char* name; CommandHandler handler; };

static void cmdIdentify(const String&) {
    char response[128];
    JoyCore::formatIdentifyResponse(response, sizeof(response));
    Serial.println(response);
}
static void cmdStatus(const String&) {
    ConfigStatus status = g_configManager.getStatus();
    Serial.print("Config Status - Storage: "); Serial.print(status.storageInitialized ? "OK" : "FAIL");
    Serial.print(", Loaded: "); Serial.print(status.configLoaded ? "YES" : "NO");
    Serial.print(", Mode: "); Serial.print(status.currentMode);
    Serial.print(", Version: "); Serial.println(status.configVersion);
}
static void cmdForceDefaults(const String&) {
    Serial.println("Forcing default configuration creation...");
    g_configManager.resetToDefaults();
    Serial.println("Default configuration created and saved");
}
static void cmdSaveConfig(const String&) {
    Serial.println("Saving current configuration to storage...");
    bool result = g_configManager.saveConfiguration();
    Serial.println(result?"Configuration saved successfully":"Configuration save failed");
}
static void cmdTestWrite(const String&) {
    const char* testData = "Hello World!";
    g_configManager.writeFile("/test.txt", (const uint8_t*)testData, strlen(testData));
    Serial.println("Test write completed");
}
static void cmdCreateTestFiles(const String&) {
#if CONFIG_FEATURE_STORAGE_ENABLED
    Serial.println("Creating test files...");
    const char* versionData = "13";
    StorageResult result = g_configManager.writeFile("/fw_version.txt", (const uint8_t*)versionData, strlen(versionData));
    Serial.print("Writing /fw_version.txt: "); Serial.println(result == StorageResult::SUCCESS ? "SUCCESS" : "FAILED");
    bool saveResult = g_configManager.saveConfiguration();
    Serial.print("Save configuration result: "); Serial.println(saveResult?"SUCCESS":"FAILED");
    g_configManager.debugStorage();
#else
    Serial.println("ERROR:STORAGE_NOT_ENABLED");
#endif
}
#if CONFIG_FEATURE_STORAGE_ENABLED
static void cmdListFiles(const String&) {
    char fileNames[8][32];
    uint8_t fileCount = g_configManager.listStorageFiles(fileNames, 8);
    Serial.println("FILES:");
    for (uint8_t i = 0; i < fileCount; i++) Serial.println(fileNames[i]);
    Serial.println("END_FILES");
}
static void cmdStorageInfo(const String&) {
    Serial.print("STORAGE_USED:"); Serial.println(g_configManager.getStorageUsed());
    Serial.print("STORAGE_AVAILABLE:"); Serial.println(g_configManager.getStorageAvailable());
    Serial.print("STORAGE_INITIALIZED:"); Serial.println(g_configManager.isStorageInitialized()?"YES":"NO");
}
static void cmdReadFile(const String& args) {
    String f = String(args); f.trim();
    if(f.length()==0){ Serial.println("ERROR:NO_FILENAME"); return; }
    uint8_t buffer[1024]; size_t bytesRead=0; auto res = g_configManager.readFile(f.c_str(), buffer, sizeof(buffer), &bytesRead);
    if(res==StorageResult::SUCCESS) {
        Serial.print("FILE_DATA:"); Serial.print(f); Serial.print(":"); Serial.print(bytesRead); Serial.print(":");
        for(size_t i=0;i<bytesRead;i++){ if(buffer[i]<0x10) Serial.print('0'); Serial.print(buffer[i], HEX);} Serial.println();
    } else if(res==StorageResult::ERROR_FILE_NOT_FOUND) {
        Serial.print("ERROR:FILE_NOT_FOUND:"); Serial.println(f);
    } else {
        Serial.print("ERROR:READ_FAILED:"); Serial.println(f);
    }
}
#endif

static const SerialCommand kCommands[] = {
    {"IDENTIFY", cmdIdentify},
    {JoyCore::IDENTIFY_COMMAND, cmdIdentify},
    {"STATUS", cmdStatus},
    {"FORCE_DEFAULT_CONFIG", cmdForceDefaults},
    {"SAVE_CONFIG", cmdSaveConfig},
    {"TEST_WRITE", cmdTestWrite},
    {"CREATE_TEST_FILES", cmdCreateTestFiles},
#if CONFIG_FEATURE_STORAGE_ENABLED
    {"LIST_FILES", cmdListFiles},
    {"STORAGE_INFO", cmdStorageInfo},
    {"READ_FILE", cmdReadFile},
    {"INIT_STORAGE", [](const String&){ RP2040EEPROMStorage storage; auto r=storage.initialize(); Serial.print("Storage init result: "); Serial.println((int)r); }},
    {"FORMAT_STORAGE", [](const String&){ RP2040EEPROMStorage storage; auto r=storage.format(); Serial.print("Format result: "); Serial.println((int)r); if(r==StorageResult::SUCCESS){ storage.initialize(); Serial.print("Available space: "); Serial.println(storage.getAvailableSpace()); } }},
#endif
};
static constexpr size_t kCommandCount = sizeof(kCommands)/sizeof(kCommands[0]);

void processSerialLine(String line) {
    line.trim();
    int spaceIdx = line.indexOf(' ');
    String cmd = (spaceIdx>=0)? line.substring(0, spaceIdx): line;
    String args = (spaceIdx>=0)? line.substring(spaceIdx+1): String();
    for(size_t i=0;i<kCommandCount;i++) {
        if(cmd.equalsIgnoreCase(kCommands[i].name)) { kCommands[i].handler(args); return; }
    }
    Serial.println("ERROR:UNKNOWN_COMMAND");
}
