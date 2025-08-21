// SPDX-License-Identifier: GPL-3.0-or-later
#include "RawStateReader.h"
#include "../inputs/buttons/MatrixInput.h"
#include "../inputs/ShiftRegisterManager.h"
#include "../Config.h"
#include <hardware/gpio.h>

// Static member definitions
bool RawStateReader::s_rawMonitoringEnabled = false;
uint32_t RawStateReader::s_lastMonitorUpdate = 0;

// External references to input subsystems
extern ShiftRegisterManager g_shiftRegisterManager;

uint64_t RawStateReader::getCurrentTimestamp() {
    return to_us_since_boot(get_absolute_time());
}

void RawStateReader::readGpioStates() {
    uint32_t gpio_mask = 0;
    
    // Read all 30 GPIO pins (GPIO 0-29)
    for (int i = 0; i < 30; i++) {
        bool pin_state = gpio_get(i);
        if (pin_state) {
            gpio_mask |= (1UL << i);
        }
    }
    
    uint64_t timestamp = getCurrentTimestamp();
    Serial.print("GPIO_STATES:0x");
    Serial.print(gpio_mask, HEX);
    Serial.print(":");
    Serial.println((unsigned long)timestamp);
}

void RawStateReader::readMatrixState() {
    uint64_t timestamp = getCurrentTimestamp();
    
    // Get matrix dimensions
    uint8_t rows = getMatrixRows();
    uint8_t cols = getMatrixCols();
    
    if (rows == 0 || cols == 0) {
        Serial.println("MATRIX_STATE:NO_MATRIX_CONFIGURED");
        return;
    }
    
    // Access matrix pin configuration through accessor functions
    uint8_t* rowPins = MatrixRawAccess::getRowPins();
    uint8_t* colPins = MatrixRawAccess::getColPins();
    
    if (!rowPins || !colPins) {
        Serial.println("MATRIX_STATE:NO_MATRIX_PINS_CONFIGURED");
        return;
    }
    
    // Scan matrix
    for (uint8_t row = 0; row < rows; row++) {
        
        // Drive this row LOW (active)
        gpio_set_dir(rowPins[row], GPIO_OUT);
        gpio_put(rowPins[row], 0);
        
        // Small delay for signal to settle
        delayMicroseconds(10);
        
        // Read all columns
        for (uint8_t col = 0; col < cols; col++) {
            
            // Read column state (with pull-up, LOW means pressed)
            bool is_connected = !gpio_get(colPins[col]);
            
            // Send state for this intersection
            Serial.print("MATRIX_STATE:");
            Serial.print(row);
            Serial.print(":");
            Serial.print(col);
            Serial.print(":");
            Serial.print(is_connected ? 1 : 0);
            Serial.print(":");
            Serial.println((unsigned long)timestamp);
        }
        
        // Return row to high-impedance state
        gpio_set_dir(rowPins[row], GPIO_IN);
    }
}

void RawStateReader::readShiftRegState() {
    uint64_t timestamp = getCurrentTimestamp();
    
    // Access shift register buffer
    uint8_t* buffer = g_shiftRegisterManager.getBuffer();
    if (!buffer) {
        Serial.println("SHIFT_REG:NO_SHIFT_REG_CONFIGURED");
        return;
    }
    
    // Read from current buffer (non-intrusive)
    for (uint8_t reg = 0; reg < SHIFTREG_COUNT; reg++) {
        Serial.print("SHIFT_REG:");
        Serial.print(reg);
        Serial.print(":0x");
        if (buffer[reg] < 0x10) Serial.print('0');
        Serial.print(buffer[reg], HEX);
        Serial.print(":");
        Serial.println((unsigned long)timestamp);
    }
}

void RawStateReader::startRawMonitor() {
    s_rawMonitoringEnabled = true;
    s_lastMonitorUpdate = millis();
    Serial.println("OK:RAW_MONITOR_STARTED");
}

void RawStateReader::stopRawMonitor() {
    s_rawMonitoringEnabled = false;
    Serial.println("OK:RAW_MONITOR_STOPPED");
}

void RawStateReader::updateRawMonitoring() {
    if (!s_rawMonitoringEnabled) return;
    
    uint32_t now = millis();
    if (now - s_lastMonitorUpdate >= MONITOR_INTERVAL_MS) {
        s_lastMonitorUpdate = now;
        sendAllStates();
    }
}

void RawStateReader::sendAllStates() {
    readGpioStates();
    readMatrixState();
    readShiftRegState();
}