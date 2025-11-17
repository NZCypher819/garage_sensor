# Tasks: Garage Parking Position Sensor

**Input**: Design documents from `/specs/001-parking-sensor/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: Constitutional principle VI MANDATES automated testing for all modules with 80% minimum coverage. Test tasks are integrated throughout implementation phases.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single project**: `src/`, `tests/` at repository root
- Paths shown below assume single embedded project per plan.md structure

---

## Phase 1: Setup (Project Initialization)

**Purpose**: Initialize PlatformIO project structure and basic configuration

- [X] T001 Create PlatformIO project structure with platformio.ini configuration
- [X] T002 [P] Initialize Git repository and configure .gitignore for PlatformIO projects
- [X] T003 [P] Create project directory structure per plan.md specification in src/
- [X] T004 [P] Setup SPIFFS filesystem structure in data/ directory
- [X] T005 [P] Configure PlatformIO library dependencies for ESP32, WiFi, JSON, and HTTPS client
- [X] T006 Create main.cpp entry point with basic initialization framework
- [X] T007 [P] Setup hardware pin definitions in src/config/hardware.h
- [X] T008 [P] Create basic configuration schema in data/config.json
- [X] T008a [P] Setup PlatformIO test environments (native + ESP32) per constitutional requirement VI
- [X] T008b [P] Configure test coverage measurement with gcov/lcov for 80% minimum coverage
- [X] T008c [P] Create hardware abstraction interfaces for test mocking in test/mocks/
- [X] T008d [P] Setup GitHub Actions CI/CD pipeline for automated test execution

**Checkpoint**: Project structure ready - foundational implementation can begin ✅

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T009 Implement base logging system in src/diagnostics/logger.cpp with structured output
- [X] T010 [P] Create configuration manager in src/config/ for reading/writing SPIFFS config
- [X] T011 [P] Implement timer utilities in src/utils/timer_utils.cpp for precise <100ms timing
- [X] T012 [P] Setup basic error handling and fault detection framework
- [X] T013 Create device status manager in src/diagnostics/health_monitor.cpp
- [X] T014 [P] Implement WiFi connection manager base in src/network/wifi_manager.cpp
- [X] T015 [P] Create power management framework in src/power/power_manager.cpp
- [X] T016 Setup interrupt handling framework for GPIO sensors and timing-critical operations
- [X] T016a [P] Implement mock sensor interface for unit testing in test/mocks/mock_sensor.cpp
- [X] T016b [P] Create test utilities for timing validation in test/utils/timing_test.cpp
- [X] T016c [P] Setup hardware-in-the-loop test bench for integration testing per research.md

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Basic Parking Detection (Priority: P1) 🎯 MVP

**Goal**: IR beam sensor triggers LED within 100ms when vehicle breaks beam at parking position

**Independent Test**: Place object in beam path and verify LED illuminates within 100ms, turns off when object removed

### Implementation for User Story 1

- [X] T017 [P] [US1] Implement E3JK-RR11 sensor driver in src/sensors/e3jk_sensor.cpp
- [X] T017a [P] [US1] Write unit tests for sensor driver in test/test_sensor/test_e3jk_sensor.cpp
- [X] T018 [P] [US1] Create parking LED controller with fast response in src/actuators/parking_led_controller.cpp
- [X] T018a [P] [US1] Write unit tests for parking LED controller in test/test_actuators/test_parking_led.cpp
- [X] T018b [P] [US1] Create common LED interface definitions in src/actuators/led_controllers.h
- [X] T019 [US1] Implement sensor validation logic with debouncing in src/sensors/e3jk_sensor.cpp
- [X] T019a [US1] Write unit tests for sensor validation and debouncing logic
- [X] T020 [US1] Create parking event detection logic linking sensor to LED response
- [X] T020a [US1] Write integration tests for sensor-to-LED response timing validation
- [X] T021 [US1] Implement timing measurement for response time validation (<100ms requirement)
- [X] T021a [P] [US1] Create automated timing tests to validate <100ms response requirement
- [X] T022 [US1] Add sensor interrupt handling for immediate response to beam state changes
- [X] T022a [P] [US1] Write interrupt handling tests with mock GPIO interfaces
- [X] T023 [US1] Integrate parking event logging with structured diagnostic output
- [X] T023a [P] [US1] Create unit tests for parking event logging and data validation

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - System Health Indication (Priority: P2)

**Goal**: Visual indication of system operational status via LED patterns and health monitoring

**Independent Test**: Power cycle system, disconnect sensor, verify appropriate LED status patterns

### Implementation for User Story 2

- [X] T024 [P] [US2] Implement system health monitoring in src/diagnostics/health_monitor.cpp
- [X] T024a [P] [US2] Write unit tests for health monitoring state machine in test/test_health/
- [X] T025 [P] [US2] Create status LED controller with patterns: startup (2x 200ms blinks), normal idle (25% brightness), error (rapid 100ms blinks) in src/actuators/status_led_controller.cpp
- [X] T025a [P] [US2] Write unit tests for status LED patterns and timing validation
- [X] T026 [US2] Implement sensor fault detection and health state transitions
- [X] T026a [P] [US2] Create unit tests for fault detection algorithms and edge cases
- [X] T027 [US2] Add system status reporting via structured logging and status endpoints
- [X] T027a [P] [US2] Write integration tests for status reporting and logging validation
- [X] T028 [US2] Create watchdog timer functionality for fault recovery
- [X] T028a [P] [US2] Write unit tests for watchdog timer and recovery mechanisms
- [X] T029 [US2] Implement health check scheduling and automated status updates
- [X] T029a [P] [US2] Create tests for health check scheduling and status update intervals

**Checkpoint**: System health monitoring complete and independently verifiable

---

## Phase 5: User Story 3 - Power Management (Priority: P3)

**Goal**: Intelligent power management with <10mA idle consumption while maintaining <100ms response

**Independent Test**: Measure current consumption in active vs idle states, verify wake-up responsiveness

### Implementation for User Story 3

- [X] T030 [P] [US3] Implement ESP32 light sleep mode management in src/power/power_manager.cpp
- [X] T030a [P] [US3] Write unit tests for power state machine and sleep transitions
- [X] T031 [P] [US3] Configure GPIO wake-up from sensor interrupts for instant response
- [X] T031a [P] [US3] Create tests for wake-up timing and GPIO interrupt responsiveness
- [X] T032 [US3] Implement power state transitions with timeout-based sleep entry
- [X] T032a [P] [US3] Write unit tests for power state timeout logic and edge cases
- [X] T033 [US3] Add current consumption monitoring and reporting
- [X] T033a [P] [US3] Create automated power consumption measurement tests (<10mA validation)
- [X] T034 [US3] Optimize peripheral power usage (disable unused WiFi during sleep)
- [X] T034a [P] [US3] Write integration tests for peripheral power optimization
- [X] T035 [US3] Validate power savings while maintaining sensor responsiveness requirements
- [X] T035a [P] [US3] Create comprehensive power management integration tests

**Checkpoint**: Power management complete with verified <10mA idle consumption and <100ms response

---

## Phase 6: Security & OTA Updates

**Goal**: Secure HTTPS-based OTA updates from GitHub releases with integrity validation and rollback

**Independent Test**: Deploy firmware update from GitHub release, verify integrity checks and rollback capability

### Implementation for Security & OTA

- [X] T036 [P] Implement HTTPS client with certificate validation in src/network/github_client.cpp
- [ ] T036a [P] Write unit tests for HTTPS client and certificate validation logic
- [X] T037 [P] Create file integrity validation using SHA256 in src/security/integrity_check.cpp
- [ ] T037a [P] Write unit tests for SHA256 integrity checking and edge cases
- [X] T038 [P] Implement rollback manager in src/security/rollback_manager.cpp for failed updates
- [ ] T038a [P] Create integration tests for rollback scenarios and failure recovery
- [X] T039 Setup OTA update workflow in src/network/ota_handler.cpp with GitHub API integration
- [ ] T039a [P] Write integration tests for OTA workflow end-to-end validation
- [X] T040 [P] Add firmware version management and update status tracking
- [ ] T040a [P] Create unit tests for version management and status tracking
- [X] T041 [P] Create update download and installation process with progress reporting
- [ ] T041a [P] Write integration tests for download process and progress validation
- [X] T042 Implement automatic rollback on boot failure or corruption detection
- [ ] T042a [P] Create hardware-in-the-loop tests for boot failure detection
- [X] T043 [P] Add OTA update logging and diagnostic reporting
- [ ] T043a [P] Write unit tests for OTA logging and diagnostic data validation

**Checkpoint**: Secure OTA updates functional with 99% success rate and automatic rollback protection

---

## Phase 7: Integration & Polish

**Goal**: Final integration, optimization, and cross-cutting concerns

### Integration Tasks

- [ ] T044 [P] Implement comprehensive error recovery across all subsystems
- [ ] T044a [P] Create system-wide integration tests for error recovery scenarios
- [ ] T045 [P] Add performance monitoring and metrics collection
- [ ] T045a [P] Write unit tests for performance metrics collection and reporting
- [ ] T046 [P] Optimize memory usage and prevent memory leaks
- [ ] T046a [P] Create memory leak detection tests and usage validation
- [ ] T047 Create comprehensive diagnostic API endpoints per contracts/logging-api.json
- [ ] T047a [P] Write integration tests for all diagnostic API endpoints
- [ ] T048 [P] Implement telemetry data collection and optional remote reporting per FR-007 structured logging
- [ ] T048a [P] Create unit tests for telemetry collection and data validation
- [ ] T049 [P] Add configuration validation and factory reset functionality with JSON schema validation
- [ ] T049a [P] Write unit tests for configuration validation and factory reset
- [ ] T050 [P] Create deployment documentation and hardware setup guide
- [ ] T051 Final integration testing across all user stories and edge cases
- [ ] T051a [P] Execute comprehensive system test suite and validate 80% coverage requirement
- [ ] T052 [P] Performance validation against all success criteria (SC-001 through SC-009)
- [ ] T052a [P] Create automated test suite for all success criteria validation

**Checkpoint**: Complete system ready for deployment with all requirements validated

---

## Dependencies

### Story Completion Order
1. **Setup → Foundational** (blocking)
2. **US1** (independent MVP - can deploy after this)
3. **US2** (depends on US1 LED controller, otherwise independent) 
4. **US3** (depends on US1 sensor framework, otherwise independent)
5. **Security/OTA** (can develop in parallel with US2/US3)
6. **Integration** (depends on all previous phases)

### Parallel Execution Opportunities

#### Phase 2 (Foundational)
- T010, T011, T012, T014, T015 can run simultaneously

#### Phase 3 (US1)  
- T017, T018 can run in parallel
- T019-T023 are sequential on sensor/LED integration

#### Phase 4-6 (US2, US3, Security)
- **US2, US3, and Security can be developed completely in parallel**
- T024-T035 and T036-T043 have no cross-dependencies

#### Phase 7 (Integration)
- T044, T045, T046, T048, T049, T050, T052 can run in parallel

---

## Implementation Strategy

### MVP First (Minimum Viable Product)
- **Phases 1-3** deliver working parking sensor (User Story 1 only)
- Provides immediate value: <100ms LED response to parking detection
- Can be deployed and tested independently

### Incremental Delivery
- **Phase 4**: Add health monitoring and fault detection  
- **Phase 5**: Add power optimization for long-term deployment
- **Phase 6**: Add secure remote update capability
- **Phase 7**: Polish and optimization

### Success Validation
Each user story includes independent test criteria that map to the success criteria (SC-001 through SC-009) from the specification.

---

## Task Summary

- **Total Tasks**: 84 (updated for constitutional testing requirement VI)
- **Setup Tasks**: 12 (8 implementation + 4 testing infrastructure)  
- **Foundational Tasks**: 11 (8 implementation + 3 testing framework)
- **US1 (Basic Parking Detection)**: 14 tasks (7 implementation + 7 testing)
- **US2 (Health Indication)**: 12 tasks (6 implementation + 6 testing)  
- **US3 (Power Management)**: 12 tasks (6 implementation + 6 testing)
- **Security & OTA**: 16 tasks (8 implementation + 8 testing)
- **Integration & Polish**: 17 tasks (9 implementation + 8 testing)

**Testing Coverage**: 32 dedicated test tasks ensure constitutional requirement VI compliance (80% minimum coverage)

**Parallelizable Tasks**: 58 tasks marked with [P] can run independently when prerequisites are met (includes test tasks that can run parallel with implementation)