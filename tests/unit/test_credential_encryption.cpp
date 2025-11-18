#include <unity.h>
#include <Arduino.h>
#include "../src/security/credential_encryption.h"
#include "../src/config/wifi_config.h"

// Test fixtures
SecurityContext* test_security_context = nullptr;
const char* test_ssid = "TestNetwork";
const char* test_password = "TestPassword123";

void setUp(void) {
    // Initialize security context for each test
    test_security_context = new SecurityContext();
    TEST_ASSERT_TRUE(test_security_context->begin());
}

void tearDown(void) {
    // Clean up after each test
    if (test_security_context) {
        test_security_context->clearKeys();
        delete test_security_context;
        test_security_context = nullptr;
    }
}

// Test encryption/decryption basic functionality
void test_encrypt_decrypt_basic(void) {
    const char* plaintext = "WiFi Test Data";
    size_t plaintext_len = strlen(plaintext);
    
    uint8_t ciphertext[64];
    size_t ciphertext_len = sizeof(ciphertext);
    
    // Test encryption
    bool encrypt_success = test_security_context->encrypt(
        (uint8_t*)plaintext, plaintext_len, ciphertext, ciphertext_len);
    TEST_ASSERT_TRUE(encrypt_success);
    TEST_ASSERT_GREATER_THAN(plaintext_len, ciphertext_len);
    
    // Test decryption
    uint8_t decrypted[32];
    size_t decrypted_len = sizeof(decrypted);
    
    bool decrypt_success = test_security_context->decrypt(
        ciphertext, ciphertext_len, decrypted, decrypted_len);
    TEST_ASSERT_TRUE(decrypt_success);
    TEST_ASSERT_EQUAL(plaintext_len, decrypted_len);
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted, plaintext_len);
    
    // Ensure ciphertext doesn't contain plaintext
    bool plaintext_found = false;
    for (size_t i = 0; i <= ciphertext_len - plaintext_len; i++) {
        if (memcmp(ciphertext + i, plaintext, plaintext_len) == 0) {
            plaintext_found = true;
            break;
        }
    }
    TEST_ASSERT_FALSE(plaintext_found);
}

// Test credential encryption/decryption
void test_credential_encryption_decryption(void) {
    String original_ssid = test_ssid;
    String original_password = test_password;
    
    uint8_t encrypted_data[128];
    size_t encrypted_size = sizeof(encrypted_data);
    
    // Test credential encryption
    bool encrypt_success = CredentialEncryption::encryptCredentials(
        original_ssid, original_password, encrypted_data, encrypted_size);
    TEST_ASSERT_TRUE(encrypt_success);
    TEST_ASSERT_GREATER_THAN(0, encrypted_size);
    
    // Test credential decryption
    String decrypted_ssid, decrypted_password;
    bool decrypt_success = CredentialEncryption::decryptCredentials(
        encrypted_data, encrypted_size, decrypted_ssid, decrypted_password);
    TEST_ASSERT_TRUE(decrypt_success);
    
    // Verify credentials match
    TEST_ASSERT_EQUAL_STRING(test_ssid, decrypted_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(test_password, decrypted_password.c_str());
}

// Test encryption with invalid inputs
void test_encryption_invalid_inputs(void) {
    uint8_t ciphertext[64];
    size_t ciphertext_len = sizeof(ciphertext);
    
    // Test null plaintext
    bool result = test_security_context->encrypt(nullptr, 10, ciphertext, ciphertext_len);
    TEST_ASSERT_FALSE(result);
    
    // Test null ciphertext buffer
    const char* plaintext = "test";
    result = test_security_context->encrypt((uint8_t*)plaintext, 4, nullptr, ciphertext_len);
    TEST_ASSERT_FALSE(result);
    
    // Test buffer too small
    ciphertext_len = 10; // Too small for encrypted data
    result = test_security_context->encrypt((uint8_t*)plaintext, 4, ciphertext, ciphertext_len);
    TEST_ASSERT_FALSE(result);
}

// Test credential format validation
void test_credential_format_validation(void) {
    // Valid credentials
    TEST_ASSERT_TRUE(CredentialEncryption::validateCredentialFormat("ValidSSID", "ValidPassword"));
    
    // Empty SSID
    TEST_ASSERT_FALSE(CredentialEncryption::validateCredentialFormat("", "password"));
    
    // Empty password
    TEST_ASSERT_FALSE(CredentialEncryption::validateCredentialFormat("ssid", ""));
    
    // SSID too long (> 32 characters)
    String long_ssid = "";
    for (int i = 0; i < 35; i++) long_ssid += "a";
    TEST_ASSERT_FALSE(CredentialEncryption::validateCredentialFormat(long_ssid, "password"));
    
    // Password too long (> 63 characters) 
    String long_password = "";
    for (int i = 0; i < 65; i++) long_password += "a";
    TEST_ASSERT_FALSE(CredentialEncryption::validateCredentialFormat("ssid", long_password));
}

// Test key derivation consistency
void test_key_derivation_consistency(void) {
    // Create two security contexts
    SecurityContext context1, context2;
    TEST_ASSERT_TRUE(context1.begin());
    TEST_ASSERT_TRUE(context2.begin());
    
    // Both should generate the same device ID
    TEST_ASSERT_EQUAL_STRING(context1.getDeviceId().c_str(), context2.getDeviceId().c_str());
    
    // Test encryption consistency between contexts
    const char* plaintext = "Consistency Test";
    size_t plaintext_len = strlen(plaintext);
    
    uint8_t ciphertext1[64], ciphertext2[64];
    size_t ciphertext_len1 = sizeof(ciphertext1);
    size_t ciphertext_len2 = sizeof(ciphertext2);
    
    TEST_ASSERT_TRUE(context1.encrypt((uint8_t*)plaintext, plaintext_len, ciphertext1, ciphertext_len1));
    TEST_ASSERT_TRUE(context2.encrypt((uint8_t*)plaintext, plaintext_len, ciphertext2, ciphertext_len2));
    
    // Decrypt with opposite contexts to verify key consistency
    uint8_t decrypted1[32], decrypted2[32];
    size_t decrypted_len1 = sizeof(decrypted1);
    size_t decrypted_len2 = sizeof(decrypted2);
    
    TEST_ASSERT_TRUE(context2.decrypt(ciphertext1, ciphertext_len1, decrypted1, decrypted_len1));
    TEST_ASSERT_TRUE(context1.decrypt(ciphertext2, ciphertext_len2, decrypted2, decrypted_len2));
    
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted1, plaintext_len);
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted2, plaintext_len);
    
    context1.clearKeys();
    context2.clearKeys();
}

// Test secure key clearing
void test_secure_key_clearing(void) {
    // Encrypt some data
    const char* plaintext = "Secret Data";
    uint8_t ciphertext[64];
    size_t ciphertext_len = sizeof(ciphertext);
    
    TEST_ASSERT_TRUE(test_security_context->encrypt(
        (uint8_t*)plaintext, strlen(plaintext), ciphertext, ciphertext_len));
    
    // Clear keys
    test_security_context->clearKeys();
    
    // Verify context is no longer initialized
    TEST_ASSERT_FALSE(test_security_context->isInitialized());
    
    // Verify operations fail after key clearing
    uint8_t new_ciphertext[64];
    size_t new_ciphertext_len = sizeof(new_ciphertext);
    TEST_ASSERT_FALSE(test_security_context->encrypt(
        (uint8_t*)plaintext, strlen(plaintext), new_ciphertext, new_ciphertext_len));
}

// Test IV generation uniqueness
void test_iv_generation_uniqueness(void) {
    uint8_t iv1[16], iv2[16], iv3[16];
    
    TEST_ASSERT_TRUE(test_security_context->generateIV(iv1, sizeof(iv1)));
    TEST_ASSERT_TRUE(test_security_context->generateIV(iv2, sizeof(iv2)));
    TEST_ASSERT_TRUE(test_security_context->generateIV(iv3, sizeof(iv3)));
    
    // IVs should be different (extremely high probability)
    TEST_ASSERT_FALSE(memcmp(iv1, iv2, 16) == 0);
    TEST_ASSERT_FALSE(memcmp(iv2, iv3, 16) == 0);
    TEST_ASSERT_FALSE(memcmp(iv1, iv3, 16) == 0);
}

void setup() {
    delay(2000); // Allow time for initialization
    UNITY_BEGIN();
    
    RUN_TEST(test_encrypt_decrypt_basic);
    RUN_TEST(test_credential_encryption_decryption);
    RUN_TEST(test_encryption_invalid_inputs);
    RUN_TEST(test_credential_format_validation);
    RUN_TEST(test_key_derivation_consistency);
    RUN_TEST(test_secure_key_clearing);
    RUN_TEST(test_iv_generation_uniqueness);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}