#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include "../src/wifi/wifi_setup_portal.h"
#include "../src/wifi/wifi_connection_manager.h"
#include "../src/wifi/wifi_credential_manager.h"
#include "../src/actuators/status_led_controller.h"

// Test fixtures
WiFiConnectionManager* connection_manager = nullptr;
WiFiSetupPortal* setup_portal = nullptr;
StatusLedController* led_controller = nullptr;

const char* test_ssid = "TestNetwork";
const char* test_password = "TestPassword123";

void setUp(void) {
    // Initialize components for integration testing
    led_controller = StatusLedController::getInstance();
    connection_manager = new WiFiConnectionManager(*led_controller);
    setup_portal = new WiFiSetupPortal(connection_manager->getWebServer());
    
    // Start with clean state
    WiFiCredentialManager::clearCredentials();
    
    TEST_ASSERT_NOT_NULL(connection_manager);
    TEST_ASSERT_NOT_NULL(setup_portal);
}

void tearDown(void) {
    // Clean up test resources
    if (setup_portal) {
        setup_portal->stop();
        delete setup_portal;
        setup_portal = nullptr;
    }
    if (connection_manager) {
        connection_manager->stop();
        delete connection_manager;
        connection_manager = nullptr;
    }
    
    // Clear any stored credentials
    WiFiCredentialManager::clearCredentials();
}

void test_complete_fresh_device_setup_flow(void) {
    // Test the complete flow for a fresh device (no stored credentials)
    
    // 1. Device should start in setup mode when no credentials exist
    TEST_ASSERT_FALSE(WiFiCredentialManager::hasStoredCredentials());
    
    WiFiConnectionManager::ConnectionState state = connection_manager->begin();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, state);
    
    // 2. Setup portal should be active
    TEST_ASSERT_TRUE(setup_portal->isActive());
    TEST_ASSERT_TRUE(connection_manager->isAccessPointActive());
    
    // 3. LED should show setup mode indication
    TEST_ASSERT_EQUAL(StatusLedController::LedState::SETUP, led_controller->getCurrentState());
    
    // 4. Submit valid credentials through portal
    DynamicJsonDocument creds(512);
    creds["ssid"] = test_ssid;
    creds["password"] = test_password;
    
    String json_creds;
    serializeJson(creds, json_creds);
    
    WiFiSetupPortal::ConfigureResult result = setup_portal->submitCredentials(json_creds);
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::VALIDATING, result);
    
    // 5. Connection manager should attempt to connect
    state = connection_manager->processCredentials(test_ssid, test_password);
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    
    // 6. LED should show connecting state
    TEST_ASSERT_EQUAL(StatusLedController::LedState::CONNECTING, led_controller->getCurrentState());
    
    // 7. For testing, simulate successful connection
    connection_manager->simulateConnectionSuccess();
    
    // 8. Credentials should be stored encrypted
    TEST_ASSERT_TRUE(WiFiCredentialManager::hasStoredCredentials());
    
    // 9. Portal should be stopped
    TEST_ASSERT_FALSE(setup_portal->isActive());
    TEST_ASSERT_FALSE(connection_manager->isAccessPointActive());
    
    // 10. LED should show connected state
    TEST_ASSERT_EQUAL(StatusLedController::LedState::CONNECTED, led_controller->getCurrentState());
}

void test_setup_flow_with_invalid_credentials(void) {
    // Test handling of invalid WiFi credentials
    
    connection_manager->begin();
    setup_portal->start();
    
    // Submit credentials for non-existent network
    DynamicJsonDocument creds(512);
    creds["ssid"] = "NonExistentNetwork";
    creds["password"] = "WrongPassword";
    
    String json_creds;
    serializeJson(creds, json_creds);
    
    WiFiSetupPortal::ConfigureResult result = setup_portal->submitCredentials(json_creds);
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::VALIDATING, result);
    
    // Connection should fail
    WiFiConnectionManager::ConnectionState state = connection_manager->processCredentials("NonExistentNetwork", "WrongPassword");
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    
    // Simulate connection failure
    connection_manager->simulateConnectionFailure();
    
    // Should return to setup mode
    state = connection_manager->getCurrentState();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, state);
    
    // Portal should remain active for retry
    TEST_ASSERT_TRUE(setup_portal->isActive());
    
    // LED should show error state
    TEST_ASSERT_EQUAL(StatusLedController::LedState::ERROR, led_controller->getCurrentState());
    
    // No credentials should be stored
    TEST_ASSERT_FALSE(WiFiCredentialManager::hasStoredCredentials());
}

void test_boot_with_existing_valid_credentials(void) {
    // Pre-store valid credentials
    WiFiCredentialManager::storeCredentials(test_ssid, test_password);
    TEST_ASSERT_TRUE(WiFiCredentialManager::hasStoredCredentials());
    
    // Device boot should automatically connect
    WiFiConnectionManager::ConnectionState state = connection_manager->begin();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    
    // Setup portal should NOT be active
    TEST_ASSERT_FALSE(setup_portal->isActive());
    TEST_ASSERT_FALSE(connection_manager->isAccessPointActive());
    
    // LED should show connecting state
    TEST_ASSERT_EQUAL(StatusLedController::LedState::CONNECTING, led_controller->getCurrentState());
    
    // Simulate successful connection
    connection_manager->simulateConnectionSuccess();
    
    // Should reach connected state
    state = connection_manager->getCurrentState();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTED, state);
    
    // LED should show connected
    TEST_ASSERT_EQUAL(StatusLedController::LedState::CONNECTED, led_controller->getCurrentState());
}

void test_boot_with_existing_invalid_credentials(void) {
    // Pre-store credentials for network that no longer exists
    WiFiCredentialManager::storeCredentials("OldNetwork", "OldPassword");
    TEST_ASSERT_TRUE(WiFiCredentialManager::hasStoredCredentials());
    
    // Device boot should attempt connection
    WiFiConnectionManager::ConnectionState state = connection_manager->begin();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    
    // Simulate connection failure after retries
    for (int i = 0; i < 3; i++) {
        connection_manager->simulateConnectionFailure();
        state = connection_manager->getCurrentState();
        // Should keep trying for a few attempts
        if (i < 2) {
            TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
        }
    }
    
    // After max retries, should enter setup mode
    state = connection_manager->getCurrentState();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, state);
    
    // Setup portal should be active for re-configuration
    TEST_ASSERT_TRUE(setup_portal->isActive());
    TEST_ASSERT_TRUE(connection_manager->isAccessPointActive());
    
    // LED should show setup mode
    TEST_ASSERT_EQUAL(StatusLedController::LedState::SETUP, led_controller->getCurrentState());
}

void test_physical_reset_trigger_setup_mode(void) {
    // Start with stored credentials and connected state
    WiFiCredentialManager::storeCredentials(test_ssid, test_password);
    connection_manager->begin();
    connection_manager->simulateConnectionSuccess();
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTED, connection_manager->getCurrentState());
    TEST_ASSERT_FALSE(setup_portal->isActive());
    
    // Simulate physical reset button press (5-second hold)
    connection_manager->simulatePhysicalReset();
    
    // Should clear credentials and enter setup mode
    TEST_ASSERT_FALSE(WiFiCredentialManager::hasStoredCredentials());
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, connection_manager->getCurrentState());
    
    // Setup portal should be active
    TEST_ASSERT_TRUE(setup_portal->isActive());
    TEST_ASSERT_TRUE(connection_manager->isAccessPointActive());
    
    // LED should show setup mode
    TEST_ASSERT_EQUAL(StatusLedController::LedState::SETUP, led_controller->getCurrentState());
}

void test_setup_timeout_security(void) {
    // Start setup mode
    connection_manager->begin();
    setup_portal->start();
    
    TEST_ASSERT_TRUE(setup_portal->isActive());
    TEST_ASSERT_TRUE(connection_manager->isAccessPointActive());
    
    // Simulate 10 minute timeout
    setup_portal->simulateTimeout();
    
    // Portal should auto-stop for security
    TEST_ASSERT_FALSE(setup_portal->isActive());
    TEST_ASSERT_FALSE(connection_manager->isAccessPointActive());
    
    // Device should go to sleep/low power mode if no credentials
    WiFiConnectionManager::ConnectionState state = connection_manager->getCurrentState();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::FAILED, state);
}

void test_credential_update_via_reset(void) {
    // Start with old credentials
    WiFiCredentialManager::storeCredentials("OldNetwork", "OldPassword");
    connection_manager->begin();
    
    // Trigger physical reset
    connection_manager->simulatePhysicalReset();
    
    // Enter new credentials
    DynamicJsonDocument new_creds(512);
    new_creds["ssid"] = "NewNetwork";
    new_creds["password"] = "NewPassword123";
    
    String json_creds;
    serializeJson(new_creds, json_creds);
    
    setup_portal->submitCredentials(json_creds);
    connection_manager->processCredentials("NewNetwork", "NewPassword123");
    connection_manager->simulateConnectionSuccess();
    
    // Verify new credentials are stored
    TEST_ASSERT_TRUE(WiFiCredentialManager::hasStoredCredentials());
    
    // Verify old credentials are overwritten
    String stored_ssid, stored_password;
    TEST_ASSERT_TRUE(WiFiCredentialManager::getCredentials(stored_ssid, stored_password));
    TEST_ASSERT_EQUAL_STRING("NewNetwork", stored_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("NewPassword123", stored_password.c_str());
}

int main() {
    UNITY_BEGIN();
    
    // Complete flow tests
    RUN_TEST(test_complete_fresh_device_setup_flow);
    RUN_TEST(test_setup_flow_with_invalid_credentials);
    
    // Boot behavior tests
    RUN_TEST(test_boot_with_existing_valid_credentials);
    RUN_TEST(test_boot_with_existing_invalid_credentials);
    
    // Physical reset tests
    RUN_TEST(test_physical_reset_trigger_setup_mode);
    RUN_TEST(test_credential_update_via_reset);
    
    // Security and timeout tests
    RUN_TEST(test_setup_timeout_security);
    
    return UNITY_END();
}