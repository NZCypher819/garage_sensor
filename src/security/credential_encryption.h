#pragma once

// AES-256 Encryption Utilities for Secure WiFi Credential Storage
// Feature: 002-secure-wifi-storage

#include <Arduino.h>
#include <AESLib.h>
#include <esp_system.h>
#include <esp_mac.h>

/**
 * Security Context for credential encryption
 * Manages AES-256 encryption keys and security materials
 */
class SecurityContext {
public:
    SecurityContext();
    ~SecurityContext();
    
    // Initialize security context with hardware-derived keys
    bool begin();
    
    // Encrypt plaintext data using AES-256-CBC
    bool encrypt(const uint8_t* plaintext, size_t plaintext_len, 
                uint8_t* ciphertext, size_t& ciphertext_len);
    
    // Decrypt ciphertext data using AES-256-CBC  
    bool decrypt(const uint8_t* ciphertext, size_t ciphertext_len,
                uint8_t* plaintext, size_t& plaintext_len);
    
    // Generate new initialization vector for each operation
    bool generateIV(uint8_t* iv, size_t iv_size);
    
    // Securely wipe all key material from memory
    void clearKeys();
    
    // Validate encryption/decryption is working correctly
    bool validateEncryption();
    
    // Get derived device identifier for AP naming
    String getDeviceId();
    
    // Get encryption key derivation salt
    bool getSalt(uint8_t* salt, size_t salt_size);
    
    // Check if security context is initialized
    bool isInitialized() const;

private:
    AESLib aes;
    AES aes_ctx;  // Lower-level AES context for raw operations
    uint8_t encryption_key[32];     // AES-256 key (32 bytes)
    uint8_t key_derivation_salt[16]; // Salt for key derivation (16 bytes)
    String device_mac;              // MAC address for device ID
    bool initialized;
    
    // Derive encryption key from ESP32 hardware sources
    bool deriveEncryptionKey();
    
    // Generate salt for key derivation  
    bool generateSalt();
    
    // Securely zero memory
    void secureZero(void* ptr, size_t size);
};

/**
 * High-level credential encryption functions
 */
namespace CredentialEncryption {
    
    // Encrypt WiFi credentials (SSID + password) 
    bool encryptCredentials(const String& ssid, const String& password,
                          uint8_t* encrypted_data, size_t& encrypted_size);
    
    // Decrypt WiFi credentials back to SSID + password
    bool decryptCredentials(const uint8_t* encrypted_data, size_t encrypted_size,
                          String& ssid, String& password);
    
    // Calculate required buffer size for encrypted credentials
    size_t getEncryptedCredentialSize(const String& ssid, const String& password);
    
    // Validate that decrypted credentials are valid
    bool validateCredentialFormat(const String& ssid, const String& password);
    
    // Securely wipe credential strings from memory
    void secureWipeCredentials(String& ssid, String& password);
}

// Error codes for encryption operations
enum class EncryptionError {
    OK = 0,
    INVALID_INPUT,
    BUFFER_TOO_SMALL, 
    KEY_DERIVATION_FAILED,
    ENCRYPTION_FAILED,
    DECRYPTION_FAILED,
    VALIDATION_FAILED,
    NOT_INITIALIZED
};

// Convert error code to human-readable string
const char* encryptionErrorToString(EncryptionError error);