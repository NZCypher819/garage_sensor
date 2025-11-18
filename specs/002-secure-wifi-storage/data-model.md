# Data Model: Secure WiFi Credential Storage

**Feature**: 002-secure-wifi-storage  
**Generated**: November 18, 2025  
**Source**: [spec.md](spec.md) functional requirements

## Core Entities

### WiFiCredentials
Primary entity for WiFi network authentication data.

**Attributes:**
- `ssid`: String (1-32 characters) - WiFi network name
- `password`: String (1-63 characters) - WiFi network password  
- `encrypted_data`: Binary blob - AES-256 encrypted credential pair
- `storage_key`: String - NVS partition identifier ("wifi_creds")
- `created_at`: Timestamp - When credentials were first stored
- `validated`: Boolean - Whether connection was successfully tested

**Validation Rules:**
- SSID must not be empty and max 32 chars (WiFi standard)
- Password must be 1-63 chars for WPA/WPA2 compliance
- Encrypted data must pass AES-256 integrity check
- Storage operations must be atomic (success/failure only)

**State Transitions:**
- NEW → ENCRYPTING → STORED → VALIDATED
- VALIDATED → UPDATING → STORED → VALIDATED
- ANY → CLEARED (factory reset)

### DeviceConfiguration  
Configuration and state management for the device WiFi system.

**Attributes:**
- `device_id`: String - Unique identifier for AP naming (MAC-based)
- `setup_mode`: Boolean - Currently accepting new credentials
- `connection_state`: Enum - Current WiFi connection status
- `last_connection_attempt`: Timestamp - Most recent connection try
- `connection_retry_count`: Integer (0-3) - Failed attempts counter
- `led_state`: Enum - Visual feedback state

**Connection States:**
- `DISCONNECTED` - Not connected, no attempts
- `CONNECTING` - Active connection attempt in progress  
- `CONNECTED` - Successfully connected to WiFi
- `FAILED` - Connection failed, may retry or enter setup

**LED States:**
- `OFF` - Normal operation, WiFi connected
- `SETUP` - Blue flashing, setup mode active
- `CONNECTING` - Blue solid, attempting connection
- `CONNECTED` - Green solid, WiFi connected
- `ERROR` - Red flashing, connection failed

### SetupSession
Temporary session data during credential setup process.

**Attributes:**
- `session_active`: Boolean - Setup portal is accessible
- `session_timeout`: Timestamp - Security timeout (10 minutes)
- `temp_credentials`: WiFiCredentials - Unvalidated during setup
- `portal_access_count`: Integer - Number of portal accesses (security)
- `validation_in_progress`: Boolean - Testing credentials currently

**Session Lifecycle:**
- Created when entering setup mode
- Validated during credential testing
- Destroyed on successful connection or timeout
- Reset on physical button press

### SecurityContext
Security-related configuration and encryption materials.

**Attributes:**  
- `encryption_key`: Binary (32 bytes) - AES-256 key derived from hardware
- `device_mac`: String - MAC address for device identification
- `key_derivation_salt`: Binary (16 bytes) - Salt for key generation
- `encryption_iv`: Binary (16 bytes) - Initialization vector for AES

**Security Properties:**
- Encryption key derived from ESP32 eFuse MAC + hardware ID
- Keys never stored in plaintext or transmitted
- New IV generated for each encryption operation
- Salt rotated on factory reset

## Relationships

```
DeviceConfiguration 1:1 WiFiCredentials (current credentials)
DeviceConfiguration 1:0..1 SetupSession (during setup only) 
SetupSession 1:1 WiFiCredentials (temporary credentials)
SecurityContext 1:1 DeviceConfiguration (encryption context)
```

## Data Flow

1. **Initial Setup**: DeviceConfiguration.setup_mode = true → create SetupSession
2. **Credential Entry**: SetupSession.temp_credentials populated via portal
3. **Validation**: Test connection using temp_credentials
4. **Storage**: Encrypt temp_credentials using SecurityContext → store as WiFiCredentials
5. **Connection**: Load WiFiCredentials → decrypt → attempt WiFi connection
6. **Success**: DeviceConfiguration.connection_state = CONNECTED

## Persistence Strategy

- **WiFiCredentials**: ESP32 NVS partition (persistent across reboots)
- **DeviceConfiguration**: RAM + NVS for critical state (setup_mode)
- **SetupSession**: RAM only (cleared on reboot)
- **SecurityContext**: Derived at runtime, keys in secure eFuse

## Memory Requirements

- WiFiCredentials: ~128 bytes encrypted
- DeviceConfiguration: ~64 bytes  
- SetupSession: ~256 bytes (temporary)
- SecurityContext: ~96 bytes
- **Total**: <512 bytes persistent, <1KB during setup