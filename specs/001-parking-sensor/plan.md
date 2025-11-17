# Implementation Plan: Garage Parking Position Sensor

**Branch**: `001-parking-sensor` | **Date**: 2025-11-17 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-parking-sensor/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Primary requirement: ESP32 S3 Nano-based parking sensor that uses E3JK-RR11 IR beam sensor to detect vehicle position and provide immediate LED feedback (<100ms). Technical approach: PlatformIO-based firmware with WiFi connectivity for secure HTTPS-based OTA updates from GitHub releases, comprehensive sensor validation, power management, structured logging for reliability monitoring, and mandatory automated testing per constitutional requirement VI.

## Technical Context

<!--
  ACTION REQUIRED: Replace the content in this section with the technical details
  for the project. The structure here is presented in advisory capacity to guide
  the iteration process.
-->

**Language/Version**: C++ (Arduino framework via PlatformIO, ESP-IDF core)  
**Primary Dependencies**: WiFi library, ArduinoOTA, Arduino JSON, ESP32 deep sleep libraries, HTTPS client library  
**Storage**: SPIFFS/LittleFS for configuration, EEPROM for persistent settings, local logging  
**Testing**: PlatformIO Unit Testing framework, hardware-in-the-loop testing setup, 80% minimum coverage per constitution VI  
**Target Platform**: ESP32-S3-NANO (Espressif Systems), garage environment (-20°C to 60°C per constitution)
**Project Type**: Embedded IoT firmware (single microcontroller project)  
**Performance Goals**: <100ms response time, <10mA idle current, 6+ month continuous operation, 99% OTA success rate  
**Constraints**: IP65 environmental rating, WiFi range limitations, power efficiency critical, HTTPS-only updates  
**Scale/Scope**: Single device deployment, <10K lines firmware code, 3 user stories + security requirements, mandatory test coverage
**LED Implementation**: Dual LED setup - Parking LED (bright on/off for vehicle detection), Status LED (dim/blink patterns: 25% idle, 2x blink startup, rapid blink error)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Compliance | Status | Post-Design Review |
|-----------|------------|--------|--------------------|  
| **I. Reliability-First** | ✅ PASS | Sensor validation, error handling, graceful degradation designed into requirements | ✅ CONFIRMED: Data model includes sensor health monitoring, error detection, and fault recovery patterns |
| **II. Power-Aware Design** | ✅ PASS | Explicit power budgeting (<10mA idle), sleep modes, power measurement requirements | ✅ CONFIRMED: Power management strategy defined with light sleep, wake-up patterns, and consumption monitoring |
| **III. Specification-Driven** | ✅ PASS | Following speckit workflow, prioritized user stories, independent testability | ✅ CONFIRMED: Complete spec → clarify → plan → design workflow followed with all artifacts generated |
| **IV. Secure-by-Design** | ✅ PASS | WiFi encryption, HTTPS OTA updates, file integrity validation, rollback capability | ✅ CONFIRMED: Security clarified - HTTPS downloads with integrity checks and rollback protection |
| **V. Observability** | ✅ PASS | Structured logging, health metrics, diagnostic capabilities required | ✅ CONFIRMED: Comprehensive logging API, health endpoints, and diagnostic data model defined |
| **VI. Test-Driven Development** | ✅ PASS | 80% test coverage requirement, unit tests for all logic, integration tests for hardware interfaces | ✅ CONFIRMED: PlatformIO test framework selected, comprehensive testing strategy documented in research.md, coverage measurement with gcov/lcov, hardware-in-the-loop patterns established |

**Gate Result**: ✅ **PROCEED** - All constitution principles satisfied including new mandatory testing requirements

## Project Structure

### Documentation (this feature)

```text
specs/001-parking-sensor/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── sensor-api.json  # E3JK-RR11 sensor interface
│   ├── wifi-api.json    # WiFi/OTA interface definitions
│   └── logging-api.json # Diagnostic and logging interfaces
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```text
# Single embedded project structure
src/
├── main.cpp             # Main application entry point
├── config/
│   ├── hardware.h       # Pin definitions, hardware constants
│   ├── wifi_config.h    # WiFi and OTA configuration
│   └── constants.h      # Application constants and thresholds
├── sensors/
│   ├── e3jk_sensor.cpp  # E3JK-RR11 sensor driver and validation
│   └── e3jk_sensor.h
├── actuators/
│   ├── parking_led_controller.cpp # Parking LED control with <100ms timing
│   ├── status_led_controller.cpp  # Status LED patterns and health indication
│   └── led_controllers.h         # Common LED controller interfaces
├── network/
│   ├── wifi_manager.cpp # WiFi connection and OTA handling
│   ├── wifi_manager.h
│   ├── ota_handler.cpp  # Secure HTTPS OTA update management
│   └── github_client.cpp # GitHub releases API client
├── power/
│   ├── power_manager.cpp # Sleep modes and power optimization
│   └── power_manager.h
├── security/
│   ├── integrity_check.cpp # File integrity validation
│   └── rollback_manager.cpp # Firmware rollback capability
├── diagnostics/
│   ├── logger.cpp       # Structured logging system
│   ├── logger.h
│   ├── health_monitor.cpp # System health and metrics
│   └── health_monitor.h
└── utils/
    ├── timer_utils.cpp  # Precise timing for <100ms requirements
    └── timer_utils.h

platformio.ini           # PlatformIO configuration
lib/                     # External libraries
test/
├── test_sensor/         # Sensor validation tests (constitution VI)
├── test_timing/         # Response time validation tests
├── test_power/          # Power consumption tests
├── test_security/       # OTA security validation tests
├── test_integration/    # End-to-end system tests
├── test_led_controllers/ # LED controller unit tests
├── test_wifi/          # WiFi and network tests
└── test_coverage/      # Coverage measurement and reporting (80% minimum)

data/                    # SPIFFS filesystem content
├── config.json          # Runtime configuration
├── certs/              # Root CA certificates for HTTPS
└── logs/               # Local diagnostic logs
```

**Structure Decision**: Single embedded project chosen for ESP32 S3 Nano deployment. Modular architecture separates concerns (sensors, actuators, network, power, security, diagnostics) to support independent testing and maintainability while meeting real-time performance requirements. Comprehensive test structure added per constitutional principle VI requiring 80% test coverage and automated tests for all modules.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
