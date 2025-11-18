#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include "../src/wifi/wifi_connection_manager.h"
#include "../src/actuators/status_led_controller.h"

// Test fixture
WiFiConnectionManager* connection_manager = nullptr;
StatusLedController* led_controller = nullptr;

void setUp(void) {
    led_controller = StatusLedController::getInstance();
    connection_manager = new WiFiConnectionManager(*led_controller);
    
    TEST_ASSERT_NOT_NULL(connection_manager);
    TEST_ASSERT_NOT_NULL(led_controller);
}

void tearDown(void) {
    if (connection_manager) {
        connection_manager->stop();
        delete connection_manager;
        connection_manager = nullptr;
    }
}

void test_initial_state_transitions(void) {
    // Initial state should be DISCONNECTED
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::DISCONNECTED, 
                     connection_manager->getCurrentState());
    
    // Begin should transition to appropriate state based on credentials
    WiFiConnectionManager::ConnectionState state = connection_manager->begin();
    
    // With no stored credentials, should go to SETUP_MODE
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, state);
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
}

void test_setup_mode_to_connecting_transition(void) {
    // Start in setup mode
    connection_manager->begin();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
    
    // Process new credentials should transition to CONNECTING
    WiFiConnectionManager::ConnectionState state = 
        connection_manager->processCredentials("TestNetwork", "TestPassword");
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, 
                     connection_manager->getCurrentState());
}

void test_connecting_to_connected_transition(void) {
    // Start connecting
    connection_manager->begin();
    connection_manager->processCredentials("TestNetwork", "TestPassword");
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, 
                     connection_manager->getCurrentState());
    
    // Simulate successful connection
    connection_manager->simulateConnectionSuccess();
    
    // Should transition to CONNECTED
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTED, 
                     connection_manager->getCurrentState());
}

void test_connecting_to_failed_transition(void) {
    // Start connecting
    connection_manager->begin();
    connection_manager->processCredentials("BadNetwork", "BadPassword");
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, 
                     connection_manager->getCurrentState());
    
    // Simulate connection failure
    connection_manager->simulateConnectionFailure();
    
    // Should transition to FAILED
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::FAILED, 
                     connection_manager->getCurrentState());
}

void test_failed_to_setup_mode_transition(void) {
    // Reach failed state
    connection_manager->begin();
    connection_manager->processCredentials("BadNetwork", "BadPassword");
    connection_manager->simulateConnectionFailure();
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::FAILED, 
                     connection_manager->getCurrentState());
    
    // Should automatically transition to setup mode for retry
    connection_manager->handleFailedConnection();
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
}

void test_connected_to_disconnected_on_lost_connection(void) {
    // Reach connected state
    connection_manager->begin();
    connection_manager->processCredentials("TestNetwork", "TestPassword");
    connection_manager->simulateConnectionSuccess();
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTED, 
                     connection_manager->getCurrentState());
    
    // Simulate lost connection
    connection_manager->simulateConnectionLost();
    
    // Should transition to DISCONNECTED
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::DISCONNECTED, 
                     connection_manager->getCurrentState());
}

void test_automatic_reconnection_from_disconnected(void) {
    // Start from disconnected with stored credentials
    connection_manager->setStoredCredentials("TestNetwork", "TestPassword");
    
    WiFiConnectionManager::ConnectionState state = connection_manager->begin();
    
    // Should automatically try to reconnect
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
}

void test_retry_logic_on_connection_failure(void) {
    connection_manager->begin();
    connection_manager->processCredentials("BadNetwork", "BadPassword");
    
    // First failure should retry
    connection_manager->simulateConnectionFailure();
    TEST_ASSERT_EQUAL(0, connection_manager->getRetryCount()); // First attempt
    
    // Should still be trying to connect
    WiFiConnectionManager::ConnectionState state = connection_manager->retryConnection();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    TEST_ASSERT_EQUAL(1, connection_manager->getRetryCount());
    
    // Second failure
    connection_manager->simulateConnectionFailure();
    state = connection_manager->retryConnection();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, state);
    TEST_ASSERT_EQUAL(2, connection_manager->getRetryCount());
    
    // Third failure should give up and go to setup mode
    connection_manager->simulateConnectionFailure();
    state = connection_manager->retryConnection();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, state);
    TEST_ASSERT_EQUAL(0, connection_manager->getRetryCount()); // Reset after giving up
}

void test_physical_reset_from_any_state(void) {
    // Test from CONNECTED state
    connection_manager->begin();
    connection_manager->processCredentials("TestNetwork", "TestPassword");
    connection_manager->simulateConnectionSuccess();
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTED, 
                     connection_manager->getCurrentState());
    
    // Physical reset should go to setup mode
    connection_manager->handlePhysicalReset();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
    
    // Test from CONNECTING state
    connection_manager->processCredentials("TestNetwork", "TestPassword");
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, 
                     connection_manager->getCurrentState());
    
    connection_manager->handlePhysicalReset();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
}

void test_setup_timeout_transition(void) {
    // Start in setup mode
    connection_manager->begin();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
    
    // Simulate setup timeout (10 minutes)
    connection_manager->simulateSetupTimeout();
    
    // Should transition to FAILED for power saving
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::FAILED, 
                     connection_manager->getCurrentState());
}

void test_led_state_synchronization(void) {
    // Setup mode should set SETUP LED
    connection_manager->begin();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
    TEST_ASSERT_EQUAL(StatusLedController::LedState::SETUP, 
                     led_controller->getCurrentState());
    
    // Connecting should set CONNECTING LED
    connection_manager->processCredentials("TestNetwork", "TestPassword");
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, 
                     connection_manager->getCurrentState());
    TEST_ASSERT_EQUAL(StatusLedController::LedState::CONNECTING, 
                     led_controller->getCurrentState());
    
    // Connected should set CONNECTED LED
    connection_manager->simulateConnectionSuccess();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTED, 
                     connection_manager->getCurrentState());
    TEST_ASSERT_EQUAL(StatusLedController::LedState::CONNECTED, 
                     led_controller->getCurrentState());
    
    // Failed should set ERROR LED
    connection_manager->simulateConnectionLost();
    connection_manager->simulateConnectionFailure();
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::FAILED, 
                     connection_manager->getCurrentState());
    TEST_ASSERT_EQUAL(StatusLedController::LedState::ERROR, 
                     led_controller->getCurrentState());
}

void test_connection_timeout_handling(void) {
    // Start connecting
    connection_manager->begin();
    connection_manager->processCredentials("TestNetwork", "TestPassword");
    
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::CONNECTING, 
                     connection_manager->getCurrentState());
    
    // Simulate connection attempt timeout (30 seconds)
    connection_manager->simulateConnectionTimeout();
    
    // Should transition to FAILED and then retry
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::FAILED, 
                     connection_manager->getCurrentState());
}

void test_invalid_state_transitions_blocked(void) {
    // Cannot go directly from DISCONNECTED to CONNECTED
    connection_manager->begin(); // Goes to SETUP_MODE
    
    // Try to force invalid transition
    bool success = connection_manager->forceStateTransition(
        WiFiConnectionManager::ConnectionState::CONNECTED);
    
    // Should reject invalid transition
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(WiFiConnectionManager::ConnectionState::SETUP_MODE, 
                     connection_manager->getCurrentState());
}

int main() {
    UNITY_BEGIN();
    
    // Basic state transitions
    RUN_TEST(test_initial_state_transitions);
    RUN_TEST(test_setup_mode_to_connecting_transition);
    RUN_TEST(test_connecting_to_connected_transition);
    RUN_TEST(test_connecting_to_failed_transition);
    RUN_TEST(test_failed_to_setup_mode_transition);
    
    // Connection management
    RUN_TEST(test_connected_to_disconnected_on_lost_connection);
    RUN_TEST(test_automatic_reconnection_from_disconnected);
    RUN_TEST(test_retry_logic_on_connection_failure);
    
    // Special transitions
    RUN_TEST(test_physical_reset_from_any_state);
    RUN_TEST(test_setup_timeout_transition);
    RUN_TEST(test_connection_timeout_handling);
    
    // Integration tests
    RUN_TEST(test_led_state_synchronization);
    RUN_TEST(test_invalid_state_transitions_blocked);
    
    return UNITY_END();
}