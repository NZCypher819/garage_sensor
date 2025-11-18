#pragma once

// WiFi Configuration Constants for Secure Credential Storage
// Feature: 002-secure-wifi-storage

// Access Point Configuration
#define WIFI_AP_SSID_PREFIX "GarageSensor"
#define WIFI_AP_PASSWORD "SetupMode123"  // Temporary AP password
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4
#define WIFI_AP_IP_ADDRESS IPAddress(192, 168, 4, 1)
#define WIFI_AP_GATEWAY IPAddress(192, 168, 4, 1)  
#define WIFI_AP_SUBNET IPAddress(255, 255, 255, 0)

// Connection Management
#define WIFI_CONNECTION_TIMEOUT_MS 30000    // 30 seconds for connection attempt
#define WIFI_RETRY_DELAYS_MS {2000, 4000, 8000}  // Exponential backoff: 2s, 4s, 8s
#define WIFI_MAX_RETRY_ATTEMPTS 3
#define WIFI_RECONNECTION_INTERVAL_MS 60000  // Check for reconnection every minute

// Setup Mode Configuration  
#define WIFI_SETUP_TIMEOUT_MS 600000       // 10 minutes (600,000 ms)
#define WIFI_SETUP_PORTAL_PORT 80
#define WIFI_SETUP_SESSION_TIMEOUT_MS 600000  // 10 minute session timeout
#define WIFI_SETUP_MAX_PORTAL_REQUESTS 10  // Rate limiting
#define SETUP_SESSION_TIMEOUT_MS 600000        // 10 minutes (alias)
#define MAX_CONNECTION_RETRIES 3               // Maximum connection attempts

// Security Configuration
#define WIFI_CREDENTIAL_NVS_NAMESPACE "wifi_creds"
#define WIFI_CREDENTIAL_NVS_KEY "credentials" 
#define WIFI_DEVICE_CONFIG_NVS_KEY "device_cfg"
#define WIFI_ENCRYPTION_KEY_SIZE 32        // AES-256 requires 32-byte key
#define WIFI_ENCRYPTION_IV_SIZE 16         // AES block size
#define WIFI_ENCRYPTION_SALT_SIZE 16       // Salt for key derivation
#define WIFI_MAX_SSID_LENGTH 32            // IEEE 802.11 standard
#define WIFI_MAX_PASSWORD_LENGTH 63        // WPA/WPA2 standard

// Physical Reset Configuration
#define WIFI_RESET_BUTTON_PIN GPIO_NUM_0   // Boot button on ESP32-S3
#define WIFI_RESET_HOLD_TIME_MS 10000      // 10 seconds hold time
#define PHYSICAL_RESET_HOLD_TIME_MS 10000  // 10 seconds (alias)
#define WIFI_RESET_DEBOUNCE_TIME_MS 50     // 50ms debounce

// LED Feedback Configuration
#define WIFI_STATUS_LED_PIN GPIO_NUM_2     // Built-in LED
#define WIFI_LED_BLINK_FAST_MS 250         // Fast blink for setup mode
#define WIFI_LED_BLINK_SLOW_MS 1000        // Slow blink for connecting
#define WIFI_LED_SOLID_ON_MS 0             // Solid on for connected

// Protocol Support
#define WIFI_ENABLE_WPA2 true
#define WIFI_ENABLE_WPA3 true
#define WIFI_ENABLE_WEP false              // Deprecated and insecure

// Memory and Performance Constraints
#define WIFI_PORTAL_ASSET_MAX_SIZE 8192    // 8KB max for HTML/CSS/JS
#define WIFI_MAX_MEMORY_OVERHEAD_PERCENT 10
#define WIFI_PORTAL_RESPONSE_TIMEOUT_MS 3000  // 3 second response requirement

// Logging Configuration
#define WIFI_LOG_LEVEL_ERROR 1
#define WIFI_LOG_LEVEL_WARN  2  
#define WIFI_LOG_LEVEL_INFO  3
#define WIFI_LOG_LEVEL_DEBUG 4

#ifndef WIFI_LOG_LEVEL
#define WIFI_LOG_LEVEL WIFI_LOG_LEVEL_INFO
#endif

// Validation Constraints
static_assert(WIFI_MAX_SSID_LENGTH <= 32, "SSID length must comply with IEEE 802.11");
static_assert(WIFI_MAX_PASSWORD_LENGTH <= 63, "Password length must comply with WPA standards");
static_assert(WIFI_ENCRYPTION_KEY_SIZE == 32, "AES-256 requires 32-byte key");