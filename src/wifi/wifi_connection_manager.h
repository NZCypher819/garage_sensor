#pragma once

// WiFi Connection Manager for Secure WiFi Credential Storage
// Feature: 002-secure-wifi-storage

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "../config/wifi_config.h"
#include "wifi_credential_manager.h"
#include "../actuators/status_led_controller.h"
#include "../config/device_configuration.h"
#include "wifi_setup_portal.h"  // For SetupSession

// Forward declaration
class WiFiCredentialManager;

/**
 * WiFi Connection Manager
 * Handles connection management, state transitions, and access point creation
 */
class WiFiConnectionManager {
public:
    WiFiConnectionManager();
    ~WiFiConnectionManager();
    
    /**
     * Initialize connection manager
     * @param credential_manager Pointer to credential manager instance
     * @param led_controller Pointer to LED controller for visual feedback
     * @return true if initialization successful
     */
    bool begin(WiFiCredentialManager* credential_manager, 
              Actuators::StatusLEDController* led_controller);
    
    /**
     * Main update loop - call regularly from main loop
     * Handles connection state machine and timeouts
     */
    void update();
    
    /**
     * Attempt to connect to stored WiFi network
     * @return true if connection attempt started successfully
     */
    bool connectToStoredNetwork();
    
    /**
     * Create temporary access point for setup
     * @return true if access point created successfully
     */
    bool createSetupAccessPoint();
    
    /**
     * Stop temporary access point
     */
    void stopSetupAccessPoint();
    
    /**
     * Validate WiFi credentials by attempting connection
     * @param ssid WiFi network name
     * @param password WiFi network password
     * @return true if connection successful
     */
    bool validateCredentials(const String& ssid, const String& password);
    
    /**
     * Enter setup mode (create AP, start session)
     * @return true if setup mode activated
     */
    bool enterSetupMode();
    
    /**
     * Exit setup mode (stop AP, clear session)
     */
    void exitSetupMode();
    
    /**
     * Check if device is in setup mode
     * @return true if setup mode active
     */
    bool isInSetupMode() const;
    
    /**
     * Get current connection state
     * @return connection state from device configuration
     */
    DeviceConfiguration::ConnectionState getConnectionState() const;
    
    /**
     * Get current setup session
     * @return reference to setup session (read-only)
     */
    const SetupSession& getSetupSession() const { return setup_session_; }
    
    /**
     * Get device access point SSID
     * @return AP SSID string (e.g., "GarageSensor-A1B2C3")
     */
    String getDeviceAPSSID() const { return device_ap_ssid_; }
    
    /**
     * Handle physical reset button press
     * @param button_pressed true if reset button currently pressed
     */
    void handlePhysicalReset(bool button_pressed);
    
    /**
     * Check if WiFi is currently connected
     * @return true if connected to WiFi network
     */
    bool isWiFiConnected() const;
    
    /**
     * Get current WiFi status information
     * @param connected_ssid Output - currently connected SSID (if connected)
     * @param signal_strength Output - signal strength in dBm
     * @param ip_address Output - assigned IP address
     * @return true if connected, false otherwise
     */
    bool getWiFiStatus(String& connected_ssid, int& signal_strength, String& ip_address) const;
    
    /**
     * Force disconnect from current WiFi network
     */
    void disconnect();
    
    /**
     * Validate WPA2/WPA3 protocol support for given network
     * @param ssid Network to scan and validate
     * @return true if supported security protocol detected
     */
    bool validateWiFiSecurity(const String& ssid);

private:
    WiFiCredentialManager* credential_manager_;
    Actuators::StatusLEDController* led_controller_;
    SetupSession setup_session_;
    
    // Connection retry state
    uint8_t retry_count_;
    unsigned long last_retry_time_;
    const uint16_t retry_delays_[3] = WIFI_RETRY_DELAYS_MS;
    
    // Physical reset button state
    bool button_pressed_last_;
    unsigned long button_press_start_;
    
    // Access point state
    bool access_point_active_;
    String device_ap_ssid_;
    
    // Connection state management
    DeviceConfiguration::ConnectionState last_connection_state_;
    unsigned long state_change_time_;
    
    /**
     * Update connection state machine
     */
    void updateConnectionStateMachine();
    
    /**
     * Update LED status based on current state
     */
    void updateLEDStatus();
    
    /**
     * Handle connection timeout
     */
    void handleConnectionTimeout();
    
    /**
     * Handle setup session timeout
     */
    void handleSetupTimeout();
    
    /**
     * Attempt WiFi connection with exponential backoff
     * @return true if connection attempt started
     */
    bool attemptConnection();
    
    /**
     * Scan for available WiFi networks
     * @param ssid Specific SSID to scan for (empty for all networks)
     * @return number of networks found
     */
    int scanNetworks(const String& ssid = "");
    
    /**
     * Check if network uses supported security protocols
     * @param network_index Index from WiFi scan results
     * @return true if WPA2/WPA3 supported
     */
    bool isSupportedSecurityProtocol(int network_index);
    
    /**
     * Generate unique access point SSID
     * @return AP SSID string
     */
    String generateAPSSID();
    
    /**
     * Set connection state and update device configuration
     * @param new_state New connection state
     */
    void setConnectionState(DeviceConfiguration::ConnectionState new_state);
    
    /**
     * Reset connection retry counters
     */
    void resetRetryState();
    
    /**
     * Check if connection retry should be attempted
     * @return true if retry should be attempted
     */
    bool shouldRetryConnection();
    
    /**
     * Generate unique session ID for setup sessions
     * @return unique session ID string
     */
    String generateSessionId();
};

/**
 * Global connection manager instance
 */
extern WiFiConnectionManager g_wifi_connection_manager;