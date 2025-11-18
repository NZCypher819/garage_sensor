#include <unity.h>
#include <Arduino.h>
#include <Preferences.h>
#include "../src/wifi/wifi_credential_manager.h"
#include "../src/config/wifi_config.h"

// Test fixtures
WiFiCredentialManager* test_manager = nullptr;
Preferences test_prefs;
const char* test_namespace = "test_wifi";

void setUp(void) {
    // Clear any existing test data
    test_prefs.begin(test_namespace, false);
    test_prefs.clear();
    test_prefs.end();
    
    // Create fresh credential manager for testing
    test_manager = new WiFiCredentialManager();
    TEST_ASSERT_TRUE(test_manager->begin());
}

void tearDown(void) {
    // Clean up after each test
    if (test_manager) {
        test_manager->clearCredentials();
        delete test_manager;
        test_manager = nullptr;
    }
    
    // Clear test data
    test_prefs.begin(test_namespace, false);
    test_prefs.clear();
    test_prefs.end();
}

// Test basic credential storage and retrieval
void test_store_and_load_credentials(void) {
    const String test_ssid = "TestNetwork";
    const String test_password = "TestPassword123";
    
    // Initially should have no credentials
    TEST_ASSERT_FALSE(test_manager->hasValidCredentials());
    
    // Store credentials
    bool store_success = test_manager->storeCredentials(test_ssid, test_password);
    TEST_ASSERT_TRUE(store_success);
    
    // Should now have valid credentials
    TEST_ASSERT_TRUE(test_manager->hasValidCredentials());
    
    // Load credentials
    String loaded_ssid, loaded_password;
    bool load_success = test_manager->loadCredentials(loaded_ssid, loaded_password);
    TEST_ASSERT_TRUE(load_success);
    
    // Verify credentials match
    TEST_ASSERT_EQUAL_STRING(test_ssid.c_str(), loaded_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(test_password.c_str(), loaded_password.c_str());
}

// Test credential persistence across restarts
void test_credential_persistence(void) {
    const String test_ssid = "PersistentNetwork";
    const String test_password = "PersistentPass";
    
    // Store credentials in first manager instance
    TEST_ASSERT_TRUE(test_manager->storeCredentials(test_ssid, test_password));
    delete test_manager;
    test_manager = nullptr;
    
    // Create new manager instance (simulates restart)
    test_manager = new WiFiCredentialManager();
    TEST_ASSERT_TRUE(test_manager->begin());
    
    // Should still have valid credentials
    TEST_ASSERT_TRUE(test_manager->hasValidCredentials());
    
    // Load and verify credentials
    String loaded_ssid, loaded_password;
    TEST_ASSERT_TRUE(test_manager->loadCredentials(loaded_ssid, loaded_password));
    TEST_ASSERT_EQUAL_STRING(test_ssid.c_str(), loaded_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(test_password.c_str(), loaded_password.c_str());
}

// Test credential clearing
void test_clear_credentials(void) {
    const String test_ssid = "ClearTest";
    const String test_password = "ClearPass";
    
    // Store credentials
    TEST_ASSERT_TRUE(test_manager->storeCredentials(test_ssid, test_password));
    TEST_ASSERT_TRUE(test_manager->hasValidCredentials());
    
    // Clear credentials
    test_manager->clearCredentials();
    TEST_ASSERT_FALSE(test_manager->hasValidCredentials());
    
    // Attempt to load should fail
    String loaded_ssid, loaded_password;
    TEST_ASSERT_FALSE(test_manager->loadCredentials(loaded_ssid, loaded_password));
    
    // Device should be in setup mode
    const auto& config = test_manager->getDeviceConfig();
    TEST_ASSERT_TRUE(config.setup_mode);
}

// Test invalid credential format rejection
void test_invalid_credential_formats(void) {
    // Empty SSID
    TEST_ASSERT_FALSE(test_manager->storeCredentials("", "password"));
    
    // Empty password  
    TEST_ASSERT_FALSE(test_manager->storeCredentials("ssid", ""));
    
    // SSID too long
    String long_ssid = "";
    for (int i = 0; i < 35; i++) long_ssid += "a";
    TEST_ASSERT_FALSE(test_manager->storeCredentials(long_ssid, "password"));
    
    // Password too long
    String long_password = "";
    for (int i = 0; i < 65; i++) long_password += "a";
    TEST_ASSERT_FALSE(test_manager->storeCredentials("ssid", long_password));
    
    // Should still have no valid credentials
    TEST_ASSERT_FALSE(test_manager->hasValidCredentials());
}

// Test device configuration management
void test_device_configuration(void) {
    // Check initial configuration
    const auto& initial_config = test_manager->getDeviceConfig();
    TEST_ASSERT_FALSE(initial_config.device_id.isEmpty());
    TEST_ASSERT_TRUE(initial_config.setup_mode); // Should start in setup mode
    
    // Update configuration
    DeviceConfiguration new_config = initial_config;
    new_config.setup_mode = false;
    new_config.connection_state = DeviceConfiguration::CONNECTED;
    new_config.led_state = DeviceConfiguration::LED_CONNECTED;
    
    test_manager->updateDeviceConfig(new_config);
    
    // Verify update
    const auto& updated_config = test_manager->getDeviceConfig();
    TEST_ASSERT_FALSE(updated_config.setup_mode);
    TEST_ASSERT_EQUAL(DeviceConfiguration::CONNECTED, updated_config.connection_state);
    TEST_ASSERT_EQUAL(DeviceConfiguration::LED_CONNECTED, updated_config.led_state);
    
    // Save and reload configuration
    TEST_ASSERT_TRUE(test_manager->saveDeviceConfig());
    TEST_ASSERT_TRUE(test_manager->loadDeviceConfig());
    
    // Verify persistence
    const auto& reloaded_config = test_manager->getDeviceConfig();
    TEST_ASSERT_FALSE(reloaded_config.setup_mode);
    TEST_ASSERT_EQUAL(DeviceConfiguration::CONNECTED, reloaded_config.connection_state);
}

// Test storage size reporting
void test_storage_size(void) {
    // Initially no storage used
    TEST_ASSERT_EQUAL(0, test_manager->getStorageSize());
    
    // Store credentials
    const String test_ssid = "SizeTest";
    const String test_password = "SizePass";
    TEST_ASSERT_TRUE(test_manager->storeCredentials(test_ssid, test_password));
    
    // Should now report storage size
    size_t storage_size = test_manager->getStorageSize();
    TEST_ASSERT_GREATER_THAN(0, storage_size);
    TEST_ASSERT_EQUAL(sizeof(WiFiCredentials), storage_size);
    
    // Clear credentials
    test_manager->clearCredentials();
    TEST_ASSERT_EQUAL(0, test_manager->getStorageSize());
}

// Test credential overwrite
void test_credential_overwrite(void) {
    // Store initial credentials
    const String ssid1 = "Network1";
    const String password1 = "Password1";
    TEST_ASSERT_TRUE(test_manager->storeCredentials(ssid1, password1));
    
    // Store new credentials (should overwrite)
    const String ssid2 = "Network2"; 
    const String password2 = "Password2";
    TEST_ASSERT_TRUE(test_manager->storeCredentials(ssid2, password2));
    
    // Load and verify only new credentials exist
    String loaded_ssid, loaded_password;
    TEST_ASSERT_TRUE(test_manager->loadCredentials(loaded_ssid, loaded_password));
    TEST_ASSERT_EQUAL_STRING(ssid2.c_str(), loaded_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(password2.c_str(), loaded_password.c_str());
    
    // Should not match old credentials
    TEST_ASSERT_NOT_EQUAL_STRING(ssid1.c_str(), loaded_ssid.c_str());
    TEST_ASSERT_NOT_EQUAL_STRING(password1.c_str(), loaded_password.c_str());
}

// Test encryption validation
void test_encryption_validation(void) {
    // Test manager should pass encryption validation
    TEST_ASSERT_TRUE(test_manager->validateEncryption());
    
    // Test with uninitialized manager
    WiFiCredentialManager uninitialized_manager;
    TEST_ASSERT_FALSE(uninitialized_manager.validateEncryption());
}

// Test device ID generation
void test_device_id_generation(void) {
    String device_id = test_manager->getDeviceId();
    
    // Device ID should not be empty
    TEST_ASSERT_FALSE(device_id.isEmpty());
    
    // Should be consistent across calls
    String device_id2 = test_manager->getDeviceId();
    TEST_ASSERT_EQUAL_STRING(device_id.c_str(), device_id2.c_str());
    
    // Should be consistent across manager instances
    WiFiCredentialManager manager2;
    TEST_ASSERT_TRUE(manager2.begin());
    String device_id3 = manager2.getDeviceId();
    TEST_ASSERT_EQUAL_STRING(device_id.c_str(), device_id3.c_str());
}

// Test maximum credential lengths
void test_maximum_credential_lengths(void) {
    // Test maximum valid SSID length (32 characters)
    String max_ssid = "";
    for (int i = 0; i < 32; i++) max_ssid += "a";
    
    // Test maximum valid password length (63 characters)
    String max_password = "";
    for (int i = 0; i < 63; i++) max_password += "b";
    
    // Should successfully store maximum length credentials
    TEST_ASSERT_TRUE(test_manager->storeCredentials(max_ssid, max_password));
    
    // Should successfully load them back
    String loaded_ssid, loaded_password;
    TEST_ASSERT_TRUE(test_manager->loadCredentials(loaded_ssid, loaded_password));
    TEST_ASSERT_EQUAL_STRING(max_ssid.c_str(), loaded_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(max_password.c_str(), loaded_password.c_str());
}

void setup() {
    delay(2000); // Allow time for initialization
    UNITY_BEGIN();
    
    RUN_TEST(test_store_and_load_credentials);
    RUN_TEST(test_credential_persistence);
    RUN_TEST(test_clear_credentials);
    RUN_TEST(test_invalid_credential_formats);
    RUN_TEST(test_device_configuration);
    RUN_TEST(test_storage_size);
    RUN_TEST(test_credential_overwrite);
    RUN_TEST(test_encryption_validation);
    RUN_TEST(test_device_id_generation);
    RUN_TEST(test_maximum_credential_lengths);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}