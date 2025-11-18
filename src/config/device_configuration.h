#ifndef DEVICE_CONFIGURATION_H
#define DEVICE_CONFIGURATION_H

#include <Arduino.h>

/**
 * Device Configuration Management
 * 
 * Manages global device state and configuration settings.
 * Provides singleton access to device state information.
 */
class DeviceConfiguration {
public:
    /**
     * WiFi Connection States
     */
    enum class ConnectionState {
        DISCONNECTED = 0,    // No WiFi connection, not attempting
        CONNECTING,          // Actively attempting to connect
        CONNECTED,           // Successfully connected to WiFi
        CONNECTION_FAILED,   // Connection attempt failed
        SETUP_MODE          // Device in setup mode (AP active)
    };
    
    /**
     * LED Status States
     */
    enum class LEDState {
        OFF = 0,            // LED off
        SETUP_MODE,         // Blue pulsing for setup mode
        CONNECTING,         // Blue solid for connecting
        CONNECTED,          // Green solid for connected
        ERROR              // Red flashing for errors
    };
    
    /**
     * Device Operation Modes
     */
    enum class OperationMode {
        NORMAL = 0,         // Normal sensor operation
        SETUP,              // WiFi setup mode
        OTA_UPDATE,         // Over-the-air update in progress
        MAINTENANCE         // Maintenance mode
    };

    /**
     * Get singleton instance
     * @return Reference to the device configuration instance
     */
    static DeviceConfiguration& getInstance();
    
    /**
     * Initialize device configuration
     * @return true if initialization successful
     */
    bool begin();
    
    /**
     * Get current WiFi connection state
     * @return Current connection state
     */
    ConnectionState getConnectionState() const;
    
    /**
     * Set WiFi connection state
     * @param state New connection state
     */
    void setConnectionState(ConnectionState state);
    
    /**
     * Get current LED state
     * @return Current LED state
     */
    LEDState getLEDState() const;
    
    /**
     * Set LED state
     * @param state New LED state
     */
    void setLEDState(LEDState state);
    
    /**
     * Get current operation mode
     * @return Current operation mode
     */
    OperationMode getOperationMode() const;
    
    /**
     * Set operation mode
     * @param mode New operation mode
     */
    void setOperationMode(OperationMode mode);
    
    /**
     * Get device unique identifier
     * @return Device ID string
     */
    String getDeviceId() const;
    
    /**
     * Set device unique identifier
     * @param device_id New device ID
     */
    void setDeviceId(const String& device_id);
    
    /**
     * Check if device is in setup mode
     * @return true if in setup mode
     */
    bool isInSetupMode() const;
    
    /**
     * Set setup mode state
     * @param setup_mode true to enter setup mode
     */
    void setSetupMode(bool setup_mode);
    
    /**
     * Get connection retry count
     * @return Current retry count
     */
    int getConnectionRetryCount() const;
    
    /**
     * Set connection retry count
     * @param count New retry count
     */
    void setConnectionRetryCount(int count);
    
    /**
     * Get device firmware version
     * @return Firmware version string
     */
    String getFirmwareVersion() const;
    
    /**
     * Check if WiFi is connected
     * @return true if WiFi is connected
     */
    bool isWiFiConnected() const;
    
    /**
     * Get uptime in milliseconds
     * @return Device uptime
     */
    unsigned long getUptime() const;
    
    /**
     * Update device configuration
     * Call regularly from main loop
     */
    void update();
    
    /**
     * Save current configuration to persistent storage
     * @return true if save successful
     */
    bool saveConfiguration();
    
    /**
     * Load configuration from persistent storage
     * @return true if load successful
     */
    bool loadConfiguration();
    
    /**
     * Reset to factory defaults
     * @return true if reset successful
     */
    bool resetToDefaults();

private:
    // Private constructor for singleton
    DeviceConfiguration();
    
    // Delete copy constructor and assignment operator
    DeviceConfiguration(const DeviceConfiguration&) = delete;
    DeviceConfiguration& operator=(const DeviceConfiguration&) = delete;
    
    // Configuration state
    ConnectionState connection_state_;
    LEDState led_state_;
    OperationMode operation_mode_;
    
    // Device information
    String device_id_;
    String firmware_version_;
    unsigned long boot_time_;
    
    // State tracking
    unsigned long last_state_change_;
    bool configuration_dirty_;
    bool setup_mode_;
    int connection_retry_count_;
    
    /**
     * Generate unique device ID
     * @return Generated device ID
     */
    String generateDeviceId();
    
    /**
     * Initialize device ID from MAC address
     */
    void initializeDeviceId();
    
    /**
     * Initialize firmware version
     */
    void initializeFirmwareVersion();
    
    /**
     * Update LED state based on connection state
     */
    void updateLEDState();
};

/**
 * Global device configuration instance
 */
extern DeviceConfiguration& g_device_config;

#endif // DEVICE_CONFIGURATION_H