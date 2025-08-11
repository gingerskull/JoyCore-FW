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
 * Runtime order and timing now centralized by InputManager (shift -> buttons -> matrix -> encoders -> axes -> HID).
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
#include "inputs/ShiftRegisterManager.h"
#include "inputs/InputManager.h"
#include "config/ConfigAxis.h"
#include "config/core/ConfigManager.h"
#include "config/core/DeviceIdentifier.h"
#include "utils/Debug.h"
#include "comm/SerialCommands.h"

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
    // Initialize configuration manager (debug output will be lost but USB works)
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
    
    g_inputManager.begin(configInputs, configInputCount);

    // Initialize axis system
    setupUserAxes(MyJoystick);
    
    // Delay for USB enumeration BEFORE enabling Serial
    delay(500);
    
    // Initialize serial for debugging after USB is established
    Serial.begin(115200);
    Serial.println("JoyCore Configuration System Ready");
#if CONFIG_DEBUG
    ConfigStatus status = g_configManager.getStatus();
    Serial.print("Config Loaded: "); Serial.print(status.configLoaded ? "YES" : "NO");
    Serial.print(", Using Defaults: "); Serial.println(status.usingDefaults ? "YES" : "NO");
    // Dynamic allocation summary
    extern uint16_t getButtonPinGroupCount();
    extern uint16_t getShiftRegGroupCount();
    extern uint8_t getMatrixRows();
    extern uint8_t getMatrixCols();
    extern uint8_t getEncoderCount();
    Serial.print("Alloc Buttons(pinGroups/shiftGroups): ");
    Serial.print(getButtonPinGroupCount()); Serial.print("/"); Serial.println(getShiftRegGroupCount());
    Serial.print("Alloc Matrix(rows x cols): ");
    Serial.print(getMatrixRows()); Serial.print(" x "); Serial.println(getMatrixCols());
    Serial.print("Alloc Encoders: "); Serial.println(getEncoderCount());
#endif
}

void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        processSerialLine(line);
    }
    g_inputManager.update(MyJoystick);
}
