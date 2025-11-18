# Research: Secure WiFi Credential Storage

**Feature**: 002-secure-wifi-storage  
**Date**: November 18, 2025  
**Phase**: 0 (Research & Planning)

## Research Summary

This document consolidates technical research for implementing secure WiFi credential storage on ESP32-based garage sensor devices. All research was based on existing project implementation and ESP32 best practices.

---

## ESP32 WiFi Connection Management

### Decision: Multi-state connection manager with exponential backoff retry
**Rationale**: ESP32 WiFi requires careful state management due to hardware limitations and power considerations. The existing `WiFiConnectionManager` implements proven patterns.
**Alternatives considered**: Simple blocking connection (unreliable), polling-only approach (inefficient)
**Implementation notes**: 
- State machine: DISCONNECTED → CONNECTING → CONNECTED → SETUP_MODE
- Exponential backoff: 2s, 4s, 8s delays between retries (max 3 attempts)
- 30-second connection timeout per attempt
- Automatic fallback to setup mode if no stored credentials

---

## ESP32 NVS Storage Strategy

### Decision: ESP32 NVS (Non-Volatile Storage) with dedicated namespace
**Rationale**: NVS is designed for small key-value storage on ESP32 and provides wear leveling automatically. More reliable than direct flash writes.
**Alternatives considered**: SPIFFS (overkill for credentials), EEPROM emulation (deprecated on ESP32)
**Implementation notes**:
- Namespace: `wifi_creds` to isolate credential data
- Keys: `credentials` (encrypted blob), `device_cfg` (device metadata)
- Maximum credential blob size: 128 bytes (sufficient for encrypted SSID+password)
- Built-in wear leveling and power-loss protection

---

## AES-256 Encryption Implementation

### Decision: AESLib library with hardware-derived keys
**Rationale**: AESLib provides proven AES-256-CBC implementation optimized for Arduino/ESP32. Hardware derivation ensures unique keys per device.
**Alternatives considered**: mbedTLS (complex integration), ESP32 hardware crypto (limited API)
**Implementation notes**:
- AES-256-CBC with random IV per encryption operation
- Key derivation: SHA-256(MAC_address + hardware_eFuse + salt)
- 32-byte encryption key, 16-byte IV, 16-byte salt
- PKCS#7 padding for variable-length credentials
- Secure memory wiping after operations

---

## ESPAsyncWebServer Setup Portal

### Decision: ESPAsyncWebServer with minimal HTML/CSS/JS assets
**Rationale**: ESPAsyncWebServer provides non-blocking HTTP server ideal for ESP32. Minimal assets reduce memory usage.
**Alternatives considered**: ESP32WebServer (blocking), custom HTTP parser (too complex)
**Implementation notes**:
- Single-page application stored in ESP32 flash (SPIFFS)
- Assets: setup_portal.html (form), setup_portal.css (styling), setup_portal.js (validation)
- Maximum asset size: 8KB total (memory constraint)
- POST endpoint for credential submission with CSRF protection
- Automatic portal shutdown after successful connection

---

## ESP32 Access Point Configuration

### Decision: Temporary AP with unique SSID and no password
**Rationale**: Open AP simplifies user connection process. Unique SSID prevents confusion in multi-device environments.
**Alternatives considered**: WPA-protected AP (adds setup complexity), fixed SSID (device conflicts)
**Implementation notes**:
- SSID pattern: `GarageSensor-[DEVICE_ID]` where DEVICE_ID = last 6 chars of MAC
- Channel: 1 (most compatible)
- IP range: 192.168.4.1/24 (ESP32 default, widely supported)
- Maximum 4 concurrent connections
- 10-minute timeout for security

---

## LED Status Feedback Patterns

### Decision: Status LED with distinct patterns for each WiFi state
**Rationale**: Visual feedback essential for headless device troubleshooting. Clear patterns enable user diagnosis.
**Alternatives considered**: RGB LED (increased complexity), buzzer (annoying), no feedback (poor UX)
**Implementation notes**:
- LED states aligned with connection manager states:
  - OFF = Connected to WiFi (normal operation)
  - Fast blink (250ms) = Setup mode active
  - Slow blink (1000ms) = Connecting to WiFi
  - Solid on = Setup portal ready
  - Fast red flash = Connection error (if RGB available)
- Non-blocking LED updates in main loop
- Configurable pin assignment via wifi_config.h

---

## Physical Reset Implementation

### Decision: Boot button long-press (10 seconds) with debouncing
**Rationale**: Boot button available on all ESP32 dev boards. Long press prevents accidental resets.
**Alternatives considered**: Dedicated button (additional hardware), short press (too sensitive)
**Implementation notes**:
- Button pin: GPIO_NUM_0 (ESP32 boot button)
- Press duration: 10 seconds minimum
- Debounce time: 50ms
- Visual feedback: LED pattern change during press
- Action: Enter setup mode with credential clearing

---

## Power Efficiency Optimizations

### Decision: Aggressive AP shutdown and connection state optimization
**Rationale**: Battery-powered garage sensors require minimal power consumption. WiFi is the largest power consumer.
**Alternatives considered**: Always-on AP (high power), periodic wakeup (connectivity gaps)
**Implementation notes**:
- AP mode disabled immediately after successful credential validation
- Connection retry delays to reduce scan power consumption
- WiFi sleep mode enabled when connected
- Setup mode timeout (10 minutes) to prevent battery drain
- Connection state caching to minimize WiFi library calls

---

## Security Best Practices

### Decision: Zero plaintext credential exposure with secure key management
**Rationale**: Garage sensors control physical access. Credential compromise could expose home network.
**Alternatives considered**: Base64 encoding (not secure), simple XOR (easily broken)
**Implementation notes**:
- Credentials never stored in plaintext anywhere in system
- Encryption keys derived from hardware MAC + eFuse + salt
- Secure memory wiping for sensitive variables
- No credential logging even in debug modes
- IV randomization for each encryption operation
- Credential validation via actual connection test before storage

---

## Memory Management Strategy

### Decision: Stack allocation for encryption buffers with careful sizing
**Rationale**: ESP32 has limited heap. Stack allocation is predictable and avoids fragmentation.
**Alternatives considered**: Dynamic allocation (fragmentation risk), static globals (memory waste)
**Implementation notes**:
- Maximum encrypted credential size: 128 bytes
- Encryption buffer sizing: ((plaintext + 15) / 16) * 16 + 16 (padding + IV)
- String operations minimized during encryption/decryption
- Automatic cleanup of sensitive stack variables
- Memory overhead target: <10% of total ESP32 SRAM

---

## Implementation Strategy Summary

**Phase Priority**: 
1. Core credential encryption (User Story 3) - Foundation for all other features
2. Setup portal and connection management (User Story 1) - Primary user workflow  
3. Physical reset capability (User Story 2) - Recovery mechanism

**Risk Mitigation**:
- Extensive unit tests for encryption/decryption functions
- Integration tests for complete setup workflows
- Security validation tests for credential isolation
- Power consumption monitoring during development

**Performance Targets**:
- WiFi connection: <30 seconds from boot
- Setup portal response: <3 seconds
- Credential encryption: <1 second
- Memory usage: <10% overhead

All technical decisions align with garage sensor constitutional principles of reliability-first design, power awareness, security-by-design, and comprehensive testing requirements.