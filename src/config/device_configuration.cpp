#include "config/device_configuration.h"
#include "config/version_manager.h"
#include "../diagnostics/logger.h"
#include <WiFi.h>
#include <Preferences.h>

// Global instance
DeviceConfiguration& g_device_config = DeviceConfiguration::getInstance();

// Configuration keys for NVS storage
static const char* CONFIG_NAMESPACE = "device_config";
static const char* KEY_DEVICE_ID = "device_id";
static const char* KEY_CONNECTION_STATE = "conn_state";
static const char* KEY_OPERATION_MODE = "op_mode";

DeviceConfiguration::DeviceConfiguration() 
    : connection_state_(ConnectionState::DISCONNECTED)
    , led_state_(LEDState::OFF)
    , operation_mode_(OperationMode::NORMAL)
    , device_id_("")
    , firmware_version_("")
    , boot_time_(0)
    , last_state_change_(0)
    , configuration_dirty_(false)
    , setup_mode_(false)
    , connection_retry_count_(0) {
}

DeviceConfiguration& DeviceConfiguration::getInstance() {
    static DeviceConfiguration instance;
    return instance;
}

bool DeviceConfiguration::begin() {
    boot_time_ = millis();
    
    // Initialize device identification
    initializeDeviceId();
    initializeFirmwareVersion();
    
    // Load saved configuration
    if (!loadConfiguration()) {
        Diagnostics::Logger::warning("Device Configuration: Could not load saved configuration, using defaults");
    }
    
    Diagnostics::Logger::info("Device Configuration: Initialized - ID: " + device_id_);
    Diagnostics::Logger::info("Device Configuration: Firmware Version: " + firmware_version_);
    
    return true;
}

DeviceConfiguration::ConnectionState DeviceConfiguration::getConnectionState() const {
    return connection_state_;
}

void DeviceConfiguration::setConnectionState(ConnectionState state) {
    if (connection_state_ != state) {
        ConnectionState previous_state = connection_state_;
        connection_state_ = state;
        last_state_change_ = millis();
        configuration_dirty_ = true;
        
        Diagnostics::Logger::info("Device Configuration: Connection state changed from " + 
                    String(static_cast<int>(previous_state)) + " to " + 
                    String(static_cast<int>(state)));
        
        // Update LED state based on connection state
        updateLEDState();
    }
}

DeviceConfiguration::LEDState DeviceConfiguration::getLEDState() const {
    return led_state_;
}

void DeviceConfiguration::setLEDState(LEDState state) {
    if (led_state_ != state) {
        led_state_ = state;
        Diagnostics::Logger::debug_log("Device Configuration: LED state changed to " + String(static_cast<int>(state)));
    }
}

DeviceConfiguration::OperationMode DeviceConfiguration::getOperationMode() const {
    return operation_mode_;
}

void DeviceConfiguration::setOperationMode(OperationMode mode) {
    if (operation_mode_ != mode) {
        OperationMode previous_mode = operation_mode_;
        operation_mode_ = mode;
        configuration_dirty_ = true;
        
        Diagnostics::Logger::info("Device Configuration: Operation mode changed from " + 
                    String(static_cast<int>(previous_mode)) + " to " + 
                    String(static_cast<int>(mode)));
    }
}

String DeviceConfiguration::getDeviceId() const {
    return device_id_;
}

String DeviceConfiguration::getFirmwareVersion() const {
    return firmware_version_;
}

bool DeviceConfiguration::isWiFiConnected() const {
    return connection_state_ == ConnectionState::CONNECTED;
}

unsigned long DeviceConfiguration::getUptime() const {
    return millis() - boot_time_;
}

void DeviceConfiguration::update() {
    // Auto-save configuration if dirty and enough time has passed
    static unsigned long last_save_attempt = 0;
    const unsigned long save_interval = 30000; // Save every 30 seconds if dirty
    
    if (configuration_dirty_ && 
        (millis() - last_save_attempt > save_interval)) {
        
        if (saveConfiguration()) {
            configuration_dirty_ = false;
            Diagnostics::Logger::debug_log("Device Configuration: Auto-saved configuration");
        }
        last_save_attempt = millis();
    }
}

bool DeviceConfiguration::saveConfiguration() {
    Preferences prefs;
    if (!prefs.begin(CONFIG_NAMESPACE, false)) {
        Diagnostics::Logger::error("Device Configuration: Failed to open preferences for save");
        return false;
    }
    
    bool success = true;
    
    // Save device ID
    if (prefs.putString(KEY_DEVICE_ID, device_id_) != device_id_.length()) {
        Diagnostics::Logger::error("Device Configuration: Failed to save device ID");
        success = false;
    }
    
    // Save connection state
    if (!prefs.putUChar(KEY_CONNECTION_STATE, static_cast<uint8_t>(connection_state_))) {
        Diagnostics::Logger::error("Device Configuration: Failed to save connection state");
        success = false;
    }
    
    // Save operation mode
    if (!prefs.putUChar(KEY_OPERATION_MODE, static_cast<uint8_t>(operation_mode_))) {
        Diagnostics::Logger::error("Device Configuration: Failed to save operation mode");
        success = false;
    }
    
    prefs.end();
    
    if (success) {
        configuration_dirty_ = false;
        Diagnostics::Logger::debug_log("Device Configuration: Configuration saved successfully");
    }
    
    return success;
}

bool DeviceConfiguration::loadConfiguration() {
    Preferences prefs;
    if (!prefs.begin(CONFIG_NAMESPACE, true)) { // Read-only mode
        Diagnostics::Logger::warning("Device Configuration: Failed to open preferences for load");
        return false;
    }
    
    // Load device ID (use generated one if not found)
    String saved_device_id = prefs.getString(KEY_DEVICE_ID, "");
    if (saved_device_id.length() > 0) {
        device_id_ = saved_device_id;
    }
    
    // Load connection state
    uint8_t saved_connection_state = prefs.getUChar(KEY_CONNECTION_STATE, 
                                                   static_cast<uint8_t>(ConnectionState::DISCONNECTED));
    connection_state_ = static_cast<ConnectionState>(saved_connection_state);
    
    // Load operation mode
    uint8_t saved_operation_mode = prefs.getUChar(KEY_OPERATION_MODE, 
                                                 static_cast<uint8_t>(OperationMode::NORMAL));
    operation_mode_ = static_cast<OperationMode>(saved_operation_mode);
    
    prefs.end();
    
    // Update LED state based on loaded connection state
    updateLEDState();
    
    Diagnostics::Logger::info("Device Configuration: Configuration loaded successfully");
    return true;
}

bool DeviceConfiguration::resetToDefaults() {
    Diagnostics::Logger::info("Device Configuration: Resetting to factory defaults");
    
    // Clear saved preferences
    Preferences prefs;
    if (prefs.begin(CONFIG_NAMESPACE, false)) {
        prefs.clear();
        prefs.end();
    }
    
    // Reset state to defaults
    connection_state_ = ConnectionState::DISCONNECTED;
    led_state_ = LEDState::OFF;
    operation_mode_ = OperationMode::NORMAL;
    
    // Regenerate device ID
    initializeDeviceId();
    
    configuration_dirty_ = true;
    
    Diagnostics::Logger::info("Device Configuration: Reset to defaults complete");
    return true;
}

String DeviceConfiguration::generateDeviceId() {
    // Generate device ID from MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    char device_id[16];
    snprintf(device_id, sizeof(device_id), "GS-%02X%02X%02X", 
             mac[3], mac[4], mac[5]);
    
    return String(device_id);
}

void DeviceConfiguration::initializeDeviceId() {
    if (device_id_.length() == 0) {
        device_id_ = generateDeviceId();
        configuration_dirty_ = true;
    }
}

void DeviceConfiguration::initializeFirmwareVersion() {
    // Get version from version manager
    Config::VersionManager version_mgr;
    firmware_version_ = version_mgr.getCurrentVersion();
    
    // Fallback to compile-time version if version manager not available
    if (firmware_version_.length() == 0) {
        #ifdef FIRMWARE_VERSION
        firmware_version_ = FIRMWARE_VERSION;
        #else
        firmware_version_ = "1.0.0-dev";
        #endif
    }
}

void DeviceConfiguration::updateLEDState() {
    // Update LED state based on connection state
    switch (connection_state_) {
        case ConnectionState::DISCONNECTED:
            setLEDState(LEDState::OFF);
            break;
            
        case ConnectionState::CONNECTING:
            setLEDState(LEDState::CONNECTING);
            break;
            
        case ConnectionState::CONNECTED:
            setLEDState(LEDState::CONNECTED);
            break;
            
        case ConnectionState::CONNECTION_FAILED:
            setLEDState(LEDState::ERROR);
            break;
            
        case ConnectionState::SETUP_MODE:
            setLEDState(LEDState::SETUP_MODE);
            break;
            
        default:
            setLEDState(LEDState::OFF);
            break;
    }
}

void DeviceConfiguration::setDeviceId(const String& device_id) {
    if (device_id_ != device_id) {
        device_id_ = device_id;
        configuration_dirty_ = true;
        last_state_change_ = millis();
    }
}

bool DeviceConfiguration::isInSetupMode() const {
    return setup_mode_;
}

void DeviceConfiguration::setSetupMode(bool setup_mode) {
    if (setup_mode_ != setup_mode) {
        setup_mode_ = setup_mode;
        configuration_dirty_ = true;
        last_state_change_ = millis();
        Diagnostics::Logger::info("Device Configuration: Setup mode changed to " + String(setup_mode ? "true" : "false"));
    }
}

int DeviceConfiguration::getConnectionRetryCount() const {
    return connection_retry_count_;
}

void DeviceConfiguration::setConnectionRetryCount(int count) {
    if (connection_retry_count_ != count) {
        connection_retry_count_ = count;
        configuration_dirty_ = true;
        last_state_change_ = millis();
        Diagnostics::Logger::debug_log("Device Configuration: Connection retry count set to " + String(count));
    }
}