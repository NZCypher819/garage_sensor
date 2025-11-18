#include <unity.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "../src/wifi/wifi_setup_portal.h"
#include "../src/wifi/wifi_credential_manager.h"

// Test fixture data
AsyncWebServer* test_server = nullptr;
WiFiSetupPortal* test_portal = nullptr;
const char* test_ssid = "TestNetwork";
const char* test_password = "TestPassword123";

void setUp(void) {
    // Initialize test server on port 8080 for testing
    test_server = new AsyncWebServer(8080);
    test_portal = new WiFiSetupPortal(*test_server);
    
    // Ensure portal is not active initially
    TEST_ASSERT_FALSE(test_portal->isActive());
}

void tearDown(void) {
    // Clean up test resources
    if (test_portal) {
        test_portal->stop();
        delete test_portal;
        test_portal = nullptr;
    }
    if (test_server) {
        test_server->end();
        delete test_server;  
        test_server = nullptr;
    }
}

void test_setup_portal_initialization(void) {
    TEST_ASSERT_NOT_NULL(test_portal);
    TEST_ASSERT_FALSE(test_portal->isActive());
    TEST_ASSERT_FALSE(test_portal->hasCredentials());
}

void test_setup_portal_start_creates_endpoints(void) {
    // Start the portal
    bool success = test_portal->start();
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(test_portal->isActive());
    
    // Verify endpoints are registered (implementation should provide method to check)
    TEST_ASSERT_TRUE(test_portal->hasEndpoint("/setup"));
    TEST_ASSERT_TRUE(test_portal->hasEndpoint("/configure"));
    TEST_ASSERT_TRUE(test_portal->hasEndpoint("/status"));
}

void test_get_setup_returns_html_form(void) {
    test_portal->start();
    
    // Simulate GET /setup request
    String html_response = test_portal->getSetupPage();
    
    TEST_ASSERT_TRUE(html_response.length() > 0);
    TEST_ASSERT_TRUE(html_response.indexOf("<form") >= 0);
    TEST_ASSERT_TRUE(html_response.indexOf("ssid") >= 0);
    TEST_ASSERT_TRUE(html_response.indexOf("password") >= 0);
    TEST_ASSERT_TRUE(html_response.indexOf("action=\"/configure\"") >= 0);
}

void test_post_configure_accepts_valid_credentials(void) {
    test_portal->start();
    
    // Create test credential data
    DynamicJsonDocument request_data(512);
    request_data["ssid"] = test_ssid;
    request_data["password"] = test_password;
    
    String json_request;
    serializeJson(request_data, json_request);
    
    // Submit credentials
    WiFiSetupPortal::ConfigureResult result = test_portal->submitCredentials(json_request);
    
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::VALIDATING, result);
    TEST_ASSERT_TRUE(test_portal->hasCredentials());
}

void test_post_configure_rejects_invalid_credentials(void) {
    test_portal->start();
    
    // Test empty SSID
    DynamicJsonDocument request_data(512);
    request_data["ssid"] = "";
    request_data["password"] = test_password;
    
    String json_request;
    serializeJson(request_data, json_request);
    
    WiFiSetupPortal::ConfigureResult result = test_portal->submitCredentials(json_request);
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::INVALID_INPUT, result);
    
    // Test SSID too long (>32 chars)
    request_data["ssid"] = "ThisSSIDIsDefinitelyTooLongForWiFiStandards";
    serializeJson(request_data, json_request);
    
    result = test_portal->submitCredentials(json_request);
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::INVALID_INPUT, result);
    
    // Test password too short
    request_data["ssid"] = test_ssid;
    request_data["password"] = "";
    serializeJson(request_data, json_request);
    
    result = test_portal->submitCredentials(json_request);
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::INVALID_INPUT, result);
}

void test_get_status_returns_current_state(void) {
    test_portal->start();
    
    // Initially should show waiting for credentials
    String status_response = test_portal->getStatusJson();
    
    DynamicJsonDocument status_doc(512);
    deserializeJson(status_doc, status_response);
    
    TEST_ASSERT_EQUAL_STRING("waiting_for_credentials", status_doc["state"]);
    TEST_ASSERT_FALSE(status_doc["has_credentials"]);
}

void test_setup_session_timeout_security(void) {
    test_portal->start();
    
    // Portal should have timeout limit
    unsigned long timeout_ms = test_portal->getSessionTimeoutMs();
    TEST_ASSERT_EQUAL_UINT32(10 * 60 * 1000, timeout_ms); // 10 minutes
    
    // After timeout, portal should auto-stop
    TEST_ASSERT_TRUE(test_portal->isActive());
    
    // Simulate timeout
    test_portal->forceTimeout();
    TEST_ASSERT_FALSE(test_portal->isActive());
}

void test_portal_security_stops_after_max_attempts(void) {
    test_portal->start();
    
    // Try invalid credentials multiple times
    DynamicJsonDocument request_data(512);
    request_data["ssid"] = "invalid";
    request_data["password"] = "invalid";
    
    String json_request;
    serializeJson(request_data, json_request);
    
    // Should allow first few attempts
    for (int i = 0; i < 5; i++) {
        WiFiSetupPortal::ConfigureResult result = test_portal->submitCredentials(json_request);
        TEST_ASSERT_NOT_EQUAL(WiFiSetupPortal::ConfigureResult::TOO_MANY_ATTEMPTS, result);
    }
    
    // Should block after too many attempts
    WiFiSetupPortal::ConfigureResult result = test_portal->submitCredentials(json_request);
    TEST_ASSERT_EQUAL(WiFiSetupPortal::ConfigureResult::TOO_MANY_ATTEMPTS, result);
    TEST_ASSERT_FALSE(test_portal->isActive());
}

void test_content_type_headers_security(void) {
    test_portal->start();
    
    // HTML page should have proper content type
    String content_type = test_portal->getSetupPageContentType();
    TEST_ASSERT_EQUAL_STRING("text/html; charset=utf-8", content_type.c_str());
    
    // JSON responses should have proper content type
    String json_content_type = test_portal->getJsonContentType();
    TEST_ASSERT_EQUAL_STRING("application/json", json_content_type.c_str());
}

void test_cache_control_headers(void) {
    test_portal->start();
    
    // Setup page should not be cached for security
    String cache_control = test_portal->getCacheControlHeader();
    TEST_ASSERT_EQUAL_STRING("no-cache, no-store, must-revalidate", cache_control.c_str());
}

int main() {
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_setup_portal_initialization);
    RUN_TEST(test_setup_portal_start_creates_endpoints);
    
    // HTTP endpoint tests
    RUN_TEST(test_get_setup_returns_html_form);
    RUN_TEST(test_post_configure_accepts_valid_credentials);
    RUN_TEST(test_post_configure_rejects_invalid_credentials);
    RUN_TEST(test_get_status_returns_current_state);
    
    // Security tests
    RUN_TEST(test_setup_session_timeout_security);
    RUN_TEST(test_portal_security_stops_after_max_attempts);
    RUN_TEST(test_content_type_headers_security);
    RUN_TEST(test_cache_control_headers);
    
    return UNITY_END();
}