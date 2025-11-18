#include "wifi_connection_manager.h"
#include "../diagnostics/logger.h"
#include "wifi_setup_portal.h"
#include "config/device_configuration.h"
#include "diagnostics/logger.h"
#include "wifi/wifi_setup_portal.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

// Global instance
WiFiConnectionManager g_wifi_connection_manager;

// Constructor
WiFiConnectionManager::WiFiConnectionManager() 
    : credential_manager_(nullptr)
    , led_controller_(nullptr)
    , retry_count_(0)
    , last_retry_time_(0)
    , button_pressed_last_(false)
    , button_press_start_(0)
    , access_point_active_(false)
    , device_ap_ssid_("")
    , last_connection_state_(DeviceConfiguration::ConnectionState::DISCONNECTED)
    , state_change_time_(0) {
    
    // Initialize setup session
    setup_session_.session_id = "";
    setup_session_.start_time = 0;
    setup_session_.timeout_ms = SETUP_SESSION_TIMEOUT_MS;
    setup_session_.ip_address = "";
    setup_session_.active = false;
    setup_session_.credentials_submitted = false;
    setup_session_.validation_in_progress = false;
}

// Destructor
WiFiConnectionManager::~WiFiConnectionManager() {
    if (access_point_active_) {
        stopSetupAccessPoint();
    }
}

bool WiFiConnectionManager::begin(WiFiCredentialManager* credential_manager, 
                                 Actuators::StatusLEDController* led_controller) {
    if (!credential_manager || !led_controller) {
        Diagnostics::Logger::error("WiFi Connection Manager: Invalid parameters for begin()");
        return false;
    }
    
    credential_manager_ = credential_manager;
    led_controller_ = led_controller;
    
    // Initialize WiFi in station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    
    // Set initial connection state
    setConnectionState(DeviceConfiguration::ConnectionState::DISCONNECTED);
    resetRetryState();
    
    Diagnostics::Logger::info("WiFi Connection Manager: Initialized successfully");
    return true;
}

void WiFiConnectionManager::update() {
    updateConnectionStateMachine();
    updateLEDStatus();
    
    // Handle setup session timeouts
    if (setup_session_.active) {
        unsigned long elapsed = millis() - setup_session_.start_time;
        if (elapsed > setup_session_.timeout_ms) {
            handleSetupTimeout();
        }
    }
}

bool WiFiConnectionManager::connectToStoredNetwork() {
    if (!credential_manager_) {
        Diagnostics::Logger::error("WiFi Connection Manager: No credential manager available");
        return false;
    }
    
    // Check if credentials are available
    if (!credential_manager_->hasStoredCredentials()) {
        Diagnostics::Logger::info("WiFi Connection Manager: No stored credentials available");
        return enterSetupMode();
    }
    
    // Get stored credentials
    String ssid, password;
    if (!credential_manager_->getStoredCredentials(ssid, password)) {
        Diagnostics::Logger::error("WiFi Connection Manager: Failed to retrieve stored credentials");
        return false;
    }
    
    Diagnostics::Logger::info("WiFi Connection Manager: Attempting connection to stored network: " + ssid);
    
    // Set connection state and attempt connection
    setConnectionState(DeviceConfiguration::ConnectionState::CONNECTING);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    resetRetryState();
    
    return true;
}

bool WiFiConnectionManager::createSetupAccessPoint() {
    if (access_point_active_) {
        Diagnostics::Logger::warning("WiFi Connection Manager: Access point already active");
        return true;
    }
    
    // Generate unique AP SSID
    device_ap_ssid_ = generateAPSSID();
    
    // Set WiFi mode to Access Point + Station
    WiFi.mode(WIFI_AP_STA);
    
    // Create access point
    bool ap_result = WiFi.softAP(device_ap_ssid_.c_str(), nullptr, 1, false, 4);
    
    if (!ap_result) {
        Diagnostics::Logger::error("WiFi Connection Manager: Failed to create access point");
        return false;
    }
    
    access_point_active_ = true;
    
    // Get AP IP address
    IPAddress ap_ip = WiFi.softAPIP();
    setup_session_.ip_address = ap_ip.toString();
    
    Diagnostics::Logger::info("WiFi Connection Manager: Access point created - SSID: " + device_ap_ssid_);
    Diagnostics::Logger::info("WiFi Connection Manager: Access point IP: " + setup_session_.ip_address);
    
    return true;
}

void WiFiConnectionManager::stopSetupAccessPoint() {
    if (!access_point_active_) {
        return;
    }
    
    WiFi.softAPdisconnect(true);
    access_point_active_ = false;
    device_ap_ssid_ = "";
    
    // Return to station mode
    WiFi.mode(WIFI_STA);
    
    Diagnostics::Logger::info("WiFi Connection Manager: Access point stopped");
}

bool WiFiConnectionManager::validateCredentials(const String& ssid, const String& password) {
    Diagnostics::Logger::info("WiFi Connection Manager: Validating credentials for SSID: " + ssid);
    
    // T021a: Validate WPA2/WPA3 protocol support before attempting connection
    if (!validateWiFiSecurity(ssid)) {
        Diagnostics::Logger::error("WiFi Connection Manager: Network security protocol not supported for SSID: " + ssid);
        return false;
    }
    
    // Save current connection state
    DeviceConfiguration::ConnectionState original_state = getConnectionState();
    
    // Disconnect from current network if connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(1000); // Wait for disconnect
    }
    
    // Attempt connection with new credentials
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection attempt (with timeout)
    unsigned long start_time = millis();
    const unsigned long validation_timeout = 15000; // 15 seconds
    
    while (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED) {
        if (millis() - start_time > validation_timeout) {
            Diagnostics::Logger::warning("WiFi Connection Manager: Credential validation timed out");
            break;
        }
        delay(100);
        yield(); // Allow other tasks to run
        esp_task_wdt_reset(); // Reset watchdog timer
    }
    
    bool validation_successful = (WiFi.status() == WL_CONNECTED);
    
    if (validation_successful) {
        Diagnostics::Logger::info("WiFi Connection Manager: Credential validation successful");
        setConnectionState(DeviceConfiguration::ConnectionState::CONNECTED);
    } else {
        Diagnostics::Logger::error("WiFi Connection Manager: Credential validation failed");
        WiFi.disconnect();
        setConnectionState(original_state);
    }
    
    return validation_successful;
}

bool WiFiConnectionManager::enterSetupMode() {
    Diagnostics::Logger::info("WiFi Connection Manager: Entering setup mode");
    
    // Create access point
    if (!createSetupAccessPoint()) {
        return false;
    }
    
    // Initialize setup session
    setup_session_.session_id = generateSessionId();
    setup_session_.start_time = millis();
    setup_session_.active = true;
    setup_session_.credentials_submitted = false;
    setup_session_.validation_in_progress = false;
    
    // Update connection state
    setConnectionState(DeviceConfiguration::ConnectionState::SETUP_MODE);
    
    Diagnostics::Logger::info("WiFi Connection Manager: Setup mode activated with session ID: " + setup_session_.session_id);
    return true;
}

void WiFiConnectionManager::exitSetupMode() {
    if (!isInSetupMode()) {
        return;
    }
    
    Diagnostics::Logger::info("WiFi Connection Manager: Exiting setup mode");
    
    // Stop access point
    stopSetupAccessPoint();
    
    // Clear setup session
    setup_session_.active = false;
    setup_session_.session_id = "";
    setup_session_.ip_address = "";
    setup_session_.credentials_submitted = false;
    setup_session_.validation_in_progress = false;
    
    // Update connection state
    setConnectionState(DeviceConfiguration::ConnectionState::DISCONNECTED);
    
    // Attempt to connect to stored network
    connectToStoredNetwork();
}

bool WiFiConnectionManager::isInSetupMode() const {
    return setup_session_.active && access_point_active_;
}

DeviceConfiguration::ConnectionState WiFiConnectionManager::getConnectionState() const {
    return DeviceConfiguration::getInstance().getConnectionState();
}

void WiFiConnectionManager::handlePhysicalReset(bool button_pressed) {
    // Detect button press transition
    if (button_pressed && !button_pressed_last_) {
        button_press_start_ = millis();
        Diagnostics::Logger::debug_log("WiFi Connection Manager: Reset button pressed");
    }
    
    // Handle button release
    if (!button_pressed && button_pressed_last_) {
        unsigned long press_duration = millis() - button_press_start_;
        
        if (press_duration >= PHYSICAL_RESET_HOLD_TIME_MS) {
            Diagnostics::Logger::info("WiFi Connection Manager: Physical reset triggered");
            
            // Clear stored credentials
            if (credential_manager_) {
                credential_manager_->clearCredentials();
            }
            
            // Enter setup mode
            enterSetupMode();
        }
    }
    
    button_pressed_last_ = button_pressed;
}

bool WiFiConnectionManager::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiConnectionManager::getWiFiStatus(String& connected_ssid, int& signal_strength, String& ip_address) const {
    if (!isWiFiConnected()) {
        return false;
    }
    
    connected_ssid = WiFi.SSID();
    signal_strength = WiFi.RSSI();
    ip_address = WiFi.localIP().toString();
    
    return true;
}

void WiFiConnectionManager::disconnect() {
    Diagnostics::Logger::info("WiFi Connection Manager: Disconnecting from WiFi");
    
    WiFi.disconnect();
    setConnectionState(DeviceConfiguration::ConnectionState::DISCONNECTED);
    resetRetryState();
}

bool WiFiConnectionManager::validateWiFiSecurity(const String& ssid) {
    Diagnostics::Logger::debug_log("WiFi Connection Manager: Validating security for SSID: " + ssid);
    
    // Scan for the specific network
    int networks_found = scanNetworks(ssid);
    
    if (networks_found == 0) {
        Diagnostics::Logger::warning("WiFi Connection Manager: Network not found: " + ssid);
        return false;
    }
    
    // Check security protocol of found network(s)
    for (int i = 0; i < networks_found; i++) {
        if (WiFi.SSID(i) == ssid) {
            return isSupportedSecurityProtocol(i);
        }
    }
    
    return false;
}

void WiFiConnectionManager::updateConnectionStateMachine() {
    DeviceConfiguration::ConnectionState current_state = getConnectionState();
    
    switch (current_state) {
        case DeviceConfiguration::ConnectionState::CONNECTING:
            if (isWiFiConnected()) {
                setConnectionState(DeviceConfiguration::ConnectionState::CONNECTED);
                Diagnostics::Logger::info("WiFi Connection Manager: Successfully connected to WiFi");
                resetRetryState();
            } else if (shouldRetryConnection()) {
                attemptConnection();
            } else {
                // Max retries reached
                setConnectionState(DeviceConfiguration::ConnectionState::CONNECTION_FAILED);
                Diagnostics::Logger::error("WiFi Connection Manager: Connection failed after max retries");
            }
            break;
            
        case DeviceConfiguration::ConnectionState::CONNECTED:
            if (!isWiFiConnected()) {
                Diagnostics::Logger::warning("WiFi Connection Manager: Lost WiFi connection");
                setConnectionState(DeviceConfiguration::ConnectionState::CONNECTING);
                resetRetryState();
            }
            break;
            
        case DeviceConfiguration::ConnectionState::CONNECTION_FAILED:
            // Stay in failed state until manual retry or reset
            break;
            
        case DeviceConfiguration::ConnectionState::SETUP_MODE:
            // Handle setup mode timeouts and transitions
            break;
            
        default:
            break;
    }
}

void WiFiConnectionManager::updateLEDStatus() {
    if (!led_controller_) {
        return;
    }
    
    DeviceConfiguration::ConnectionState state = getConnectionState();
    
    switch (state) {
        case DeviceConfiguration::ConnectionState::CONNECTING:
            led_controller_->setPattern(Actuators::StatusLEDController::StatusPattern::WIFI_CONNECTING);
            break;
            
        case DeviceConfiguration::ConnectionState::CONNECTED:
            led_controller_->setPattern(Actuators::StatusLEDController::StatusPattern::WIFI_CONNECTED);
            break;
            
        case DeviceConfiguration::ConnectionState::CONNECTION_FAILED:
            led_controller_->setPattern(Actuators::StatusLEDController::StatusPattern::WIFI_ERROR);
            break;
            
        case DeviceConfiguration::ConnectionState::SETUP_MODE:
            led_controller_->setPattern(Actuators::StatusLEDController::StatusPattern::WIFI_SETUP);
            break;
            
        default:
            led_controller_->setPattern(Actuators::StatusLEDController::StatusPattern::OFF);
            break;
    }
}

void WiFiConnectionManager::handleConnectionTimeout() {
    Diagnostics::Logger::warning("WiFi Connection Manager: Connection timeout");
    
    if (shouldRetryConnection()) {
        attemptConnection();
    } else {
        setConnectionState(DeviceConfiguration::ConnectionState::CONNECTION_FAILED);
    }
}

void WiFiConnectionManager::handleSetupTimeout() {
    Diagnostics::Logger::info("WiFi Connection Manager: Setup session timed out");
    
    // End the setup session
    setup_session_.active = false;
    
    // If no credentials were submitted, stay in setup mode
    if (!setup_session_.credentials_submitted) {
        Diagnostics::Logger::info("WiFi Connection Manager: No credentials submitted, extending setup session");
        
        // Create new session
        setup_session_.session_id = generateSessionId();
        setup_session_.start_time = millis();
        setup_session_.active = true;
        setup_session_.credentials_submitted = false;
        setup_session_.validation_in_progress = false;
    } else {
        // Exit setup mode and attempt connection
        exitSetupMode();
    }
}

bool WiFiConnectionManager::attemptConnection() {
    if (retry_count_ >= MAX_CONNECTION_RETRIES) {
        return false;
    }
    
    unsigned long current_time = millis();
    uint16_t retry_delay = retry_delays_[retry_count_];
    
    if (current_time - last_retry_time_ < retry_delay) {
        return false; // Not time for retry yet
    }
    
    Diagnostics::Logger::info("WiFi Connection Manager: Retry attempt " + String(retry_count_ + 1) + "/" + String(MAX_CONNECTION_RETRIES));
    
    // Disconnect and reconnect
    WiFi.disconnect();
    delay(100);
    
    if (credential_manager_) {
        String ssid, password;
        if (credential_manager_->getStoredCredentials(ssid, password)) {
            WiFi.begin(ssid.c_str(), password.c_str());
        }
    }
    
    retry_count_++;
    last_retry_time_ = current_time;
    
    return true;
}

int WiFiConnectionManager::scanNetworks(const String& ssid) {
    WiFi.scanDelete(); // Clear previous scan results
    
    int networks_found = WiFi.scanNetworks();
    
    if (ssid.length() > 0) {
        // Filter for specific SSID
        int filtered_count = 0;
        for (int i = 0; i < networks_found; i++) {
            if (WiFi.SSID(i) == ssid) {
                filtered_count++;
            }
        }
        return filtered_count;
    }
    
    return networks_found;
}

bool WiFiConnectionManager::isSupportedSecurityProtocol(int network_index) {
    if (network_index < 0 || network_index >= WiFi.scanComplete()) {
        return false;
    }
    
    wifi_auth_mode_t auth_mode = WiFi.encryptionType(network_index);
    
    // Support WPA2 and WPA3 protocols
    switch (auth_mode) {
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
        case WIFI_AUTH_WPA2_ENTERPRISE:
        case WIFI_AUTH_WPA3_PSK:
        case WIFI_AUTH_WPA2_WPA3_PSK:
        case WIFI_AUTH_OPEN: // Allow open networks
            return true;
            
        default:
            Diagnostics::Logger::warning("WiFi Connection Manager: Unsupported security protocol: " + String(auth_mode));
            return false;
    }
}

String WiFiConnectionManager::generateAPSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    char ap_ssid[32];
    snprintf(ap_ssid, sizeof(ap_ssid), "GarageSensor-%02X%02X%02X", 
             mac[3], mac[4], mac[5]);
    
    return String(ap_ssid);
}

String WiFiConnectionManager::generateSessionId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    unsigned long timestamp = millis();
    
    char session_id[17];
    snprintf(session_id, sizeof(session_id), "%02X%02X%02X%08lX", 
             mac[3], mac[4], mac[5], timestamp);
    
    return String(session_id);
}

void WiFiConnectionManager::setConnectionState(DeviceConfiguration::ConnectionState new_state) {
    if (new_state != last_connection_state_) {
        last_connection_state_ = new_state;
        state_change_time_ = millis();
        
        // Update device configuration
        DeviceConfiguration::getInstance().setConnectionState(new_state);
        
        Diagnostics::Logger::info("WiFi Connection Manager: State changed to " + String(static_cast<int>(new_state)));
    }
}

void WiFiConnectionManager::resetRetryState() {
    retry_count_ = 0;
    last_retry_time_ = 0;
}

bool WiFiConnectionManager::shouldRetryConnection() {
    return retry_count_ < MAX_CONNECTION_RETRIES;
}