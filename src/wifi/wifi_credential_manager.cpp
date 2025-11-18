#include "wifi_credential_manager.h"
#include "../security/credential_encryption.h"
#include "../diagnostics/logger.h"

// Global instance
WiFiCredentialManager g_wifi_credential_manager;

WiFiCredentialManager::WiFiCredentialManager() 
    : security_context_(nullptr), initialized_(false) {
}

WiFiCredentialManager::~WiFiCredentialManager() {
    if (security_context_) {
        security_context_->clearKeys();
        delete security_context_;
    }
}

bool WiFiCredentialManager::begin() {
    if (initialized_) {
        return true;
    }
    
    Serial.println("[WiFiCredMgr] Starting initialization...");
    
    // Initialize NVS for persistent storage
    Serial.println("[WiFiCredMgr] Initializing NVS...");
    if (!initializeNVS()) {
        Serial.println("[WiFiCredMgr] ERROR: NVS initialization failed");
        return false;
    }
    Serial.println("[WiFiCredMgr] NVS initialized successfully");
    
    // Create and initialize security context
    Serial.println("[WiFiCredMgr] Creating SecurityContext...");
    security_context_ = new SecurityContext();
    if (!security_context_) {
        Serial.println("[WiFiCredMgr] ERROR: SecurityContext allocation failed");
        return false;
    }
    
    Serial.println("[WiFiCredMgr] Initializing SecurityContext...");
    if (!security_context_->begin()) {
        Serial.println("[WiFiCredMgr] ERROR: SecurityContext initialization failed");
        delete security_context_;
        security_context_ = nullptr;
        return false;
    }
    Serial.println("[WiFiCredMgr] SecurityContext initialized successfully");
    
    // Generate device identifier
    Serial.println("[WiFiCredMgr] Generating device ID...");
    if (!generateDeviceId()) {
        Serial.println("[WiFiCredMgr] ERROR: Device ID generation failed");
        return false;
    }
    Serial.println("[WiFiCredMgr] Device ID generated successfully");
    
    // Load device configuration if it exists
    Serial.println("[WiFiCredMgr] Loading device config...");
    loadDeviceConfig();
    
    initialized_ = true;
    Serial.println("[WiFiCredMgr] Initialization complete");
    return true;
}

bool WiFiCredentialManager::storeCredentials(const String& ssid, const String& password) {
    if (!initialized_ || !security_context_) {
        return false;
    }
    
    // Validate input credentials
    if (!validateCredentialFormat(ssid, password)) {
        return false;
    }
    
    // Calculate required encrypted buffer size
    size_t encrypted_size = CredentialEncryption::getEncryptedCredentialSize(ssid, password);
    if (encrypted_size > sizeof(WiFiCredentials::encrypted_data)) {
        return false;
    }
    
    // Encrypt credentials
    WiFiCredentials creds;
    creds.ssid = ssid;
    creds.password = password;
    creds.encrypted_size = sizeof(creds.encrypted_data);
    
    if (!CredentialEncryption::encryptCredentials(ssid, password, 
                                                creds.encrypted_data, creds.encrypted_size)) {
        // Secure cleanup
        secureZero(&creds, sizeof(creds));
        return false;
    }
    
    creds.created_at = millis();
    creds.validated = false; // Will be validated during connection attempt
    
    // Store encrypted credentials in NVS
    bool success = preferences_.putBytes(WIFI_CREDENTIAL_NVS_KEY, &creds, sizeof(WiFiCredentials));
    
    // Secure cleanup
    secureZero(&creds, sizeof(creds));
    
    if (success) {
        // Credentials stored successfully
        Diagnostics::Logger::info("WiFi Credential Manager: Credentials stored successfully");
    }
    
    return success;
}

bool WiFiCredentialManager::loadCredentials(String& ssid, String& password) {
    if (!initialized_ || !security_context_) {
        return false;
    }
    
    // Check if credentials exist
    if (!hasValidCredentials()) {
        return false;
    }
    
    // Load encrypted credentials from NVS
    WiFiCredentials creds;
    size_t loaded_size = preferences_.getBytes(WIFI_CREDENTIAL_NVS_KEY, &creds, sizeof(WiFiCredentials));
    
    if (loaded_size != sizeof(WiFiCredentials)) {
        secureZero(&creds, sizeof(creds));
        return false;
    }
    
    // Validate encrypted data
    if (!validateEncryptedData(creds.encrypted_data, creds.encrypted_size)) {
        secureZero(&creds, sizeof(creds));
        return false;
    }
    
    // Decrypt credentials
    bool success = CredentialEncryption::decryptCredentials(
        creds.encrypted_data, creds.encrypted_size, ssid, password);
    
    // Secure cleanup
    secureZero(&creds, sizeof(creds));
    
    if (!success) {
        CredentialEncryption::secureWipeCredentials(ssid, password);
        return false;
    }
    
    // Validate decrypted credential format
    if (!validateCredentialFormat(ssid, password)) {
        CredentialEncryption::secureWipeCredentials(ssid, password);
        return false;
    }
    
    return true;
}

bool WiFiCredentialManager::loadDeviceConfig() {
    // Load device configuration from NVS if it exists
    // For now, just return true - configuration is managed by DeviceConfiguration singleton
    return true;
}

bool WiFiCredentialManager::hasValidCredentials() {
    if (!initialized_) {
        return false;
    }
    
    size_t stored_size = preferences_.getBytesLength(WIFI_CREDENTIAL_NVS_KEY);
    return (stored_size == sizeof(WiFiCredentials));
}

void WiFiCredentialManager::clearCredentials() {
    if (!initialized_) {
        return;
    }
    
    // Remove credentials from NVS
    preferences_.remove(WIFI_CREDENTIAL_NVS_KEY);
    
    // Clear device configuration
    DeviceConfiguration& device_config = DeviceConfiguration::getInstance();
    device_config.setSetupMode(true);
    device_config.setConnectionState(DeviceConfiguration::ConnectionState::DISCONNECTED);
    device_config.setConnectionRetryCount(0);
    device_config.setLEDState(DeviceConfiguration::LEDState::SETUP_MODE);
}

bool WiFiCredentialManager::validateEncryption() {
    if (!initialized_ || !security_context_) {
        return false;
    }
    
    return security_context_->validateEncryption();
}

size_t WiFiCredentialManager::getStorageSize() {
    if (!initialized_) {
        return 0;
    }
    
    return preferences_.getBytesLength(WIFI_CREDENTIAL_NVS_KEY);
}

bool WiFiCredentialManager::hasStoredCredentials() const {
    if (!initialized_) {
        return false;
    }
    
    // Check if credential data exists in NVS
    // Note: Preferences methods are not const, need to access non-const member
    // This is safe since reading doesn't modify state
    WiFiCredentialManager* non_const_this = const_cast<WiFiCredentialManager*>(this);
    size_t required_size = non_const_this->preferences_.getBytesLength(WIFI_CREDENTIAL_NVS_KEY);
    return (required_size > 0 && required_size == sizeof(WiFiCredentials));
}

bool WiFiCredentialManager::getStoredCredentials(String& ssid, String& password) const {
    if (!initialized_ || !security_context_) {
        return false;
    }
    
    // Load encrypted credentials
    WiFiCredentials creds;
    WiFiCredentialManager* non_const_this = const_cast<WiFiCredentialManager*>(this);
    size_t loaded_size = non_const_this->preferences_.getBytes(WIFI_CREDENTIAL_NVS_KEY, &creds, sizeof(creds));
    
    if (loaded_size != sizeof(creds)) {
        return false;
    }
    
    // Decrypt credentials using namespace function
    String decrypted_ssid, decrypted_password;
    bool decrypt_success = CredentialEncryption::decryptCredentials(
        creds.encrypted_data, creds.encrypted_size,
        decrypted_ssid, decrypted_password
    );
    
    // Secure cleanup - need to cast away const for cleanup method
    non_const_this->secureZero(&creds, sizeof(creds));
    
    if (decrypt_success) {
        ssid = decrypted_ssid;
        password = decrypted_password;
        return true;
    }
    
    return false;
}

bool WiFiCredentialManager::validateCredentialFormat(const String& ssid, const String& password) {
    return (ssid.length() > 0 && ssid.length() <= WIFI_MAX_SSID_LENGTH &&
            password.length() > 0 && password.length() <= WIFI_MAX_PASSWORD_LENGTH);
}

String WiFiCredentialManager::getDeviceId() {
    if (!initialized_ || !security_context_) {
        // Generate temporary ID from MAC if not initialized
        uint8_t mac[6];
        esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
        if (ret == ESP_OK) {
            char mac_str[13];
            snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X", 
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return String(mac_str);
        }
        return String("UNKNOWN");
    }
    
    return security_context_->getDeviceId();
}

bool WiFiCredentialManager::initializeNVS() {
    return preferences_.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, false); // Read-write mode
}

bool WiFiCredentialManager::generateDeviceId() {
    if (!security_context_) {
        return false;
    }
    
    String device_id = security_context_->getDeviceId();
    DeviceConfiguration::getInstance().setDeviceId(device_id);
    return !device_id.isEmpty();
}

bool WiFiCredentialManager::validateEncryptedData(const uint8_t* encrypted_data, size_t encrypted_size) {
    if (!encrypted_data || encrypted_size == 0) {
        return false;
    }
    
    // Basic validation: must be at least IV size + minimum block size
    if (encrypted_size < 32) { // 16-byte IV + 16-byte minimum data
        return false;
    }
    
    // Size must be multiple of AES block size plus IV
    if ((encrypted_size - 16) % 16 != 0) {
        return false;
    }
    
    return true;
}

void WiFiCredentialManager::secureZero(void* ptr, size_t size) {
    if (ptr) {
        volatile uint8_t* p = (volatile uint8_t*)ptr;
        for (size_t i = 0; i < size; i++) {
            p[i] = 0;
        }
    }
}