// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

/**
 * @brief Raw hardware state reader for configuration and debugging
 * 
 * Provides non-intrusive access to raw pin states, matrix scanning results,
 * and shift register data without interfering with normal input processing.
 */
class RawStateReader {
public:
    /**
     * @brief Read all GPIO pin states as a bitmask
     * Reads GPIO pins 0-29 and returns state as 32-bit hex with timestamp
     * Format: GPIO_STATES:0x[32-bit-hex]:[timestamp]
     */
    static void readGpioStates();
    
    /**
     * @brief Read matrix button states by scanning
     * Temporarily scans matrix without affecting normal operation
     * Format: MATRIX_STATE:[row]:[col]:[0/1]:[timestamp]
     */
    static void readMatrixState();
    
    /**
     * @brief Read shift register buffer states
     * Returns current buffered shift register data
     * Format: SHIFT_REG:[reg_id]:[8-bit-hex]:[timestamp]
     */
    static void readShiftRegState();
    
    /**
     * @brief Start continuous raw state monitoring
     * Enables periodic transmission of all raw states
     */
    static void startRawMonitor();
    
    /**
     * @brief Stop continuous raw state monitoring
     */
    static void stopRawMonitor();
    
    /**
     * @brief Update raw state monitoring (call from main loop)
     * Sends periodic updates when monitoring is enabled
     */
    static void updateRawMonitoring();

private:
    static bool s_rawMonitoringEnabled;
    static uint32_t s_lastMonitorUpdate;
    static constexpr uint32_t MONITOR_INTERVAL_MS = 50;
    
    // Helper functions
    static uint64_t getCurrentTimestamp();
    static void sendAllStates();
};