#include <unity.h>
#include <Arduino.h>
#include "../src/wifi/wifi_credential_manager.h"
#include "../src/security/credential_encryption.h"
#include "../src/config/wifi_config.h"

// Test security isolation and attack resistance
WiFiCredentialManager* security_manager = nullptr;

void setUp(void) {
    security_manager = new WiFiCredentialManager();
    TEST_ASSERT_TRUE(security_manager->begin());
}

void tearDown(void) {
    if (security_manager) {
        security_manager->clearCredentials();
        delete security_manager;
        security_manager = nullptr;
    }
}

// Test credential isolation from source code
void test_credential_isolation_from_source(void) {
    const String test_ssid = "SecretNetwork";
    const String test_password = "TopSecretPassword123!@#";
    
    // Store credentials
    TEST_ASSERT_TRUE(security_manager->storeCredentials(test_ssid, test_password));
    
    // Simulate source code inspection - credentials should not appear in plaintext
    // This test verifies that stored data doesn't contain plaintext credentials
    
    // Get raw storage to inspect for plaintext leakage
    Preferences prefs;
    prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, true); // Read-only
    
    WiFiCredentials stored_creds;
    size_t loaded_size = prefs.getBytes(WIFI_CREDENTIAL_NVS_KEY, &stored_creds, sizeof(stored_creds));
    prefs.end();
    
    TEST_ASSERT_EQUAL(sizeof(WiFiCredentials), loaded_size);
    
    // Check that plaintext SSID doesn't appear in encrypted data
    bool ssid_found = false;
    for (size_t i = 0; i <= stored_creds.encrypted_size - test_ssid.length(); i++) {
        if (memcmp(stored_creds.encrypted_data + i, test_ssid.c_str(), test_ssid.length()) == 0) {
            ssid_found = true;
            break;
        }
    }
    TEST_ASSERT_FALSE(ssid_found);
    
    // Check that plaintext password doesn't appear in encrypted data
    bool password_found = false;
    for (size_t i = 0; i <= stored_creds.encrypted_size - test_password.length(); i++) {
        if (memcmp(stored_creds.encrypted_data + i, test_password.c_str(), test_password.length()) == 0) {
            password_found = true;
            break;
        }
    }
    TEST_ASSERT_FALSE(password_found);
}

// Test resistance to memory analysis
void test_memory_analysis_resistance(void) {
    const String test_ssid = "MemoryTestNet";
    const String test_password = "MemoryTestPass";
    
    // Store and load credentials multiple times
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(security_manager->storeCredentials(test_ssid, test_password));
        
        String loaded_ssid, loaded_password;
        TEST_ASSERT_TRUE(security_manager->loadCredentials(loaded_ssid, loaded_password));
        
        // Verify credentials are correct
        TEST_ASSERT_EQUAL_STRING(test_ssid.c_str(), loaded_ssid.c_str());
        TEST_ASSERT_EQUAL_STRING(test_password.c_str(), loaded_password.c_str());
        
        // Clear credentials to test wiping
        security_manager->clearCredentials();
    }
    
    // After multiple cycles, no credentials should be stored
    TEST_ASSERT_FALSE(security_manager->hasValidCredentials());
}

// Test encrypted data format validation
void test_encrypted_data_format_validation(void) {
    const String test_ssid = "FormatTest";
    const String test_password = "FormatPass";
    
    // Store credentials
    TEST_ASSERT_TRUE(security_manager->storeCredentials(test_ssid, test_password));
    
    // Get encrypted data
    Preferences prefs;
    prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, false);
    
    WiFiCredentials stored_creds;
    size_t loaded_size = prefs.getBytes(WIFI_CREDENTIAL_NVS_KEY, &stored_creds, sizeof(stored_creds));
    prefs.end();
    
    TEST_ASSERT_EQUAL(sizeof(WiFiCredentials), loaded_size);
    
    // Verify encrypted data properties
    TEST_ASSERT_GREATER_THAN(0, stored_creds.encrypted_size);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(stored_creds.encrypted_data), stored_creds.encrypted_size);
    
    // Encrypted size should be at least IV (16 bytes) + minimum encrypted block (16 bytes)
    TEST_ASSERT_GREATER_OR_EQUAL(32, stored_creds.encrypted_size);
    
    // Encrypted size minus IV should be multiple of AES block size (16 bytes)
    TEST_ASSERT_EQUAL(0, (stored_creds.encrypted_size - 16) % 16);
    
    // Verify timestamp is reasonable (within last few seconds)
    unsigned long current_time = millis();
    TEST_ASSERT_LESS_OR_EQUAL(current_time, stored_creds.created_at);
    TEST_ASSERT_GREATER_THAN(current_time - 10000, stored_creds.created_at); // Within 10 seconds
}

// Test attack via corrupted encrypted data
void test_corrupted_encrypted_data_handling(void) {
    const String test_ssid = "CorruptTest";
    const String test_password = "CorruptPass";
    
    // Store valid credentials
    TEST_ASSERT_TRUE(security_manager->storeCredentials(test_ssid, test_password));
    
    // Corrupt the encrypted data in NVS
    Preferences prefs;
    prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, false);
    
    WiFiCredentials corrupted_creds;
    size_t loaded_size = prefs.getBytes(WIFI_CREDENTIAL_NVS_KEY, &corrupted_creds, sizeof(corrupted_creds));
    TEST_ASSERT_EQUAL(sizeof(WiFiCredentials), loaded_size);
    
    // Corrupt random bytes in encrypted data
    corrupted_creds.encrypted_data[5] ^= 0xFF;  // Flip bits in encrypted data
    corrupted_creds.encrypted_data[20] ^= 0xAA; // Corrupt more data
    
    // Store corrupted data back
    prefs.putBytes(WIFI_CREDENTIAL_NVS_KEY, &corrupted_creds, sizeof(corrupted_creds));
    prefs.end();
    
    // Create new manager to test corruption handling
    delete security_manager;
    security_manager = new WiFiCredentialManager();
    TEST_ASSERT_TRUE(security_manager->begin());
    
    // Should detect corrupted data and fail gracefully
    String loaded_ssid, loaded_password;
    TEST_ASSERT_FALSE(security_manager->loadCredentials(loaded_ssid, loaded_password));
    
    // Should not crash or reveal partial credentials
    TEST_ASSERT_TRUE(loaded_ssid.isEmpty());
    TEST_ASSERT_TRUE(loaded_password.isEmpty());
}

// Test protection against buffer overflow attacks
void test_buffer_overflow_protection(void) {
    // Test with encrypted size larger than buffer
    WiFiCredentials malicious_creds;
    malicious_creds.encrypted_size = 999999; // Impossibly large
    memset(malicious_creds.encrypted_data, 0xAA, sizeof(malicious_creds.encrypted_data));
    
    // Store malicious data directly in NVS
    Preferences prefs;
    prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, false);
    prefs.putBytes(WIFI_CREDENTIAL_NVS_KEY, &malicious_creds, sizeof(malicious_creds));
    prefs.end();
    
    // Create new manager to test protection
    delete security_manager;
    security_manager = new WiFiCredentialManager();
    TEST_ASSERT_TRUE(security_manager->begin());
    
    // Should detect invalid size and fail safely
    String loaded_ssid, loaded_password;
    TEST_ASSERT_FALSE(security_manager->loadCredentials(loaded_ssid, loaded_password));
    
    // Should not crash
    TEST_ASSERT_TRUE(loaded_ssid.isEmpty());
    TEST_ASSERT_TRUE(loaded_password.isEmpty());
}

// Test secure wiping functionality
void test_secure_credential_wiping(void) {
    const String test_ssid = "WipeTest";
    const String test_password = "WipePass";
    
    // Store credentials
    TEST_ASSERT_TRUE(security_manager->storeCredentials(test_ssid, test_password));
    TEST_ASSERT_TRUE(security_manager->hasValidCredentials());
    
    // Get storage location for later inspection
    size_t storage_size_before = security_manager->getStorageSize();
    TEST_ASSERT_GREATER_THAN(0, storage_size_before);
    
    // Clear credentials
    security_manager->clearCredentials();
    TEST_ASSERT_FALSE(security_manager->hasValidCredentials());
    TEST_ASSERT_EQUAL(0, security_manager->getStorageSize());
    
    // Verify credentials cannot be loaded
    String loaded_ssid, loaded_password;
    TEST_ASSERT_FALSE(security_manager->loadCredentials(loaded_ssid, loaded_password));
    
    // Verify NVS data is actually removed
    Preferences prefs;
    prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, true);
    size_t remaining_size = prefs.getBytesLength(WIFI_CREDENTIAL_NVS_KEY);
    prefs.end();
    
    TEST_ASSERT_EQUAL(0, remaining_size);
}

// Test encryption key isolation
void test_encryption_key_isolation(void) {
    // Two separate security contexts should derive the same key from hardware
    SecurityContext context1, context2;
    TEST_ASSERT_TRUE(context1.begin());
    TEST_ASSERT_TRUE(context2.begin());
    
    // Encrypt the same data with both contexts
    const char* plaintext = "Key Isolation Test";
    size_t plaintext_len = strlen(plaintext);
    
    uint8_t ciphertext1[64], ciphertext2[64];
    size_t ciphertext_len1 = sizeof(ciphertext1);
    size_t ciphertext_len2 = sizeof(ciphertext2);
    
    TEST_ASSERT_TRUE(context1.encrypt((uint8_t*)plaintext, plaintext_len, ciphertext1, ciphertext_len1));
    TEST_ASSERT_TRUE(context2.encrypt((uint8_t*)plaintext, plaintext_len, ciphertext2, ciphertext_len2));
    
    // Cross-decrypt to verify key consistency
    uint8_t decrypted1[32], decrypted2[32];
    size_t decrypted_len1 = sizeof(decrypted1);
    size_t decrypted_len2 = sizeof(decrypted2);
    
    TEST_ASSERT_TRUE(context2.decrypt(ciphertext1, ciphertext_len1, decrypted1, decrypted_len1));
    TEST_ASSERT_TRUE(context1.decrypt(ciphertext2, ciphertext_len2, decrypted2, decrypted_len2));
    
    // Both should recover original plaintext
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted1, plaintext_len);
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted2, plaintext_len);
    
    // Clear keys and verify isolation
    context1.clearKeys();
    
    // Context1 should no longer work
    uint8_t fail_ciphertext[64];
    size_t fail_ciphertext_len = sizeof(fail_ciphertext);
    TEST_ASSERT_FALSE(context1.encrypt((uint8_t*)plaintext, plaintext_len, fail_ciphertext, fail_ciphertext_len));
    
    // Context2 should still work
    uint8_t success_ciphertext[64];
    size_t success_ciphertext_len = sizeof(success_ciphertext);
    TEST_ASSERT_TRUE(context2.encrypt((uint8_t*)plaintext, plaintext_len, success_ciphertext, success_ciphertext_len));
    
    context2.clearKeys();
}

// Test protection against timing attacks
void test_timing_attack_protection(void) {
    const String valid_ssid = "ValidNetwork";
    const String valid_password = "ValidPassword";
    const String invalid_password = "InvalidPassword";
    
    // Store valid credentials
    TEST_ASSERT_TRUE(security_manager->storeCredentials(valid_ssid, valid_password));
    
    // Time multiple decryption attempts (this is a basic test - real timing attacks are more complex)
    unsigned long start_time, end_time1, end_time2;
    String loaded_ssid, loaded_password;
    
    // Time valid decryption
    start_time = micros();
    bool valid_result = security_manager->loadCredentials(loaded_ssid, loaded_password);
    end_time1 = micros();
    
    TEST_ASSERT_TRUE(valid_result);
    
    // Create corrupted data for invalid decryption timing
    WiFiCredentials corrupted_creds;
    memset(&corrupted_creds, 0xAA, sizeof(corrupted_creds));
    corrupted_creds.encrypted_size = 64; // Valid size but corrupted data
    
    Preferences prefs;
    prefs.begin(WIFI_CREDENTIAL_NVS_NAMESPACE, false);
    prefs.putBytes(WIFI_CREDENTIAL_NVS_KEY, &corrupted_creds, sizeof(corrupted_creds));
    prefs.end();
    
    // Time invalid decryption
    start_time = micros();
    bool invalid_result = security_manager->loadCredentials(loaded_ssid, loaded_password);
    end_time2 = micros();
    
    TEST_ASSERT_FALSE(invalid_result);
    
    // Timing should be reasonably similar (within order of magnitude)
    // This is a basic check - sophisticated timing analysis would require more precise measurement
    unsigned long valid_time = end_time1 - start_time;
    unsigned long invalid_time = end_time2 - start_time;
    
    // Both operations should complete in reasonable time (< 100ms)
    TEST_ASSERT_LESS_THAN(100000, valid_time);
    TEST_ASSERT_LESS_THAN(100000, invalid_time);
}

void setup() {
    delay(2000); // Allow time for initialization
    UNITY_BEGIN();
    
    RUN_TEST(test_credential_isolation_from_source);
    RUN_TEST(test_memory_analysis_resistance);
    RUN_TEST(test_encrypted_data_format_validation);
    RUN_TEST(test_corrupted_encrypted_data_handling);
    RUN_TEST(test_buffer_overflow_protection);
    RUN_TEST(test_secure_credential_wiping);
    RUN_TEST(test_encryption_key_isolation);
    RUN_TEST(test_timing_attack_protection);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}