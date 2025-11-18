# Feature Specification: Secure WiFi Credential Storage

**Feature Branch**: `002-secure-wifi-storage`  
**Created**: November 18, 2025  
**Status**: Draft  
**Input**: User description: "We need a way to securely store the wifi creds on the device. The creds must not be sync'd with github."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Initial WiFi Setup via Web Portal (Priority: P1)

A user needs to configure WiFi credentials on their new garage sensor device when they first set it up in their garage. The device should create a temporary access point that allows the user to connect and enter their home WiFi credentials through a simple web interface.

**Why this priority**: This is the foundational capability - without WiFi connectivity, the garage sensor cannot function. This must work out-of-the-box for any user.

**Independent Test**: Can be fully tested by powering on a fresh device, connecting to its AP, entering WiFi credentials via web interface, and confirming the device connects to home WiFi. Delivers complete initial setup capability.

**Acceptance Scenarios**:

1. **Given** a fresh garage sensor device, **When** powered on for the first time, **Then** it creates an access point named "GarageSensor-[DeviceID]"
2. **Given** the device is in setup mode, **When** user connects to the AP and navigates to the setup portal, **Then** they see a simple form to enter WiFi SSID and password
3. **Given** valid WiFi credentials are entered, **When** user submits the form, **Then** credentials are encrypted and stored securely on device
4. **Given** credentials are stored, **When** device restarts, **Then** it automatically connects to the configured WiFi network
5. **Given** successful WiFi connection, **When** setup is complete, **Then** the temporary access point is disabled

---

### User Story 2 - WiFi Credential Update via Physical Button (Priority: P2)

A user needs to update WiFi credentials when their network password changes or they move the device to a different location. They should be able to reset the device to setup mode using a physical button sequence.

**Why this priority**: Essential for long-term usability - users need a way to reconfigure WiFi without reflashing firmware or technical knowledge.

**Independent Test**: Can be tested by holding a button sequence on a configured device, confirming it enters setup mode, and successfully updating credentials.

**Acceptance Scenarios**:

1. **Given** a configured device, **When** user holds the reset button for 10 seconds, **Then** device enters setup mode with temporary AP
2. **Given** device is in credential update mode, **When** new credentials are entered, **Then** old credentials are securely overwritten
3. **Given** invalid credentials are entered, **When** connection fails, **Then** device remains in setup mode for retry

---

### User Story 3 - Credential Security and Isolation (Priority: P1)

The device must protect WiFi credentials from unauthorized access and ensure they are never transmitted to or stored in any external repository including GitHub.

**Why this priority**: Security is fundamental - compromised credentials could expose the user's entire home network.

**Independent Test**: Can be tested by inspecting device firmware, checking repository contents, and attempting to extract credentials via various attack vectors.

**Acceptance Scenarios**:

1. **Given** WiFi credentials are stored on device, **When** firmware is built and published, **Then** no credentials appear in source code or binaries
2. **Given** stored credentials, **When** unauthorized access is attempted, **Then** credentials remain encrypted and inaccessible
3. **Given** device reset or factory restore, **When** old credentials are cleared, **Then** they are securely wiped and unrecoverable

---

### Edge Cases

- What happens when the user enters invalid WiFi credentials multiple times?
- How does the system handle WiFi network outages or temporary disconnections?
- What occurs if the device fills up its credential storage or encounters memory corruption?
- How does the device behave when WiFi signal is weak or intermittent?
- What happens if multiple devices with the same device ID exist on the network?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST provide a temporary WiFi access point for initial setup when no valid credentials exist
- **FR-002**: System MUST present a web-based configuration portal accessible via the temporary access point
- **FR-003**: System MUST encrypt WiFi credentials using AES-256 encryption before storage
- **FR-004**: System MUST store encrypted credentials in ESP32 NVS (Non-Volatile Storage) partition
- **FR-005**: System MUST automatically attempt connection to stored WiFi network on boot
- **FR-006**: System MUST provide a physical reset mechanism to clear stored credentials and enter setup mode
- **FR-007**: System MUST validate WiFi credentials by attempting actual connection before saving
- **FR-008**: System MUST disable temporary access point once valid WiFi connection is established  
- **FR-009**: System MUST support WPA2 and WPA3 WiFi security protocols
- **FR-010**: System MUST never transmit, log, or expose plaintext WiFi credentials
- **FR-011**: System MUST generate unique device identifier for access point naming
- **FR-012**: System MUST provide visual feedback (LED) during different connection states
- **FR-013**: System MUST timeout setup mode after 10 minutes of inactivity for security
- **FR-014**: System MUST securely wipe old credentials when new ones are saved

### Key Entities

- **WiFi Credentials**: SSID (network name) and password pair, stored encrypted in device flash memory
- **Device Configuration**: Current WiFi connection state, setup mode status, and connection attempt history  
- **Setup Portal**: Temporary web interface for credential entry, served only during setup mode
- **Security Context**: Encryption keys, device identifiers, and access control mechanisms

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can complete initial WiFi setup in under 5 minutes from device power-on
- **SC-002**: Device connects to configured WiFi network within 30 seconds of boot under normal conditions
- **SC-003**: WiFi credentials remain secure with zero plaintext exposure in logs, source code, or external storage
- **SC-004**: Device maintains WiFi connectivity with 99%+ uptime when network is available
- **SC-005**: Setup mode is accessible 100% of the time via physical reset button within 10 seconds
- **SC-006**: Credential updates complete successfully within 2 minutes including device reconnection
- **SC-007**: Device consumes less than 10% additional memory for credential storage and management
- **SC-008**: Setup portal loads and responds within 3 seconds of connecting to device access point

## Assumptions

- Users have basic knowledge of their WiFi network name and password
- Device will be used in residential environments with standard WiFi routers
- Physical access to device reset button is available for credential updates
- Users can connect to temporary WiFi access points using smartphones or computers
- Standard web browsers support the setup portal interface
- ESP32 NVS partition has sufficient space for encrypted credential storage
- Device operates in environments with 2.4GHz WiFi coverage

## Dependencies

- ESP32 WiFi hardware and Arduino WiFi libraries
- ESP32 NVS (Non-Volatile Storage) system for persistent storage
- AES encryption libraries for credential protection
- Web server capabilities for setup portal
- LED indicators for visual feedback during setup

## Scope Boundaries

**In Scope:**
- WiFi credential storage and management
- Initial device setup via temporary access point
- Credential encryption and security
- Physical reset mechanism for reconfiguration
- Basic web portal for credential entry

**Out of Scope:**
- Advanced network configuration (static IPs, proxy settings)
- Enterprise WiFi authentication (WPA2-Enterprise)  
- Remote credential management via internet services
- Bluetooth or other alternative connection methods
- Network diagnostics or troubleshooting tools
- Multi-network credential storage
