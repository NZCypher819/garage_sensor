#pragma once

// WiFi Credential Manager for Secure Credential Storage
// Feature: 002-secure-wifi-storage

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "../config/wifi_config.h"
#include "../config/device_configuration.h"
#include "../security/credential_encryption.h"

/**
 * WiFi Credential Entity
 * Represents encrypted WiFi credentials stored in NVS
 */
struct WiFiCredentials {
    String ssid;                    // WiFi network name
    String password;                // WiFi network password
    uint8_t encrypted_data[128];    // AES-256 encrypted credential blob
    size_t encrypted_size;          // Size of encrypted data
    unsigned long created_at;       // Timestamp when stored
    bool validated;                 // Whether connection was tested
    
    WiFiCredentials() : encrypted_size(0), created_at(0), validated(false) {
        memset(encrypted_data, 0, sizeof(encrypted_data));
    }
};

// DeviceConfiguration moved to config/device_configuration.h

/**
 * WiFi Credential Manager
 * Core class for secure credential storage, encryption, and management
 */
class WiFiCredentialManager {
public:
    WiFiCredentialManager();
    ~WiFiCredentialManager();
    
    /**
     * Initialize credential manager and encryption context
     * @return true if initialization successful
     */
    bool begin();
    
    /**
     * Store WiFi credentials securely (encrypts before storage)
     * @param ssid WiFi network name
     * @param password WiFi network password
     * @return true on success, false on encryption/storage failure
     */
    bool storeCredentials(const String& ssid, const String& password);
    
    /**
     * Load and decrypt stored credentials
     * @param ssid Output WiFi network name
     * @param password Output WiFi network password
     * @return true if credentials exist and decrypt successfully
     */
    bool loadCredentials(String& ssid, String& password);
    
    /**
     * Check if valid credentials exist in storage
     * @return true if encrypted credentials are stored
     */
    bool hasValidCredentials();
    
    /**
     * Securely wipe all stored credential data
     */
    void clearCredentials();
    
    /**
     * Validate encryption/decryption is working correctly
     * @return true if encryption round-trip succeeds
     */
    bool validateEncryption();
    
    /**
     * Get storage statistics for diagnostics
     * @return size of stored credential data in bytes
     */
    size_t getStorageSize();
    
    /**
     * Check if credentials are stored
     * @return true if valid credentials exist
     */
    bool hasStoredCredentials() const;
    
    /**
     * Get stored credentials
     * @param ssid Output parameter for SSID
     * @param password Output parameter for password
     * @return true if credentials retrieved successfully
     */
    bool getStoredCredentials(String& ssid, String& password) const;
    
    /**
     * Save device configuration to persistent storage
     * @return true if save successful
     */
    bool saveDeviceConfig();
    
    /**
     * Load device configuration from persistent storage
     * @return true if load successful
     */
    bool loadDeviceConfig();
    
    /**
     * Check credential format validity
     * @param ssid WiFi network name to validate
     * @param password WiFi password to validate
     * @return true if credentials meet format requirements
     */
    bool validateCredentialFormat(const String& ssid, const String& password);
    
    /**
     * Get unique device identifier for AP naming
     * @return device ID string (MAC-based)
     */
    String getDeviceId();
    
    /**
     * Check if credential manager is initialized
     * @return true if ready for operations
     */
    bool isInitialized() const { return initialized_; }

private:
    Preferences preferences_;
    SecurityContext* security_context_;
    bool initialized_;
    
    /**
     * Initialize NVS namespace for credential storage
     * @return true if namespace ready
     */
    bool initializeNVS();
    
    /**
     * Generate device ID from hardware MAC address
     * @return true if device ID generated successfully
     */
    bool generateDeviceId();
    
    /**
     * Validate stored encrypted data integrity
     * @param encrypted_data Encrypted credential blob
     * @param encrypted_size Size of encrypted data
     * @return true if data appears valid
     */
    bool validateEncryptedData(const uint8_t* encrypted_data, size_t encrypted_size);
    
    /**
     * Securely zero memory containing sensitive data
     * @param ptr Pointer to memory to clear
     * @param size Number of bytes to clear
     */
    void secureZero(void* ptr, size_t size);
};

/**
 * Global credential manager instance
 * Provides singleton access to credential management
 */
extern WiFiCredentialManager g_wifi_credential_manager;