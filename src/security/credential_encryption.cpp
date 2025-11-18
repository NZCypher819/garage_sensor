#include "credential_encryption.h"
#include "../config/wifi_config.h"
#include <Preferences.h>
#include <esp_random.h>
#include <mbedtls/sha256.h>

// Global security context instance
static SecurityContext* g_security_context = nullptr;

SecurityContext::SecurityContext() : initialized(false) {
    secureZero(encryption_key, sizeof(encryption_key));
    secureZero(key_derivation_salt, sizeof(key_derivation_salt));
}

SecurityContext::~SecurityContext() {
    clearKeys();
}

bool SecurityContext::begin() {
    if (initialized) {
        return true;
    }
    
    Serial.println("[SecurityContext] Starting initialization...");
    
    // Get device MAC address for unique device identification
    uint8_t mac[6];
    Serial.println("[SecurityContext] Reading MAC address...");
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (ret != ESP_OK) {
        Serial.printf("[SecurityContext] ERROR: Failed to read MAC (error %d)\n", ret);
        return false;
    }
    
    // Format MAC as device identifier
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    device_mac = String(mac_str);
    Serial.printf("[SecurityContext] MAC: %s\n", mac_str);
    
    // Generate or load salt for key derivation
    Serial.println("[SecurityContext] Generating/loading salt...");
    if (!generateSalt()) {
        Serial.println("[SecurityContext] ERROR: Salt generation failed");
        return false;
    }
    Serial.println("[SecurityContext] Salt ready");
    
    // Derive encryption key from hardware sources
    Serial.println("[SecurityContext] Deriving encryption key...");
    if (!deriveEncryptionKey()) {
        Serial.println("[SecurityContext] ERROR: Key derivation failed");
        return false;
    }
    Serial.println("[SecurityContext] Key derived successfully");
    
    // Set up AES context with the derived key BEFORE validation
    aes_ctx.set_key(encryption_key, sizeof(encryption_key));
    
    // Mark as initialized so encrypt/decrypt functions will work
    initialized = true;
    
    // Validate encryption is working
    Serial.println("[SecurityContext] Validating encryption...");
    if (!validateEncryption()) {
        Serial.println("[SecurityContext] ERROR: Encryption validation failed");
        clearKeys();
        initialized = false;  // Reset on validation failure
        return false;
    }
    Serial.println("[SecurityContext] Encryption validated successfully");
    
    return true;
}

bool SecurityContext::encrypt(const uint8_t* plaintext, size_t plaintext_len, 
                            uint8_t* ciphertext, size_t& ciphertext_len) {
    Serial.printf("[SecurityContext] encrypt() called: plaintext_len=%d, buffer_len=%d\n", plaintext_len, ciphertext_len);
    
    if (!initialized || !plaintext || !ciphertext) {
        Serial.printf("[SecurityContext] Early fail: initialized=%d, plaintext=%p, ciphertext=%p\n", 
                     initialized, plaintext, ciphertext);
        return false;
    }
    
    // Generate random IV for this encryption operation
    uint8_t iv[16];
    if (!generateIV(iv, sizeof(iv))) {
        return false;
    }
    
    // Calculate padded length (PKCS7 padding to 16-byte blocks)
    size_t padded_len = plaintext_len;
    uint8_t padding = 16 - (plaintext_len % 16);
    if (padding == 0) {
        padding = 16; // Full block of padding
    }
    padded_len += padding;
    
    // Required size: IV (16 bytes) + padded ciphertext
    size_t required_size = 16 + padded_len;
    
    if (ciphertext_len < required_size) {
        ciphertext_len = required_size;
        return false;
    }
    
    // Create padded plaintext buffer
    uint8_t* padded_plaintext = new uint8_t[padded_len];
    memcpy(padded_plaintext, plaintext, plaintext_len);
    
    // Apply PKCS7 padding
    for (size_t i = plaintext_len; i < padded_len; i++) {
        padded_plaintext[i] = padding;
    }
    
    // Copy IV to beginning of ciphertext buffer
    memcpy(ciphertext, iv, 16);
    
    // Encrypt using AES-CBC mode manually
    uint8_t* cipher_ptr = ciphertext + 16;
    int n_blocks = padded_len / 16;
    
    Serial.printf("[SecurityContext] Encrypting %d blocks (%d bytes)...\n", n_blocks, padded_len);
    byte result = aes_ctx.cbc_encrypt(padded_plaintext, cipher_ptr, n_blocks, iv);
    Serial.printf("[SecurityContext] cbc_encrypt returned: %d\n", result);
    
    delete[] padded_plaintext;
    
    ciphertext_len = 16 + padded_len;
    return true;
}

bool SecurityContext::decrypt(const uint8_t* ciphertext, size_t ciphertext_len,
                            uint8_t* plaintext, size_t& plaintext_len) {
    if (!initialized || !ciphertext || !plaintext || ciphertext_len < 16) {
        return false;
    }
    
    // Extract IV from beginning of ciphertext
    uint8_t iv[16];
    memcpy(iv, ciphertext, 16);
    
    // The encrypted data follows the IV
    const uint8_t* encrypted_data = ciphertext + 16;
    size_t encrypted_size = ciphertext_len - 16;
    
    // Decrypt using AES-CBC mode manually
    uint8_t* decrypted_padded = new uint8_t[encrypted_size];
    int n_blocks = encrypted_size / 16;
    
    aes_ctx.cbc_decrypt(encrypted_data, decrypted_padded, n_blocks, iv);
    
    // Remove PKCS7 padding
    size_t decrypted_len = encrypted_size;
    if (decrypted_len > 0) {
        uint8_t padding = decrypted_padded[decrypted_len - 1];
        if (padding > 0 && padding <= 16) {
            // Verify padding is correct
            bool valid_padding = true;
            for (int i = 0; i < padding; i++) {
                if (decrypted_padded[decrypted_len - 1 - i] != padding) {
                    valid_padding = false;
                    break;
                }
            }
            if (valid_padding) {
                decrypted_len -= padding;
            }
        }
    }
    
    // Copy unpadded plaintext
    memcpy(plaintext, decrypted_padded, decrypted_len);
    plaintext_len = decrypted_len;
    
    delete[] decrypted_padded;
    return true;
}

bool SecurityContext::generateIV(uint8_t* iv, size_t iv_size) {
    if (!iv || iv_size != 16) {
        return false;
    }
    
    esp_fill_random(iv, iv_size);
    return true;
}

void SecurityContext::clearKeys() {
    secureZero(encryption_key, sizeof(encryption_key));
    secureZero(key_derivation_salt, sizeof(key_derivation_salt));
    initialized = false;
}

bool SecurityContext::validateEncryption() {
    const char* test_data = "WiFi Encryption Test";
    size_t test_len = strlen(test_data);
    
    uint8_t ciphertext[64];
    size_t ciphertext_len = sizeof(ciphertext);
    
    // Test encryption
    Serial.println("[SecurityContext] Testing encrypt...");
    if (!encrypt((uint8_t*)test_data, test_len, ciphertext, ciphertext_len)) {
        Serial.println("[SecurityContext] Encrypt test failed");
        return false;
    }
    Serial.printf("[SecurityContext] Encrypted %d bytes -> %d bytes\n", test_len, ciphertext_len);
    
    uint8_t decrypted[32];
    size_t decrypted_len = sizeof(decrypted);
    
    // Test decryption
    Serial.println("[SecurityContext] Testing decrypt...");
    if (!decrypt(ciphertext, ciphertext_len, decrypted, decrypted_len)) {
        Serial.println("[SecurityContext] Decrypt test failed");
        return false;
    }
    Serial.printf("[SecurityContext] Decrypted %d bytes -> %d bytes\n", ciphertext_len, decrypted_len);
    
    // Verify data integrity
    if (decrypted_len != test_len || memcmp(test_data, decrypted, test_len) != 0) {
        Serial.printf("[SecurityContext] Data mismatch: expected %d bytes, got %d bytes\n", test_len, decrypted_len);
        secureZero(decrypted, sizeof(decrypted));
        return false;
    }
    
    Serial.println("[SecurityContext] Encryption roundtrip successful");
    secureZero(decrypted, sizeof(decrypted));
    return true;
}

String SecurityContext::getDeviceId() {
    return device_mac;
}

bool SecurityContext::getSalt(uint8_t* salt, size_t salt_size) {
    if (!salt || salt_size != sizeof(key_derivation_salt)) {
        return false;
    }
    
    memcpy(salt, key_derivation_salt, sizeof(key_derivation_salt));
    return true;
}

bool SecurityContext::isInitialized() const {
    return initialized;
}

bool SecurityContext::deriveEncryptionKey() {
    // Use ESP32 hardware sources for key derivation
    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (ret != ESP_OK) {
        return false;
    }
    
    // Create key derivation input from MAC + salt + chip ID
    uint8_t key_input[64];
    memcpy(key_input, mac, 6);
    memcpy(key_input + 6, key_derivation_salt, 16);
    
    // Get chip ID for additional entropy
    uint64_t chip_id = ESP.getEfuseMac();
    memcpy(key_input + 22, &chip_id, 8);
    
    // Fill remaining space with controlled data
    memset(key_input + 30, 0xA5, 34); // Pattern fill
    
    // Use SHA-256 to derive the AES key
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // SHA-256 (not SHA-224)
    mbedtls_sha256_update(&ctx, key_input, sizeof(key_input));
    mbedtls_sha256_finish(&ctx, encryption_key);
    mbedtls_sha256_free(&ctx);
    
    // Secure cleanup
    secureZero(key_input, sizeof(key_input));
    
    return true;
}

bool SecurityContext::generateSalt() {
    Preferences prefs;
    if (!prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Try to load existing salt
    size_t salt_len = prefs.getBytesLength("salt");
    if (salt_len == sizeof(key_derivation_salt)) {
        size_t read_len = prefs.getBytes("salt", key_derivation_salt, sizeof(key_derivation_salt));
        prefs.end();
        return (read_len == sizeof(key_derivation_salt));
    }
    
    // Generate new salt if none exists
    esp_fill_random(key_derivation_salt, sizeof(key_derivation_salt));
    
    // Store salt persistently
    bool success = prefs.putBytes("salt", key_derivation_salt, sizeof(key_derivation_salt));
    prefs.end();
    
    return success;
}

void SecurityContext::secureZero(void* ptr, size_t size) {
    if (ptr) {
        volatile uint8_t* p = (volatile uint8_t*)ptr;
        for (size_t i = 0; i < size; i++) {
            p[i] = 0;
        }
    }
}

// Utility function for secure memory wiping (available globally)
static void secureZero(void* ptr, size_t size) {
    if (ptr) {
        volatile uint8_t* p = (volatile uint8_t*)ptr;
        for (size_t i = 0; i < size; i++) {
            p[i] = 0;
        }
    }
}

// Global credential encryption functions
namespace CredentialEncryption {
    
    bool encryptCredentials(const String& ssid, const String& password,
                          uint8_t* encrypted_data, size_t& encrypted_size) {
        if (!g_security_context || !g_security_context->isInitialized()) {
            // Initialize global context if needed
            if (!g_security_context) {
                g_security_context = new SecurityContext();
            }
            if (!g_security_context->begin()) {
                return false;
            }
        }
        
        // Validate input
        if (!validateCredentialFormat(ssid, password)) {
            return false;
        }
        
        // Pack credentials into binary format: [ssid_len][ssid][password_len][password]
        size_t ssid_len = ssid.length();
        size_t password_len = password.length();
        size_t total_len = 2 + ssid_len + password_len; // 1 byte each for lengths
        
        uint8_t* plaintext = new uint8_t[total_len];
        plaintext[0] = (uint8_t)ssid_len;
        memcpy(plaintext + 1, ssid.c_str(), ssid_len);
        plaintext[1 + ssid_len] = (uint8_t)password_len;
        memcpy(plaintext + 2 + ssid_len, password.c_str(), password_len);
        
        // Encrypt the packed data
        bool success = g_security_context->encrypt(plaintext, total_len, encrypted_data, encrypted_size);
        
        // Secure cleanup
        g_security_context->clearKeys();
        secureZero(plaintext, total_len);
        delete[] plaintext;
        
        return success;
    }
    
    bool decryptCredentials(const uint8_t* encrypted_data, size_t encrypted_size,
                          String& ssid, String& password) {
        if (!g_security_context || !g_security_context->isInitialized()) {
            // Initialize global context if needed
            if (!g_security_context) {
                g_security_context = new SecurityContext();
            }
            if (!g_security_context->begin()) {
                return false;
            }
        }
        
        // Decrypt the data
        uint8_t plaintext[128]; // Max possible credential size
        size_t plaintext_len = sizeof(plaintext);
        
        if (!g_security_context->decrypt(encrypted_data, encrypted_size, plaintext, plaintext_len)) {
            return false;
        }
        
        // Unpack credentials from binary format
        if (plaintext_len < 2) {
            secureZero(plaintext, sizeof(plaintext));
            return false;
        }
        
        uint8_t ssid_len = plaintext[0];
        if (ssid_len > WIFI_MAX_SSID_LENGTH || 1 + ssid_len >= plaintext_len) {
            secureZero(plaintext, sizeof(plaintext));
            return false;
        }
        
        uint8_t password_len = plaintext[1 + ssid_len];
        if (password_len > WIFI_MAX_PASSWORD_LENGTH || 2 + ssid_len + password_len != plaintext_len) {
            secureZero(plaintext, sizeof(plaintext));
            return false;
        }
        
        // Extract strings
        char ssid_buffer[WIFI_MAX_SSID_LENGTH + 1] = {0};
        char password_buffer[WIFI_MAX_PASSWORD_LENGTH + 1] = {0};
        
        memcpy(ssid_buffer, plaintext + 1, ssid_len);
        memcpy(password_buffer, plaintext + 2 + ssid_len, password_len);
        
        ssid = String(ssid_buffer);
        password = String(password_buffer);
        
        // Secure cleanup
        secureZero(plaintext, sizeof(plaintext));
        secureZero(ssid_buffer, sizeof(ssid_buffer));
        secureZero(password_buffer, sizeof(password_buffer));
        
        return validateCredentialFormat(ssid, password);
    }
    
    size_t getEncryptedCredentialSize(const String& ssid, const String& password) {
        size_t plaintext_size = 2 + ssid.length() + password.length();
        size_t padded_size = ((plaintext_size + 15) / 16) * 16;
        return 16 + padded_size; // IV + padded ciphertext
    }
    
    bool validateCredentialFormat(const String& ssid, const String& password) {
        return (ssid.length() > 0 && ssid.length() <= WIFI_MAX_SSID_LENGTH &&
                password.length() > 0 && password.length() <= WIFI_MAX_PASSWORD_LENGTH);
    }
    
    void secureWipeCredentials(String& ssid, String& password) {
        // Arduino String doesn't provide secure wiping, but we can clear them
        ssid = "";
        password = "";
    }
}

const char* encryptionErrorToString(EncryptionError error) {
    switch (error) {
        case EncryptionError::OK: return "Success";
        case EncryptionError::INVALID_INPUT: return "Invalid input parameters";
        case EncryptionError::BUFFER_TOO_SMALL: return "Output buffer too small";
        case EncryptionError::KEY_DERIVATION_FAILED: return "Key derivation failed";
        case EncryptionError::ENCRYPTION_FAILED: return "Encryption operation failed";
        case EncryptionError::DECRYPTION_FAILED: return "Decryption operation failed";
        case EncryptionError::VALIDATION_FAILED: return "Credential validation failed";
        case EncryptionError::NOT_INITIALIZED: return "Security context not initialized";
        default: return "Unknown error";
    }
}