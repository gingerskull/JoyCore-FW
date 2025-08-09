#pragma once

// Configuration mode switch - determines how configuration data is loaded
// This allows gradual transition from compile-time config to storage-based config

// Configuration modes:
// - CONFIG_MODE_STATIC: Use compile-time configuration from ConfigDigital.h and ConfigAxis.h (current behavior)
// - CONFIG_MODE_STORAGE: Load configuration from EEPROM/flash storage (future mode)
// - CONFIG_MODE_HYBRID: Load from storage, fallback to static if storage is invalid/empty

#define CONFIG_MODE_STATIC  0
#define CONFIG_MODE_STORAGE 1
#define CONFIG_MODE_HYBRID  2

// Current configuration mode - change this to switch between modes
// Start with STATIC for backwards compatibility, change to HYBRID when storage system is ready
#ifndef CONFIG_MODE
    #define CONFIG_MODE CONFIG_MODE_STORAGE
#endif

// Feature flags for configuration capabilities
#define CONFIG_FEATURE_USB_PROTOCOL_ENABLED 0  // USB HID protocol disabled (using serial instead)
#define CONFIG_FEATURE_STORAGE_ENABLED      1  // Enable storage system
#define CONFIG_FEATURE_VALIDATION_ENABLED   1  // Enable configuration validation

// Storage configuration
#define CONFIG_STORAGE_USE_LITTLEFS         0  // Use EEPROM instead of LittleFS
#define CONFIG_STORAGE_FILENAME            "/config.bin"
#define CONFIG_STORAGE_BACKUP_FILENAME     "/config_backup.bin"
#define CONFIG_STORAGE_FIRMWARE_VERSION    "/fw_version.txt"  // Firmware version tracking file
#define CONFIG_VERSION                     3   // Configuration format version

// USB protocol configuration (deprecated - using serial instead)
// #define CONFIG_USB_FEATURE_REPORT_ID       0x02  // Feature report ID for configuration
// #define CONFIG_USB_MAX_PACKET_SIZE         64    // Maximum size for config data packets

// Firmware version tracking
// Increment this value when making changes that should trigger config save on upload
#define FIRMWARE_VERSION                   26     // Increment on each build that needs fresh config