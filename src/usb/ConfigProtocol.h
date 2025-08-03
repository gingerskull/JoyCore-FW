#pragma once

#include "../ConfigMode.h"
#include "../config/ConfigStructs.h"
#include "../ConfigManager.h"

#if CONFIG_FEATURE_USB_PROTOCOL_ENABLED

// USB Configuration Protocol Handler
// Extends the existing rp2040-HID infrastructure to support configuration communication
// Uses HID Feature Reports for bidirectional configuration data exchange

class ConfigProtocol {
public:
    ConfigProtocol();
    ~ConfigProtocol();
    
    // Initialize the USB configuration protocol
    bool initialize();
    
    // Process incoming configuration messages
    bool processFeatureReport(uint8_t reportID, const uint8_t* data, uint16_t length);
    
    // Generate feature report response
    bool generateFeatureReport(uint8_t reportID, uint8_t* data, uint16_t maxLength, uint16_t* actualLength);
    
    // Check if there's a pending response
    bool hasPendingResponse() const { return m_pendingResponse; }
    
    // Get the response report ID
    uint8_t getResponseReportID() const { return m_responseReportID; }
    
private:
    // Message handlers
    bool handleGetConfig(const ConfigMessage* request);
    bool handleSetConfig(const ConfigMessage* request);
    bool handleResetConfig(const ConfigMessage* request);
    bool handleValidateConfig(const ConfigMessage* request);
    bool handleGetStatus(const ConfigMessage* request);
    bool handleSaveConfig(const ConfigMessage* request);
    bool handleLoadConfig(const ConfigMessage* request);
    
    // Multi-packet transfer support
    bool handleMultiPacketTransfer(const ConfigMessage* request);
    bool sendMultiPacketResponse(const uint8_t* data, size_t dataSize, ConfigMessageType responseType);
    
    // Response generation
    bool sendResponse(ConfigMessageType type, uint8_t status, const uint8_t* data = nullptr, size_t dataSize = 0);
    bool sendErrorResponse(ConfigMessageType type, uint8_t errorCode, const char* errorMessage = nullptr);
    
    // Validation helpers
    bool validateRequest(const ConfigMessage* request, uint16_t length) const;
    bool isValidMessageType(ConfigMessageType type) const;
    
    // State management
    bool m_initialized;
    bool m_pendingResponse;
    uint8_t m_responseReportID;
    ConfigMessage m_responseMessage;
    
    // Multi-packet transfer state
    struct TransferState {
        bool active;
        uint8_t expectedSequence;
        uint8_t totalPackets;
        ConfigMessageType messageType;
        uint8_t buffer[2048];  // Buffer for large transfers
        size_t bufferUsed;
        size_t bufferSize;
    } m_transferState;
    
    // Statistics
    uint32_t m_messagesReceived;
    uint32_t m_messagesProcessed;
    uint32_t m_errors;
};

// Global protocol handler instance
extern ConfigProtocol g_configProtocol;

#endif // CONFIG_FEATURE_USB_PROTOCOL_ENABLED