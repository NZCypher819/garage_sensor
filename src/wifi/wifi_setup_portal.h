#pragma once

// WiFi Setup Portal Entity and Session Management
// Feature: 002-secure-wifi-storage
// User Story 1: Initial WiFi setup via web portal

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Forward declarations
class WiFiCredentialManager;
class WiFiConnectionManager;

/**
 * SetupSession entity - Manages temporary session data during WiFi credential setup
 * 
 * Handles the temporary state during the setup process including:
 * - Session lifecycle and timeout management
 * - Temporary credential validation
 * - Security limits and access control
 */
class SetupSession {
public:
    // Setup session states
    enum class SessionState {
        INACTIVE,           // No session active
        WAITING,           // Waiting for credential input
        VALIDATING,        // Testing submitted credentials
        COMPLETED,         // Successfully connected
        FAILED,           // Connection failed
        TIMEOUT          // Session timed out
    };
    
    // Session configuration
    static const unsigned long SESSION_TIMEOUT_MS = 10 * 60 * 1000; // 10 minutes
    static const int MAX_VALIDATION_ATTEMPTS = 5;
    static const int MAX_PORTAL_ACCESSES = 20;
    
    SetupSession();
    ~SetupSession();
    
    // Session lifecycle
    bool start();
    void stop();
    bool isActive() const;
    void setTimeout();
    
    // Credential management
    bool setTempCredentials(const String& ssid, const String& password);
    bool getTempCredentials(String& ssid, String& password) const;
    void clearTempCredentials();
    bool hasTempCredentials() const;
    
    // Validation state
    void setValidationInProgress(bool validating);
    bool isValidationInProgress() const;
    int incrementValidationAttempts();
    int getValidationAttempts() const;
    
    // Security and access control
    int incrementPortalAccess();
    int getPortalAccessCount() const;
    bool isSecurityLimitReached() const;
    
    // Session status
    SessionState getState() const;
    void setState(SessionState state);
    unsigned long getRemainingTimeMs() const;
    unsigned long getElapsedTimeMs() const;
    bool hasTimedOut() const;
    
    // JSON serialization for API responses
    String toJson() const;
    
    // Public members for easy access
    String session_id;
    String ip_address;
    
    // Legacy compatibility fields (TODO: refactor to use methods instead)
    bool active = false;
    unsigned long start_time = 0;
    unsigned long timeout_ms = SESSION_TIMEOUT_MS;
    bool credentials_submitted = false;
    
private:
    SessionState current_state;
    String temp_ssid;
    String temp_password;
    
    unsigned long session_start_time;
    unsigned long session_timeout;
    
    int validation_attempts;
    int portal_access_count;
    
public:
    bool validation_in_progress;  // Changed to public temporarily for setup_portal.cpp access
    
private:
    // Security helpers
    void secureWipeTempCredentials();
    bool validateSessionSecurity() const;
};

/**
 * WiFiSetupPortal - HTTP endpoints and web interface for WiFi credential setup
 * 
 * Provides the web server interface for User Story 1:
 * - Serves setup portal HTML/CSS/JS
 * - Handles credential submission API
 * - Manages setup session lifecycle
 * - Provides real-time status updates
 */
class WiFiSetupPortal {
public:
    // Portal configuration result codes
    enum class ConfigureResult {
        SUCCESS_CONFIGURE,    // Credentials accepted for validation
        INVALID_SSID,         // SSID validation failed
        INVALID_PASSWORD,     // Password validation failed
        NETWORK_NOT_FOUND,    // Network not detected in scan
        UNSUPPORTED_SECURITY, // Network security not supported
        CONNECTION_FAILED,    // Connection attempt failed
        ALREADY_CONFIGURED,   // Device already has credentials
        SESSION_INVALID,      // Setup session expired/invalid
        VALIDATION_TIMEOUT    // Credential validation timed out
    };

public:
    // Constructor and destructor
    WiFiSetupPortal();
    ~WiFiSetupPortal();
    
    // Initialize portal with dependencies
    static bool begin(AsyncWebServer* server, 
                     WiFiCredentialManager* credential_mgr,
                     WiFiConnectionManager* connection_mgr);
    static void stop();
    
    // Session management
    static bool startSession(const String& device_ip);
    static void endSession();
    static bool isSessionActive();
    static const SetupSession& getCurrentSession();
    const SetupSession& getSession() const;
    
    // Session state updates (for validation task)
    static void setValidationInProgress(bool in_progress);
    static void setCredentialsSubmitted(bool submitted);
    
    // Credential submission
    ConfigureResult submitCredentials(const String& json_request);
    ConfigureResult submitCredentials(const String& ssid, const String& password);
    
    // HTTP endpoint support
    String getSetupPage() const;
    String getSetupPageContentType() const;
    String getStatusJson() const;
    String getJsonContentType() const;
    String getCacheControlHeader() const;
    
    // Session timeout and security
    unsigned long getSessionTimeoutMs() const;
    void forceTimeout();
    bool hasCredentials() const;
    
    // Endpoint registration check (for testing)
    bool hasEndpoint(const String& path) const;
    
    // Testing helpers
    void simulateConnectionSuccess();
    void simulateConnectionFailure();
    void simulateTimeout() { forceTimeout(); }
    
private:
    // Static dependencies for class-level portal management
    static SetupSession current_session_;
    static AsyncWebServer* web_server_;
    static WiFiCredentialManager* credential_manager_;
    static WiFiConnectionManager* connection_manager_;
    
    // HTTP endpoint handlers (static for callback compatibility)
    static void setupRoutes();
    static void handlePortalPage(AsyncWebServerRequest* request);
    static void handleConfigureEndpoint(AsyncWebServerRequest* request);
    static void handleStatusEndpoint(AsyncWebServerRequest* request);
    
    // Request validation and processing (static)
    static ConfigureResult validateCredentialInput(const String& ssid, const String& password);
    static void validateCredentialsAsync(const String& ssid, const String& password);
    static String generateSessionId();
    static bool isValidSession();
    
    // Response helpers (static)
    static void sendErrorResponse(AsyncWebServerRequest* request, 
                                 ConfigureResult error_code, 
                                 const String& error_message);
    static void addSecurityHeaders(AsyncWebServerResponse* response);
    static String getConfigureResultMessage(ConfigureResult result);
    
    // Registered endpoints for testing
    mutable String registered_endpoints;
    void registerEndpoint(const String& path);
    
    // Configuration constants
    static const char* SETUP_AP_SSID;
    static const char* SETUP_HTML_FILE;
    static const char* SETUP_CSS_FILE;
    static const char* SETUP_JS_FILE;
};