#include "ConfigProtocol.h"
#include "TinyUSBGamepad.h"
#include <string.h>

#if CONFIG_FEATURE_USB_PROTOCOL_ENABLED

// Global instance
ConfigProtocol g_configProtocol;

ConfigProtocol::ConfigProtocol() 
    : m_initialized(false)
    , m_pendingResponse(false)
    , m_responseReportID(0)
    , m_messagesReceived(0)
    , m_messagesProcessed(0)
    , m_errors(0)
{
    memset(&m_responseMessage, 0, sizeof(m_responseMessage));
    memset(&m_transferState, 0, sizeof(m_transferState));
}

ConfigProtocol::~ConfigProtocol() {
}

bool ConfigProtocol::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Reset state
    m_pendingResponse = false;
    m_responseReportID = 0;
    memset(&m_transferState, 0, sizeof(m_transferState));
    
    // Set up TinyUSB feature report callbacks
    TinyUSBGamepad::setFeatureReportCallback(
        // GET_REPORT callback
        [](uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) -> uint16_t {
            uint16_t actualLength = 0;
            g_configProtocol.generateFeatureReport(report_id, buffer, reqlen, &actualLength);
            return actualLength;
        },
        // SET_REPORT callback
        [](uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize) {
            g_configProtocol.processFeatureReport(report_id, buffer, bufsize);
        }
    );
    
    m_initialized = true;
    return true;
}

bool ConfigProtocol::processFeatureReport(uint8_t reportID, const uint8_t* data, uint16_t length) {
    if (!m_initialized || !data || length < sizeof(ConfigMessage)) {
        m_errors++;
        return false;
    }
    
    // Only process configuration feature reports
    if (reportID != CONFIG_USB_FEATURE_REPORT_ID) {
        return false; // Not a config message, let other handlers process it
    }
    
    m_messagesReceived++;
    
    const ConfigMessage* request = reinterpret_cast<const ConfigMessage*>(data);
    
    // Validate request
    if (!validateRequest(request, length)) {
        m_errors++;
        return sendErrorResponse(request->type, 0x01, "Invalid request format");
    }
    
    // Handle multi-packet transfers
    if (request->totalPackets > 1) {
        return handleMultiPacketTransfer(request);
    }
    
    // Process single-packet message
    bool result = false;
    
    switch (request->type) {
        case ConfigMessageType::GET_CONFIG:
            result = handleGetConfig(request);
            break;
            
        case ConfigMessageType::SET_CONFIG:
            result = handleSetConfig(request);
            break;
            
        case ConfigMessageType::RESET_CONFIG:
            result = handleResetConfig(request);
            break;
            
        case ConfigMessageType::VALIDATE_CONFIG:
            result = handleValidateConfig(request);
            break;
            
        case ConfigMessageType::GET_CONFIG_STATUS:
            result = handleGetStatus(request);
            break;
            
        case ConfigMessageType::SAVE_CONFIG:
            result = handleSaveConfig(request);
            break;
            
        case ConfigMessageType::LOAD_CONFIG:
            result = handleLoadConfig(request);
            break;
            
        default:
            result = sendErrorResponse(request->type, 0x02, "Unknown message type");
            break;
    }
    
    if (result) {
        m_messagesProcessed++;
    } else {
        m_errors++;
    }
    
    return result;
}

bool ConfigProtocol::generateFeatureReport(uint8_t reportID, uint8_t* data, uint16_t maxLength, uint16_t* actualLength) {
    if (!m_initialized || !data || !actualLength) {
        return false;
    }
    
    // Only handle configuration feature reports
    if (reportID != CONFIG_USB_FEATURE_REPORT_ID) {
        return false;
    }
    
    // If no pending response, generate a default status response
    if (!m_pendingResponse) {
        // Generate a basic status response for GET requests
        ConfigMessage statusMessage;
        memset(&statusMessage, 0, sizeof(statusMessage));
        statusMessage.reportID = reportID;
        statusMessage.type = ConfigMessageType::GET_CONFIG_STATUS;
        statusMessage.sequence = 0;
        statusMessage.totalPackets = 1;
        statusMessage.dataLength = sizeof(ConfigStatus);
        statusMessage.status = 0; // Success
        
        // Add config status data
        ConfigStatus status = g_configManager.getStatus();
        memcpy(statusMessage.data, &status, sizeof(status));
        
        memcpy(&m_responseMessage, &statusMessage, sizeof(statusMessage));
    }
    
    // Copy response message
    size_t responseSize = sizeof(ConfigMessage);
    if (responseSize > maxLength) {
        return false;
    }
    
    memcpy(data, &m_responseMessage, responseSize);
    *actualLength = responseSize;
    
    // Clear pending response
    m_pendingResponse = false;
    m_responseReportID = 0;
    
    return true;
}

bool ConfigProtocol::handleGetConfig(const ConfigMessage* request) {
    uint8_t buffer[2048];
    size_t configSize;
    
    if (!g_configManager.getSerializedConfig(buffer, sizeof(buffer), &configSize)) {
        return sendErrorResponse(ConfigMessageType::GET_CONFIG, 0x10, "Failed to serialize config");
    }
    
    // Send config data (may require multiple packets)
    return sendMultiPacketResponse(buffer, configSize, ConfigMessageType::GET_CONFIG);
}

bool ConfigProtocol::handleSetConfig(const ConfigMessage* request) {
    // For single packet, apply configuration directly
    if (request->totalPackets == 1) {
        const StoredConfig* config = reinterpret_cast<const StoredConfig*>(request->data);
        const uint8_t* variableData = request->data + sizeof(StoredConfig);
        size_t variableSize = request->dataLength - sizeof(StoredConfig);
        
        if (!g_configManager.applyConfiguration(config, variableData, variableSize)) {
            return sendErrorResponse(ConfigMessageType::SET_CONFIG, 0x20, "Config validation failed");
        }
        
        return sendResponse(ConfigMessageType::SET_CONFIG, 0x00); // Success
    }
    
    // Multi-packet transfers are handled by handleMultiPacketTransfer
    return false;
}

bool ConfigProtocol::handleResetConfig(const ConfigMessage* request) {
    if (!g_configManager.resetToDefaults()) {
        return sendErrorResponse(ConfigMessageType::RESET_CONFIG, 0x30, "Reset failed");
    }
    
    return sendResponse(ConfigMessageType::RESET_CONFIG, 0x00);
}

bool ConfigProtocol::handleValidateConfig(const ConfigMessage* request) {
    const StoredConfig* config = reinterpret_cast<const StoredConfig*>(request->data);
    ConfigValidationResult validation = g_configManager.validateConfiguration(config);
    
    return sendResponse(ConfigMessageType::VALIDATE_CONFIG, 
                       validation.isValid ? 0x00 : 0x01,
                       reinterpret_cast<const uint8_t*>(&validation), 
                       sizeof(validation));
}

bool ConfigProtocol::handleGetStatus(const ConfigMessage* request) {
    ConfigStatus status = g_configManager.getStatus();
    
    return sendResponse(ConfigMessageType::GET_CONFIG_STATUS, 0x00,
                       reinterpret_cast<const uint8_t*>(&status),
                       sizeof(status));
}

bool ConfigProtocol::handleSaveConfig(const ConfigMessage* request) {
    if (!g_configManager.saveConfiguration()) {
        return sendErrorResponse(ConfigMessageType::SAVE_CONFIG, 0x40, "Save failed");
    }
    
    return sendResponse(ConfigMessageType::SAVE_CONFIG, 0x00);
}

bool ConfigProtocol::handleLoadConfig(const ConfigMessage* request) {
    if (!g_configManager.loadConfiguration()) {
        return sendErrorResponse(ConfigMessageType::LOAD_CONFIG, 0x50, "Load failed");
    }
    
    return sendResponse(ConfigMessageType::LOAD_CONFIG, 0x00);
}

bool ConfigProtocol::handleMultiPacketTransfer(const ConfigMessage* request) {
    // Initialize transfer on first packet
    if (request->sequence == 0) {
        m_transferState.active = true;
        m_transferState.expectedSequence = 0;
        m_transferState.totalPackets = request->totalPackets;
        m_transferState.messageType = request->type;
        m_transferState.bufferUsed = 0;
        m_transferState.bufferSize = sizeof(m_transferState.buffer);
    }
    
    // Validate sequence number
    if (!m_transferState.active || request->sequence != m_transferState.expectedSequence) {
        m_transferState.active = false;
        return sendErrorResponse(request->type, 0x60, "Invalid sequence");
    }
    
    // Append data to buffer
    if (m_transferState.bufferUsed + request->dataLength > m_transferState.bufferSize) {
        m_transferState.active = false;
        return sendErrorResponse(request->type, 0x61, "Transfer too large");
    }
    
    memcpy(m_transferState.buffer + m_transferState.bufferUsed, request->data, request->dataLength);
    m_transferState.bufferUsed += request->dataLength;
    m_transferState.expectedSequence++;
    
    // Process complete transfer
    if (m_transferState.expectedSequence >= m_transferState.totalPackets) {
        m_transferState.active = false;
        
        // Handle the complete message based on type
        if (m_transferState.messageType == ConfigMessageType::SET_CONFIG) {
            const StoredConfig* config = reinterpret_cast<const StoredConfig*>(m_transferState.buffer);
            const uint8_t* variableData = m_transferState.buffer + sizeof(StoredConfig);
            size_t variableSize = m_transferState.bufferUsed - sizeof(StoredConfig);
            
            if (!g_configManager.applyConfiguration(config, variableData, variableSize)) {
                return sendErrorResponse(ConfigMessageType::SET_CONFIG, 0x20, "Config validation failed");
            }
            
            return sendResponse(ConfigMessageType::SET_CONFIG, 0x00);
        }
        
        return sendErrorResponse(m_transferState.messageType, 0x62, "Unsupported multi-packet type");
    }
    
    // Send acknowledgment for partial transfer
    return sendResponse(request->type, 0x00);
}

bool ConfigProtocol::sendMultiPacketResponse(const uint8_t* data, size_t dataSize, ConfigMessageType responseType) {
    // For now, only support single-packet responses
    // Multi-packet responses would require more complex state management
    
    if (dataSize <= CONFIG_USB_MAX_PACKET_SIZE - 8) {
        return sendResponse(responseType, 0x00, data, dataSize);
    }
    
    return sendErrorResponse(responseType, 0x70, "Response too large");
}

bool ConfigProtocol::sendResponse(ConfigMessageType type, uint8_t status, const uint8_t* data, size_t dataSize) {
    if (m_pendingResponse) {
        return false; // Previous response not yet sent
    }
    
    memset(&m_responseMessage, 0, sizeof(m_responseMessage));
    
    m_responseMessage.reportID = CONFIG_USB_FEATURE_REPORT_ID;
    m_responseMessage.type = type;
    m_responseMessage.sequence = 0;
    m_responseMessage.totalPackets = 1;
    m_responseMessage.status = status;
    
    if (data && dataSize > 0) {
        size_t copySize = min(dataSize, sizeof(m_responseMessage.data));
        memcpy(m_responseMessage.data, data, copySize);
        m_responseMessage.dataLength = copySize;
    } else {
        m_responseMessage.dataLength = 0;
    }
    
    m_pendingResponse = true;
    m_responseReportID = CONFIG_USB_FEATURE_REPORT_ID;
    
    return true;
}

bool ConfigProtocol::sendErrorResponse(ConfigMessageType type, uint8_t errorCode, const char* errorMessage) {
    uint8_t errorData[64];
    size_t errorDataSize = 0;
    
    if (errorMessage) {
        errorDataSize = min(strlen(errorMessage), sizeof(errorData) - 1);
        memcpy(errorData, errorMessage, errorDataSize);
        errorData[errorDataSize] = 0; // Null terminate
        errorDataSize++; // Include null terminator
    }
    
    return sendResponse(type, errorCode, errorData, errorDataSize);
}

bool ConfigProtocol::validateRequest(const ConfigMessage* request, uint16_t length) const {
    if (!request) {
        return false;
    }
    
    // Check minimum size
    if (length < sizeof(ConfigMessage) - sizeof(request->data)) {
        return false;
    }
    
    // Check report ID
    if (request->reportID != CONFIG_USB_FEATURE_REPORT_ID) {
        return false;
    }
    
    // Check message type
    if (!isValidMessageType(request->type)) {
        return false;
    }
    
    // Check data length consistency
    size_t expectedSize = sizeof(ConfigMessage) - sizeof(request->data) + request->dataLength;
    if (length < expectedSize) {
        return false;
    }
    
    return true;
}

bool ConfigProtocol::isValidMessageType(ConfigMessageType type) const {
    switch (type) {
        case ConfigMessageType::GET_CONFIG:
        case ConfigMessageType::SET_CONFIG:
        case ConfigMessageType::RESET_CONFIG:
        case ConfigMessageType::VALIDATE_CONFIG:
        case ConfigMessageType::GET_CONFIG_STATUS:
        case ConfigMessageType::SAVE_CONFIG:
        case ConfigMessageType::LOAD_CONFIG:
            return true;
        default:
            return false;
    }
}

#endif // CONFIG_FEATURE_USB_PROTOCOL_ENABLED