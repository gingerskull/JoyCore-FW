/*
 * JoyCore-FW: RP2040 USB Game Controller
 *
 * Overview of supported input systems (enable/configure in ConfigDigital.h and ConfigAxis.h):
 * - Direct pin buttons (pins 4–13 in current config; INPUT_PULLUP, LOW = pressed)
 * - 74HC165 shift-register inputs (active-low; current config uses 2 chained parts = 16 inputs)
 * - Matrix button scanning (available; disabled until BTN_ROW/BTN_COL and INPUT_MATRIX are configured)
 * - Rotary encoders (from direct pins, matrix pins, or shift-register bits; ENC_A then ENC_B ordering)
 * - Analog axes with deadband, filtering (EWMA/adaptive), curves, and unified HID scaling
 *
 * USB HID notes:
 * - rp2040-HID provides a descriptor with up to 128 buttons and 16 axes. This firmware configures 32 buttons
 *   and up to 8 axes via the Joystick_ wrapper; unused descriptor fields are simply not updated.
 *
 * Runtime order and timing:
 * - Shift registers are read periodically into a shared buffer (shiftRegBuffer) and never on-demand by encoders
 *   to avoid contention. Throttling depends on SHIFTREG_COUNT.
 * - Update order: buttons -> matrix -> encoders -> axes -> send HID. Encoders depend on current matrix/shift-reg
 *   states, so this order ensures consistent behavior.
 */

#include <Arduino.h>
#ifdef USE_TINYUSB
#include "Adafruit_TinyUSB.h"
#endif
#include "Config.h"
#define DEFINE_MYJOYSTICK
#include "rp2040/JoystickWrapper.h"
#include "inputs/buttons/ButtonInput.h"
#include "inputs/encoders/EncoderInput.h"
#include "inputs/buttons/MatrixInput.h"
#include "inputs/shift_register/ShiftRegister165.h"
#include "config/ConfigAxis.h"
#include "config/core/ConfigManager.h"
#include "config/core/DeviceIdentifier.h"

#if CONFIG_FEATURE_STORAGE_ENABLED
    #include "rp2040/storage/RP2040EEPROMStorage.h"
#endif

// HID configuration protocol removed - using serial communication instead
// #if CONFIG_FEATURE_USB_PROTOCOL_ENABLED
//     #include "rp2040/hid/ConfigProtocol.h"
// #endif


 // USB joystick configuration: exposes full capabilities via TinyUSB
 // TinyUSBGamepad supports 128 buttons, 16 axes (hat switches temporarily disabled)
Joystick_ MyJoystick(0x03, 0x04, 128, 0, true, true, false, false, false, false, false, false);

// External shift register components
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void setup() {
    // Initialize configuration manager
    g_configManager.initialize();
   
    // Set USB descriptor from configuration
    const StoredUSBDescriptor* usbDesc = g_configManager.getUSBDescriptor();
    MyJoystick.setUSBDescriptor(usbDesc->vendorID, usbDesc->productID, 
                               usbDesc->manufacturer, usbDesc->product);
    
    // Initialize USB joystick interface EARLY for HID functionality
    MyJoystick.begin();
 
    // Disable all hat switches to prevent phantom inputs
    MyJoystick.setHatSwitch(0, -1);
    MyJoystick.setHatSwitch(1, -1);
    MyJoystick.setHatSwitch(2, -1);
    MyJoystick.setHatSwitch(3, -1);
    
    // Initialize configuration protocol
#if CONFIG_FEATURE_USB_PROTOCOL_ENABLED
    g_configProtocol.initialize();
#endif
    
    // Initialize all input subsystems using configuration from ConfigManager
    const LogicalInput* configInputs = g_configManager.getLogicalInputs();
    uint8_t configInputCount = g_configManager.getLogicalInputCount();
    
    initButtonsFromLogical(configInputs, configInputCount);
    initEncodersFromLogical(configInputs, configInputCount);
    initMatrixFromLogical(configInputs, configInputCount);

    // Initialize axis system
    setupUserAxes(MyJoystick);
    
    // Delay for USB enumeration
    delay(500);
    
    // Debug: Enable serial communication for testing
    Serial.begin(115200);
    Serial.println("JoyCore Configuration System Ready");
    
    // Debug: Print configuration status
    ConfigStatus status = g_configManager.getStatus();
    Serial.print("Config Mode: ");
    Serial.print(status.currentMode == 0 ? "STATIC" : status.currentMode == 1 ? "STORAGE" : "HYBRID");
    Serial.print(", Loaded: ");
    Serial.print(status.configLoaded ? "YES" : "NO");
    Serial.print(", Using Defaults: ");
    Serial.println(status.usingDefaults ? "YES" : "NO");
}

void loop() {
    // Check for serial commands (for debugging/testing)
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Handle IDENTIFY command for device discovery
        if (command == "IDENTIFY" || command == JoyCore::IDENTIFY_COMMAND) {
            // Send fixed device identifier response
            char response[128];
            JoyCore::formatIdentifyResponse(response, sizeof(response));
            Serial.println(response);
        }
        else if (command == "STATUS") {
            ConfigStatus status = g_configManager.getStatus();
            Serial.print("Config Status - Storage: ");
            Serial.print(status.storageInitialized ? "OK" : "FAIL");
            Serial.print(", Loaded: ");
            Serial.print(status.configLoaded ? "YES" : "NO");
            Serial.print(", Mode: ");
            Serial.print(status.currentMode);
            Serial.print(", Version: ");
            Serial.println(status.configVersion);
        }
        else if (command.startsWith("READ_FILE ")) {
            // READ_FILE <filename>
            String filename = command.substring(10);
            filename.trim();
            
            #if CONFIG_FEATURE_STORAGE_ENABLED
                uint8_t buffer[1024];
                size_t bytesRead = 0;
                StorageResult result = g_configManager.readFile(filename.c_str(), buffer, sizeof(buffer), &bytesRead);
                
                if (result == StorageResult::SUCCESS) {
                    Serial.print("FILE_DATA:");
                    Serial.print(filename);
                    Serial.print(":");
                    Serial.print(bytesRead);
                    Serial.print(":");
                    
                    // Send as base64 to handle binary data safely
                    for (size_t i = 0; i < bytesRead; i++) {
                        if (buffer[i] < 0x10) Serial.print("0");
                        Serial.print(buffer[i], HEX);
                    }
                    Serial.println();
                } else if (result == StorageResult::ERROR_FILE_NOT_FOUND) {
                    Serial.print("ERROR:FILE_NOT_FOUND:");
                    Serial.println(filename);
                } else {
                    Serial.print("ERROR:READ_FAILED:");
                    Serial.println(filename);
                }
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
        else if (command == "LIST_FILES") {
            // List all files in storage
            #if CONFIG_FEATURE_STORAGE_ENABLED
                char fileNames[8][32];
                uint8_t fileCount = g_configManager.listStorageFiles(fileNames, 8);
                
                Serial.println("FILES:");
                for (uint8_t i = 0; i < fileCount; i++) {
                    Serial.println(fileNames[i]);
                }
                Serial.println("END_FILES");
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
        else if (command == "STORAGE_INFO") {
            // Get storage information
            #if CONFIG_FEATURE_STORAGE_ENABLED
                Serial.print("STORAGE_USED:");
                Serial.println(g_configManager.getStorageUsed());
                Serial.print("STORAGE_AVAILABLE:");
                Serial.println(g_configManager.getStorageAvailable());
                Serial.print("STORAGE_INITIALIZED:");
                Serial.println(g_configManager.isStorageInitialized() ? "YES" : "NO");
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
        else if (command == "INIT_STORAGE") {
            // Force storage initialization
            #if CONFIG_FEATURE_STORAGE_ENABLED
                Serial.println("Attempting to initialize storage...");
                // Try to initialize storage directly
                RP2040EEPROMStorage storage;
                StorageResult result = storage.initialize();
                Serial.print("Storage init result: ");
                Serial.println((int)result);
                
                if (result == StorageResult::SUCCESS) {
                    Serial.println("Storage initialized successfully");
                    Serial.print("Used space: ");
                    Serial.println(storage.getUsedSpace());
                    Serial.print("Available space: ");
                    Serial.println(storage.getAvailableSpace());
                } else {
                    Serial.println("Storage initialization failed");
                }
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
        else if (command == "FORMAT_STORAGE") {
            // Format the storage filesystem
            #if CONFIG_FEATURE_STORAGE_ENABLED
                Serial.println("Formatting storage filesystem...");
                RP2040EEPROMStorage storage;
                StorageResult result = storage.format();
                Serial.print("Format result: ");
                Serial.println((int)result);
                
                if (result == StorageResult::SUCCESS) {
                    Serial.println("Storage formatted successfully");
                    // Try to initialize after format
                    result = storage.initialize();
                    if (result == StorageResult::SUCCESS) {
                        Serial.println("Storage re-initialized successfully");
                        Serial.print("Available space: ");
                        Serial.println(storage.getAvailableSpace());
                    }
                } else {
                    Serial.println("Storage format failed");
                }
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
        else if (command == "TEST_WRITE") {
            // Simple test write
            Serial.println("Testing simple file write...");
            const char* testData = "Hello World!";
            g_configManager.writeFile("/test.txt", (const uint8_t*)testData, strlen(testData));
            Serial.println("Test write completed");
        }
        else if (command == "FORCE_DEFAULT_CONFIG") {
            // Force create default configuration (simulates firmware version change)
            Serial.println("Forcing default configuration creation...");
            
            // Force the manager to create defaults and save
            g_configManager.resetToDefaults();
            
            Serial.println("Default configuration created and saved");
        }
        else if (command == "SAVE_CONFIG") {
            // Force save current configuration to storage
            Serial.println("Saving current configuration to storage...");
            
            // Debug: Print current configuration counts
            Serial.print("Debug - Pin map count: ");
            Serial.println(g_configManager.getPinMapCount());
            Serial.print("Debug - Logical input count: ");
            Serial.println(g_configManager.getLogicalInputCount());
            Serial.print("Debug - Shift reg count: ");
            Serial.println(g_configManager.getShiftRegisterCount());
            
            bool result = g_configManager.saveConfiguration();
            if (result) {
                Serial.println("Configuration saved successfully");
            } else {
                Serial.println("Configuration save failed");
            }
        }
        else if (command == "DEBUG_STORAGE") {
            // Debug storage system
            #if CONFIG_FEATURE_STORAGE_ENABLED
                Serial.println("Debugging storage system...");
                g_configManager.debugStorage();
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
        else if (command == "CREATE_TEST_FILES") {
            // Create test files to populate storage
            #if CONFIG_FEATURE_STORAGE_ENABLED
                Serial.println("Creating test files...");
                
                // Test firmware version file
                const char* versionData = "13";
                StorageResult result = g_configManager.writeFile("/fw_version.txt", (const uint8_t*)versionData, strlen(versionData));
                Serial.print("Writing /fw_version.txt: ");
                Serial.println(result == StorageResult::SUCCESS ? "SUCCESS" : "FAILED");
                
                // Force save current configuration
                Serial.println("Saving current configuration...");
                bool saveResult = g_configManager.saveConfiguration();
                Serial.print("Save configuration result: ");
                Serial.println(saveResult ? "SUCCESS" : "FAILED");
                
                // Debug storage after writes
                g_configManager.debugStorage();
            #else
                Serial.println("ERROR:STORAGE_NOT_ENABLED");
            #endif
        }
    }
    static uint32_t lastShiftRegRead = 0;
    uint32_t currentTime = millis();
    
    // Read shift-register chain into the shared buffer at a controlled rate.
    // Encoders consume this buffer but do NOT trigger reads themselves to avoid conflicts.
    if (shiftReg && shiftRegBuffer) {
        // Read every 5 ms for 2+ registers, every 1 ms for a single register (stability versus latency)
        uint32_t readInterval = (SHIFTREG_COUNT > 1) ? 5 : 1;
        if (currentTime - lastShiftRegRead >= readInterval) {
            shiftReg->read(shiftRegBuffer);
            lastShiftRegRead = currentTime;
        }
    }
    
    // Update in dependency-safe order:
    updateButtons();   // Direct pins and shift-register buttons (uses shiftRegBuffer)
    updateMatrix();    // Matrix scanning (updates matrix state used by encoders)
    updateEncoders();  // Rotary encoders (uses current matrix/shift-register states)
    
    // Read axis values
    readUserAxes(MyJoystick);
    
    // Send all HID updates to computer
    MyJoystick.sendState();
}
