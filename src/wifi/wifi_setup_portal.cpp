#include "wifi_setup_portal.h"
#include "../diagnostics/logger.h"
#include "wifi_connection_manager.h"
#include "../config/device_configuration.h"
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using Diagnostics::Logger;

// Task structure for credential validation
struct ValidationTaskParams {
    String ssid;
    String password;
    WiFiConnectionManager* connection_manager;
    WiFiCredentialManager* credential_manager;
};

// Task function for credential validation
void credentialValidationTask(void* parameters) {
    ValidationTaskParams* params = static_cast<ValidationTaskParams*>(parameters);
    
    Logger::info("WiFi Setup Portal: Validating credentials for SSID: " + params->ssid);
    
    bool validation_successful = false;
    if (params->connection_manager) {
        // First check if network exists and has supported security
        if (params->connection_manager->validateWiFiSecurity(params->ssid)) {
            // Attempt connection validation
            validation_successful = params->connection_manager->validateCredentials(params->ssid, params->password);
        }
    }
    
    // Update session state
    WiFiSetupPortal::setValidationInProgress(false);
    
    if (validation_successful) {
        Logger::info("WiFi Setup Portal: Credential validation successful");
        
        // Store validated credentials
        if (params->credential_manager) {
            params->credential_manager->storeCredentials(params->ssid, params->password);
        }
        
        // Exit setup mode after successful configuration
        if (params->connection_manager) {
            params->connection_manager->exitSetupMode();
        }
    } else {
        Logger::error("WiFi Setup Portal: Credential validation failed");
        
        // Reset session state for retry
        WiFiSetupPortal::setCredentialsSubmitted(false);
    }
    
    // Clean up
    delete params;
    vTaskDelete(NULL); // Delete this task
}

// SetupSession Implementation
SetupSession::SetupSession() 
    : current_state(SessionState::INACTIVE)
    , session_start_time(0)
    , session_timeout(SESSION_TIMEOUT_MS)
    , validation_in_progress(false)
    , validation_attempts(0)
    , portal_access_count(0) {
}

SetupSession::~SetupSession() {
    secureWipeTempCredentials();
}

bool SetupSession::start() {
    current_state = SessionState::WAITING;
    session_start_time = millis();
    validation_in_progress = false;
    validation_attempts = 0;
    return true;
}

void SetupSession::stop() {
    current_state = SessionState::INACTIVE;
    secureWipeTempCredentials();
}

bool SetupSession::isActive() const {
    return current_state != SessionState::INACTIVE;
}

SetupSession::SessionState SetupSession::getState() const {
    return current_state;
}

void SetupSession::setState(SessionState state) {
    current_state = state;
}

void SetupSession::secureWipeTempCredentials() {
    temp_ssid = "";
    temp_password = "";
}

// WiFiSetupPortal static variables
SetupSession WiFiSetupPortal::current_session_;
AsyncWebServer* WiFiSetupPortal::web_server_ = nullptr;
WiFiCredentialManager* WiFiSetupPortal::credential_manager_ = nullptr;
WiFiConnectionManager* WiFiSetupPortal::connection_manager_ = nullptr;

// Constructor
WiFiSetupPortal::WiFiSetupPortal() {
    // Static class - no instance construction needed
}

// Destructor
WiFiSetupPortal::~WiFiSetupPortal() {
    // Static class - cleanup handled by stop()
}

bool WiFiSetupPortal::begin(AsyncWebServer* server, 
                           WiFiCredentialManager* credential_mgr,
                           WiFiConnectionManager* connection_mgr) {
    if (!server || !credential_mgr || !connection_mgr) {
        Logger::error("WiFi Setup Portal: Invalid parameters for begin()");
        return false;
    }
    
    web_server_ = server;
    credential_manager_ = credential_mgr;
    connection_manager_ = connection_mgr;
    
    // Initialize SPIFFS for serving static files
    if (!SPIFFS.begin(true)) {
        Logger::error("WiFi Setup Portal: Failed to mount SPIFFS");
        return false;
    }
    
    setupRoutes();
    Logger::info("WiFi Setup Portal: Initialized successfully");
    return true;
}

void WiFiSetupPortal::stop() {
    if (current_session_.active) {
        current_session_.active = false;
        current_session_.ip_address = "";
        Logger::info("WiFi Setup Portal: Session terminated");
    }
}

bool WiFiSetupPortal::startSession(const String& device_ip) {
    if (current_session_.active) {
        Logger::warning("WiFi Setup Portal: Session already active");
        return false;
    }
    
    current_session_.session_id = generateSessionId();
    current_session_.start_time = millis();
    current_session_.timeout_ms = SETUP_SESSION_TIMEOUT_MS;
    current_session_.ip_address = device_ip;
    current_session_.active = true;
    current_session_.credentials_submitted = false;
    current_session_.validation_in_progress = false;
    
    Logger::info("WiFi Setup Portal: Session started with ID: " + current_session_.session_id);
    return true;
}

void WiFiSetupPortal::endSession() {
    if (!current_session_.active) {
        return;
    }
    
    current_session_.active = false;
    current_session_.session_id = "";
    current_session_.ip_address = "";
    current_session_.credentials_submitted = false;
    current_session_.validation_in_progress = false;
    
    Logger::info("WiFi Setup Portal: Session ended");
}

bool WiFiSetupPortal::isSessionActive() {
    if (!current_session_.active) {
        return false;
    }
    
    // Check for timeout
    unsigned long elapsed = millis() - current_session_.start_time;
    if (elapsed > current_session_.timeout_ms) {
        Logger::info("WiFi Setup Portal: Session timed out");
        endSession();
        return false;
    }
    
    return true;
}

void WiFiSetupPortal::setValidationInProgress(bool in_progress) {
    current_session_.validation_in_progress = in_progress;
}

void WiFiSetupPortal::setCredentialsSubmitted(bool submitted) {
    current_session_.credentials_submitted = submitted;
}

const SetupSession& WiFiSetupPortal::getCurrentSession() {
    return current_session_;
}

void WiFiSetupPortal::setupRoutes() {
    if (!web_server_) {
        return;
    }
    
    // Serve the main setup portal HTML page at both / and /setup
    auto handleSetupPage = [](AsyncWebServerRequest* request) {
        handlePortalPage(request);
    };
    
    web_server_->on("/", HTTP_GET, handleSetupPage);
    web_server_->on("/setup", HTTP_GET, handleSetupPage);
    
    // Serve CSS file
    web_server_->on("/setup_portal.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (SPIFFS.exists("/setup_portal.css")) {
            request->send(SPIFFS, "/setup_portal.css", "text/css");
        } else {
            request->send(404, "text/plain", "CSS not found");
        }
    });
    
    // Serve JavaScript file
    web_server_->on("/setup_portal.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (SPIFFS.exists("/setup_portal.js")) {
            request->send(SPIFFS, "/setup_portal.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "JavaScript not found");
        }
    });
    
    // Handle WiFi credential submission
    web_server_->on("/configure", HTTP_POST, [](AsyncWebServerRequest* request) {
        handleConfigureEndpoint(request);
    });
    
    // Provide setup status information
    web_server_->on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        handleStatusEndpoint(request);
    });
    
    // Handle all other requests with 404
    web_server_->onNotFound([](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(404, "text/html", 
            "<!DOCTYPE html><html><head><title>Not Found</title></head>"
            "<body><h1>404 - Page Not Found</h1>"
            "<p><a href='/'>Return to Setup Portal</a></p></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    });
    
    // Start the web server
    web_server_->begin();
    Logger::info("WiFi Setup Portal: Web server started");
}

void WiFiSetupPortal::handlePortalPage(AsyncWebServerRequest* request) {
    if (!isValidSession()) {
        sendErrorResponse(request, ConfigureResult::SESSION_INVALID, "No active setup session");
        return;
    }
    
    if (SPIFFS.exists("/setup_portal.html")) {
        AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/setup_portal.html", "text/html");
        addSecurityHeaders(response);
        request->send(response);
    } else {
        // Fallback minimal HTML if file not found
        String html = "<!DOCTYPE html><html><head><title>WiFi Setup</title></head>"
                     "<body><h1>WiFi Setup Portal</h1>"
                     "<form method='POST' action='/configure'>"
                     "<label>SSID: <input type='text' name='ssid' required></label><br>"
                     "<label>Password: <input type='password' name='password'></label><br>"
                     "<input type='submit' value='Connect'>"
                     "</form></body></html>";
        
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", html);
        addSecurityHeaders(response);
        request->send(response);
    }
}

void WiFiSetupPortal::handleConfigureEndpoint(AsyncWebServerRequest* request) {
    if (!isValidSession()) {
        sendErrorResponse(request, ConfigureResult::SESSION_INVALID, "No active setup session");
        return;
    }
    
    // Check if credentials already submitted
    if (current_session_.credentials_submitted) {
        sendErrorResponse(request, ConfigureResult::ALREADY_CONFIGURED, "Credentials already submitted for this session");
        return;
    }
    
    // Extract SSID and password from POST data
    String ssid = "";
    String password = "";
    
    if (request->hasParam("ssid", true)) {
        ssid = request->getParam("ssid", true)->value();
    }
    if (request->hasParam("password", true)) {
        password = request->getParam("password", true)->value();
    }
    
    // Validate input
    ConfigureResult result = validateCredentialInput(ssid, password);
    if (result != ConfigureResult::SUCCESS_CONFIGURE) {
        sendErrorResponse(request, result, getConfigureResultMessage(result));
        return;
    }
    
    // Mark credentials as submitted
    current_session_.credentials_submitted = true;
    current_session_.validation_in_progress = true;
    
    // Start asynchronous validation
    Logger::info("WiFi Setup Portal: Starting credential validation for SSID: " + ssid);
    
    // Send immediate response indicating validation started
    DynamicJsonDocument response(256);
    response["status"] = "validating";
    response["message"] = "Validating WiFi credentials...";
    response["session_id"] = current_session_.session_id;
    
    String responseBody;
    serializeJson(response, responseBody);
    
    AsyncWebServerResponse* jsonResponse = request->beginResponse(202, "application/json", responseBody);
    addSecurityHeaders(jsonResponse);
    request->send(jsonResponse);
    
    // Start background validation
    validateCredentialsAsync(ssid, password);
}

void WiFiSetupPortal::handleStatusEndpoint(AsyncWebServerRequest* request) {
    DynamicJsonDocument response(512);
    
    if (!isValidSession()) {
        response["status"] = "no_session";
        response["message"] = "No active setup session";
    } else {
        response["session_id"] = current_session_.session_id;
        response["session_active"] = true;
        response["credentials_submitted"] = current_session_.credentials_submitted;
        response["validation_in_progress"] = current_session_.validation_in_progress;
        
        if (current_session_.validation_in_progress) {
            response["status"] = "validating";
            response["message"] = "Validating WiFi credentials...";
        } else if (current_session_.credentials_submitted) {
            // Check connection status
            DeviceConfiguration::ConnectionState state = connection_manager_->getConnectionState();
            switch (state) {
                case DeviceConfiguration::ConnectionState::CONNECTED: {
                    response["status"] = "connected";
                    response["message"] = "Successfully connected to WiFi";
                    
                    // Add WiFi status details
                    String connected_ssid, ip_address;
                    int signal_strength;
                    if (connection_manager_->getWiFiStatus(connected_ssid, signal_strength, ip_address)) {
                        response["connected_ssid"] = connected_ssid;
                        response["signal_strength"] = signal_strength;
                        response["ip_address"] = ip_address;
                    }
                    break;
                }
                    
                case DeviceConfiguration::ConnectionState::CONNECTION_FAILED:
                    response["status"] = "failed";
                    response["message"] = "Failed to connect with provided credentials";
                    break;
                    
                default:
                    response["status"] = "connecting";
                    response["message"] = "Attempting to connect to WiFi...";
                    break;
            }
        } else {
            response["status"] = "waiting";
            response["message"] = "Waiting for WiFi credentials";
        }
        
        // Add session timing information
        unsigned long elapsed = millis() - current_session_.start_time;
        response["session_time_remaining"] = (current_session_.timeout_ms - elapsed) / 1000;
    }
    
    String responseBody;
    serializeJson(response, responseBody);
    
    AsyncWebServerResponse* jsonResponse = request->beginResponse(200, "application/json", responseBody);
    addSecurityHeaders(jsonResponse);
    request->send(jsonResponse);
}

void WiFiSetupPortal::validateCredentialsAsync(const String& ssid, const String& password) {
    // Create task parameters (will be deleted by the task)
    ValidationTaskParams* params = new ValidationTaskParams();
    params->ssid = ssid;
    params->password = password;
    params->connection_manager = connection_manager_;
    params->credential_manager = credential_manager_;
    
    // Create FreeRTOS task for validation (runs on separate core/thread)
    xTaskCreate(
        credentialValidationTask,      // Task function
        "WiFiValidation",              // Task name
        8192,                          // Stack size (8KB)
        params,                        // Parameters
        1,                             // Priority
        NULL                           // Task handle (not needed)
    );
}

WiFiSetupPortal::ConfigureResult WiFiSetupPortal::validateCredentialInput(const String& ssid, const String& password) {
    // Check SSID length
    if (ssid.length() == 0) {
        return ConfigureResult::INVALID_SSID;
    }
    
    if (ssid.length() > 32) { // IEEE 802.11 SSID maximum length
        return ConfigureResult::INVALID_SSID;
    }
    
    // Check password length for WPA2/WPA3
    if (password.length() > 0 && password.length() < 8) {
        return ConfigureResult::INVALID_PASSWORD;
    }
    
    if (password.length() > 63) { // WPA2/WPA3 passphrase maximum length
        return ConfigureResult::INVALID_PASSWORD;
    }
    
    // Check for valid characters (basic validation)
    for (unsigned int i = 0; i < ssid.length(); i++) {
        char c = ssid[i];
        if (c < 32 || c > 126) { // Printable ASCII range
            return ConfigureResult::INVALID_SSID;
        }
    }
    
    return ConfigureResult::SUCCESS_CONFIGURE;
}

String WiFiSetupPortal::generateSessionId() {
    // Generate a simple session ID using device MAC and timestamp
    uint8_t mac[6];
    WiFi.macAddress(mac);
    unsigned long timestamp = millis();
    
    char session_id[17];
    snprintf(session_id, sizeof(session_id), "%02X%02X%02X%08lX", 
             mac[3], mac[4], mac[5], timestamp);
    
    return String(session_id);
}

bool WiFiSetupPortal::isValidSession() {
    return current_session_.active && isSessionActive();
}

void WiFiSetupPortal::sendErrorResponse(AsyncWebServerRequest* request, 
                                       ConfigureResult error_code, 
                                       const String& error_message) {
    DynamicJsonDocument response(256);
    response["status"] = "error";
    response["error_code"] = static_cast<int>(error_code);
    response["message"] = error_message;
    
    String responseBody;
    serializeJson(response, responseBody);
    
    int http_status = 400; // Bad Request
    if (error_code == ConfigureResult::SESSION_INVALID) {
        http_status = 401; // Unauthorized
    }
    
    AsyncWebServerResponse* jsonResponse = request->beginResponse(http_status, "application/json", responseBody);
    addSecurityHeaders(jsonResponse);
    request->send(jsonResponse);
    
    Logger::warning("WiFi Setup Portal: Error response sent - " + error_message);
}

void WiFiSetupPortal::addSecurityHeaders(AsyncWebServerResponse* response) {
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "DENY");
}

String WiFiSetupPortal::getConfigureResultMessage(ConfigureResult result) {
    switch (result) {
        case ConfigureResult::SUCCESS_CONFIGURE:
            return "Configuration successful";
        case ConfigureResult::INVALID_SSID:
            return "Invalid SSID - must be 1-32 characters with printable ASCII";
        case ConfigureResult::INVALID_PASSWORD:
            return "Invalid password - must be 8-63 characters for secured networks";
        case ConfigureResult::NETWORK_NOT_FOUND:
            return "WiFi network not found";
        case ConfigureResult::UNSUPPORTED_SECURITY:
            return "Unsupported security protocol - requires WPA2 or WPA3";
        case ConfigureResult::CONNECTION_FAILED:
            return "Failed to connect to network - check credentials";
        case ConfigureResult::ALREADY_CONFIGURED:
            return "Device already configured for this session";
        case ConfigureResult::SESSION_INVALID:
            return "Invalid or expired setup session";
        case ConfigureResult::VALIDATION_TIMEOUT:
            return "Credential validation timed out";
        default:
            return "Unknown error occurred";
    }
}