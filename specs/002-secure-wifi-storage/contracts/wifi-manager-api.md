# Internal WiFi Manager API Contract

**Feature**: 002-secure-wifi-storage  
**Language**: C++ (Arduino framework)  
**Target**: ESP32-S3

## Overview

Internal C++ API contracts for WiFi credential management components. These classes provide the core functionality for secure credential storage, connection management, and setup portal operation.

## Class Contracts

### WiFiCredentialManager

Handles secure storage and retrieval of WiFi credentials.

```cpp
#include <Preferences.h>
#include <WiFi.h>

class WiFiCredentialManager {
public:
    // Constructor
    WiFiCredentialManager();
    
    // Initialize credential manager and encryption context
    bool begin();
    
    // Store credentials securely (encrypts before storage)
    // Returns true on success, false on encryption/storage failure
    bool storeCredentials(const String& ssid, const String& password);
    
    // Load and decrypt stored credentials
    // Returns true if credentials exist and decrypt successfully
    bool loadCredentials(String& ssid, String& password);
    
    // Check if valid credentials exist in storage
    bool hasValidCredentials();
    
    // Securely wipe all stored credential data
    void clearCredentials();
    
    // Validate encryption/decryption is working correctly
    bool validateEncryption();
    
    // Get storage statistics (for diagnostics)
    size_t getStorageSize();
    
private:
    Preferences preferences;
    uint8_t encryptionKey[32];  // AES-256 key
    bool initialized;
    
    // Internal encryption/decryption methods
    bool encryptCredentials(const String& ssid, const String& password, uint8_t* encData, size_t& encLength);
    bool decryptCredentials(const uint8_t* encData, size_t encLength, String& ssid, String& password);
    void deriveEncryptionKey();
};
```

**Usage Pattern:**
```cpp
WiFiCredentialManager credManager;
credManager.begin();

// Store new credentials
if (credManager.storeCredentials("MyWiFi", "password123")) {
    Serial.println("Credentials stored successfully");
}

// Load existing credentials  
String ssid, password;
if (credManager.loadCredentials(ssid, password)) {
    WiFi.begin(ssid.c_str(), password.c_str());
}
```

### WiFiConnectionManager

Manages WiFi connection state and reconnection logic.

```cpp
#include <WiFi.h>

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING, 
    CONNECTED,
    FAILED
};

class WiFiConnectionManager {
public:
    WiFiConnectionManager();
    
    // Initialize connection manager
    bool begin();
    
    // Attempt connection to stored network
    bool connectToStoredNetwork();
    
    // Check current connection status  
    bool isConnected();
    
    // Get detailed connection state
    ConnectionState getConnectionState();
    
    // Get signal strength (if connected)
    int32_t getSignalStrength();
    
    // Start automatic reconnection attempts with exponential backoff
    void startReconnectionAttempts();
    
    // Stop reconnection attempts and enter setup mode
    void enterSetupMode();
    
    // Handle connection events (call from main loop)
    void handleConnectionEvents();
    
    // Get connection statistics
    struct ConnectionStats {
        uint32_t totalConnections;
        uint32_t failedAttempts;
        uint32_t currentUptimeSeconds;
        uint32_t lastDisconnectTime;
    };
    ConnectionStats getConnectionStats();
    
private:
    WiFiCredentialManager* credManager;
    ConnectionState currentState;
    uint32_t lastConnectionAttempt;
    uint8_t retryCount;
    bool setupModeRequested;
    ConnectionStats stats;
    
    void updateConnectionState();
    uint32_t calculateRetryDelay();
};
```

### WiFiSetupPortal

Manages the web portal for credential configuration.

```cpp
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

class WiFiSetupPortal {
public:
    WiFiSetupPortal();
    
    // Start setup portal (creates AP and web server)
    bool startPortal();
    
    // Stop portal and cleanup resources
    void stopPortal();
    
    // Check if portal is currently active
    bool isPortalActive();
    
    // Handle portal timeout (call from main loop)
    void handleSetupTimeout();
    
    // Get portal statistics
    struct PortalStats {
        uint32_t sessionStartTime;
        uint32_t accessCount;
        uint32_t configurationAttempts;
        bool sessionExpired;
    };
    PortalStats getPortalStats();
    
    // Set callback for credential submission
    typedef std::function<void(const String& ssid, const String& password)> CredentialCallback;
    void setCredentialCallback(CredentialCallback callback);
    
private:
    AsyncWebServer* server;
    WiFiConnectionManager* connManager;
    bool portalActive;
    uint32_t sessionStartTime;
    PortalStats stats;
    CredentialCallback onCredentialsReceived;
    
    // Web request handlers
    void handleSetupPage(AsyncWebServerRequest* request);
    void handleCredentialSubmission(AsyncWebServerRequest* request);
    void handleStatusRequest(AsyncWebServerRequest* request);
    
    // Portal utilities
    String generateDeviceId();
    String buildSetupHTML();
    bool validateCredentialInput(const String& ssid, const String& password);
};
```

## Integration Contract

### Main Application Integration

```cpp
#include "wifi/wifi_credential_manager.h"
#include "wifi/wifi_connection_manager.h"  
#include "wifi/wifi_setup_portal.h"

class WiFiManager {
public:
    bool begin() {
        credManager.begin();
        connManager.begin();
        
        if (!credManager.hasValidCredentials()) {
            portal.startPortal();
            portal.setCredentialCallback([this](const String& ssid, const String& password) {
                handleNewCredentials(ssid, password);
            });
        } else {
            connManager.connectToStoredNetwork();
        }
        
        return true;
    }
    
    void loop() {
        connManager.handleConnectionEvents();
        portal.handleSetupTimeout();
        
        // Check for setup mode trigger (button press)
        if (setupButtonPressed()) {
            enterSetupMode();
        }
    }
    
private:
    WiFiCredentialManager credManager;
    WiFiConnectionManager connManager;
    WiFiSetupPortal portal;
    
    void handleNewCredentials(const String& ssid, const String& password);
    void enterSetupMode();
    bool setupButtonPressed();
};
```

## Error Handling

All methods use consistent error handling:
- `bool` return type for success/failure operations
- Detailed logging via Serial for debugging
- Graceful degradation on component failures
- State recovery on system restart

## Memory Management

- Stack allocation preferred over heap for small objects
- String objects managed by Arduino String class
- Web server uses async operations to prevent blocking
- NVS operations batched to minimize flash wear

## Thread Safety

- All classes designed for single-threaded operation
- WiFi events handled via Arduino framework callbacks
- No explicit locking required for ESP32 Arduino environment