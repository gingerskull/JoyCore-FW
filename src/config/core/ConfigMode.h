#pragma once

// Unified configuration system
// Legacy CONFIG_MODE system fully removed: device always operates from stored configuration (config.bin).
// On firmware version change a fresh config.bin is generated from ConfigDigital.h & ConfigAxis.h.
// This header retained for feature flags & version tracking only.

// Feature flags for configuration capabilities
#define CONFIG_FEATURE_USB_PROTOCOL_ENABLED 0  // USB HID protocol disabled (using serial instead)
#define CONFIG_FEATURE_STORAGE_ENABLED      1  // Storage system always enabled
#define CONFIG_FEATURE_VALIDATION_ENABLED   1  // Enable configuration validation

// Storage configuration
#define CONFIG_STORAGE_USE_LITTLEFS         0  // Use EEPROM instead of LittleFS
#define CONFIG_STORAGE_FILENAME            "/config.bin"
#define CONFIG_STORAGE_BACKUP_FILENAME     "/config_backup.bin"
#define CONFIG_STORAGE_FIRMWARE_VERSION    "/fw_version.txt"  // Firmware version tracking file
#define CONFIG_VERSION                     7   // Configuration format version

// Firmware version tracking (semantic versioning MAJOR.MINOR.PATCH)
// Bump according to semantic versioning rules: MAJOR (breaking), MINOR (features), PATCH (bug fixes)
#define FIRMWARE_VERSION_STRING            "0.1.0"

// (CONFIG_MODE macros removed; any old references should be deleted. Update external tools accordingly.)