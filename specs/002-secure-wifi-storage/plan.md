# Implementation Plan: Secure WiFi Credential Storage

**Branch**: `002-secure-wifi-storage` | **Date**: November 18, 2025 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/002-secure-wifi-storage/spec.md`

## Summary

Implement secure WiFi credential storage system for garage sensor device. Core requirement: Store WiFi SSID/password on ESP32 using AES-256 encryption with zero GitHub exposure. Technical approach: ESP32 temporary access point during setup → web portal for credential entry → encrypted storage in NVS → automatic connection on boot → physical reset capability.

## Technical Context

**Language/Version**: C++ with Arduino framework for ESP32  
**Primary Dependencies**: Arduino WiFi, ESP32 NVS, WebServer, AES encryption library  
**Storage**: ESP32 Non-Volatile Storage (NVS) partition for encrypted credentials  
**Testing**: PlatformIO unit tests, integration tests for WiFi functionality  
**Target Platform**: ESP32-S3-NANO (Espressif 32 platform 6.4.0)  
**Project Type**: Single embedded project with web interface component  
**Performance Goals**: <30s boot-to-WiFi, <5min setup process, <3s web portal response  
**Constraints**: <10% memory overhead, 10-minute setup timeout, secure credential storage only  
**Scale/Scope**: Single-device credential management, simple web portal, basic visual feedback

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

✅ **I. Reliability-First**: Feature includes credential validation via actual WiFi connection attempt before storage. Error handling for network outages and invalid credentials explicitly defined. Graceful degradation when WiFi unavailable.

✅ **II. Power-Aware Design**: Setup mode timeout (10 minutes) conserves power. Temporary AP disabled after successful connection. LED feedback minimizes continuous power draw. Impact: Estimated <5% additional power consumption during normal operation.

✅ **III. Specification-Driven Development**: Complete user scenarios with independently testable stories. Technical plan required before implementation. All user stories prioritized (P1/P2) and independently deployable.

✅ **IV. Secure-by-Design**: AES-256 encryption mandatory before storage. No plaintext credential exposure. GitHub isolation explicitly required. Physical reset mechanism for security. Secure credential wiping implemented.

✅ **V. Observability and Diagnostics**: Visual LED feedback for connection states. Structured logging during setup and connection phases. Error states clearly communicated through LED patterns and portal interface.

✅ **VI. Test-Driven Development**: Unit tests required for credential encryption/decryption. Integration tests for WiFi functionality and NVS storage. Setup portal testing mandatory. Test coverage >80% required per constitution.

**GATE RESULT**: ✅ PASS - All constitutional principles addressed

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/
├── wifi/                         # New WiFi credential management
│   ├── wifi_credential_manager.h  # Core credential storage/encryption
│   ├── wifi_credential_manager.cpp
│   ├── wifi_setup_portal.h       # Web portal for credential entry  
│   ├── wifi_setup_portal.cpp
│   ├── wifi_connection_manager.h # Connection handling and state
│   └── wifi_connection_manager.cpp
├── config/                       # Existing configuration
│   ├── hardware.h               # Board pin definitions (existing)
│   └── wifi_config.h            # WiFi-specific constants (new)
├── security/                     # Existing security infrastructure
│   ├── credential_encryption.h  # AES-256 encryption utilities (new)
│   └── credential_encryption.cpp
└── actuators/                   # Existing LED controllers (for feedback)
    └── status_led_controller.h  # WiFi status LED patterns (extend)

data/                            # Web portal assets
├── setup_portal.html           # Setup form interface
├── setup_portal.css            # Styling
└── setup_portal.js             # Client-side validation

tests/
├── unit/
│   ├── test_wifi_credential_manager.cpp  # Encryption/storage tests
│   ├── test_credential_encryption.cpp    # AES-256 tests
│   └── test_wifi_connection_manager.cpp  # Connection logic tests
└── integration/
    ├── test_setup_portal.cpp            # Web interface tests
    ├── test_wifi_flow.cpp               # End-to-end setup tests
    └── test_nvs_storage.cpp             # NVS persistence tests
```

**Structure Decision**: Single embedded project extending existing garage sensor architecture. WiFi components organized in dedicated `src/wifi/` module for clear separation. Web assets in `data/` directory following ESP32 convention. Security utilities extend existing `src/security/` infrastructure.

## Phase 0: Research & Unknown Resolution

*Extract unknowns from Technical Context and generate research tasks*

### Research Tasks Generated

**R001: ESP32 NVS Encryption Implementation**
- Task: Research ESP32 NVS partition management and AES-256 encryption integration
- Unknown: Best practices for secure key derivation and storage on ESP32
- Why: Constitution requires secure-by-design implementation

**R002: WiFi Access Point & Web Server Libraries** 
- Task: Identify optimal Arduino libraries for temporary AP and web portal
- Unknown: WebServer library capabilities for form handling and asset serving
- Why: Setup portal must be responsive (<3s) and reliable

**R003: WiFi Connection State Management**
- Task: Research WiFi reconnection strategies and connection timeout handling  
- Unknown: Best practices for automatic reconnection vs setup mode triggering
- Why: Reliability-first requires graceful handling of network outages

**R004: ESP32 Memory Management for Web Assets**
- Task: Research efficient storage and serving of HTML/CSS/JS from ESP32
- Unknown: Memory constraints and SPIFFS vs embedded assets trade-offs
- Why: <10% memory overhead constraint and power-aware design principles

### Research Findings Summary

#### R001: ESP32 NVS Encryption Implementation

**Decision**: Use ESP32 Preferences library with custom AES-256 encryption layer
**Rationale**: Preferences library provides NVS abstraction. AES-256 encryption applied before storage using ESP32 hardware crypto acceleration.
**Alternatives considered**: Direct NVS API (more complex), built-in ESP32 encryption (less control)

#### R002: WiFi Access Point & Web Server Libraries

**Decision**: ESP32 WiFi library for AP mode, ESPAsyncWebServer for portal
**Rationale**: ESPAsyncWebServer provides non-blocking operation and better resource management than standard WebServer. Native WiFi library offers reliable AP creation.
**Alternatives considered**: Arduino WebServer (blocking), WiFiManager (limited customization)

#### R003: WiFi Connection State Management  

**Decision**: Exponential backoff reconnection with setup mode fallback
**Rationale**: Attempt reconnection 3 times with 2s, 4s, 8s delays. If all fail, trigger setup mode for user intervention.
**Alternatives considered**: Continuous retry (power drain), immediate setup mode (no resilience)

#### R004: ESP32 Memory Management for Web Assets

**Decision**: Inline HTML/CSS/JS as string constants in program memory  
**Rationale**: Avoids SPIFFS overhead and file system complexity. Portal is simple enough for inline approach. Keeps assets under 8KB total.
**Alternatives considered**: SPIFFS files (complexity), external hosting (requires internet)

## Post-Design Constitution Check

*Re-evaluate constitutional compliance after Phase 1 design completion*

✅ **I. Reliability-First**: Design includes comprehensive error handling for WiFi failures, credential corruption, and memory issues. Exponential backoff reconnection strategy prevents connection storms. State validation before storage operations.

✅ **II. Power-Aware Design**: Setup mode timeout conserves power. Async web server prevents blocking operations. LED feedback optimized for minimal power draw. Connection retry logic includes power-saving delays.

✅ **III. Specification-Driven Development**: Data model, API contracts, and quickstart guide generated from functional requirements. Clear separation of concerns in class design. All user stories mapped to specific API operations.

✅ **IV. Secure-by-Design**: AES-256 encryption with hardware-derived keys. NVS secure storage. No credential transmission. Session timeouts. Rate limiting on portal access. Secure credential wiping implemented.

✅ **V. Observability and Diagnostics**: Structured logging in all components. Connection statistics tracking. Portal access monitoring. LED state machine for visual feedback. Serial debugging support.

✅ **VI. Test-Driven Development**: Unit test contracts defined for all classes. Integration tests for end-to-end WiFi flow. Test coverage requirements >80%. Mock objects for hardware dependencies.

**FINAL GATE RESULT**: ✅ PASS - Design maintains constitutional compliance

---

## Planning Complete

**Status**: ✅ READY FOR TASK BREAKDOWN  
**Next Command**: `/speckit.tasks` to generate detailed implementation tasks  
**Artifacts Created**: 
- [plan.md](plan.md) - This implementation plan
- [data-model.md](data-model.md) - Entity definitions and relationships  
- [contracts/](contracts/) - API specifications and class interfaces
- [quickstart.md](quickstart.md) - Developer and user guides
