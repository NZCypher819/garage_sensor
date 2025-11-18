# Tasks: Secure WiFi Credential Storage

**Input**: Design documents from `/specs/002-secure-wifi-storage/`
**Prerequisites**: plan.md ✅, spec.md ✅, data-model.md ✅, contracts/ ✅

**Tests**: TDD approach explicitly required per constitutional principles - tests included for all components.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

Single embedded project: `src/`, `tests/` at repository root

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [x] T001 Create WiFi module directory structure in src/wifi/
- [x] T002 Create security module directory structure in src/security/
- [x] T003 [P] Create data directory structure for web portal assets in data/
- [x] T004 [P] Create test directory structure in tests/unit/ and tests/integration/
- [x] T005 [P] Add ESPAsyncWebServer@1.2.4 library dependency to platformio.ini
- [x] T006 [P] Add AESLib@2.0.0 encryption library dependency to platformio.ini
- [x] T007 [P] Configure test environment settings in platformio.ini

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T008 Create hardware configuration for WiFi module in src/config/wifi_config.h
- [x] T009 [P] Implement AES-256 encryption utilities in src/security/credential_encryption.h
- [x] T010 [P] Implement AES-256 encryption utilities in src/security/credential_encryption.cpp
- [x] T011 Extend LED controller for WiFi status feedback in src/actuators/status_led_controller.h
- [x] T011a [P] Define LED patterns (OFF=connected, BLUE_FLASH=setup, BLUE_SOLID=connecting, GREEN=connected, RED_FLASH=error) in src/actuators/status_led_controller.h
- [x] T012 Create WiFi credential entity structure in src/wifi/wifi_credential_manager.h
- [x] T013 Implement device configuration management in src/wifi/wifi_credential_manager.cpp

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 3 - Credential Security and Isolation (Priority: P1) 🎯 MVP

**Goal**: Implement secure credential storage with AES-256 encryption and GitHub isolation

**Independent Test**: Verify credentials are stored encrypted in NVS, never appear in plaintext logs or source, and survive unauthorized access attempts

### Tests for User Story 3 ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [x] T014 [P] [US3] Unit test for credential encryption/decryption in tests/unit/test_credential_encryption.cpp
- [x] T015 [P] [US3] Unit test for NVS storage operations in tests/unit/test_nvs_storage.cpp
- [x] T016 [P] [US3] Security test for credential isolation in tests/integration/test_credential_security.cpp

### Implementation for User Story 3

- [x] T017 [P] [US3] Implement SecurityContext entity in src/security/credential_encryption.cpp
- [x] T018 [US3] Implement secure credential storage methods in src/wifi/wifi_credential_manager.cpp
- [x] T019 [US3] Add credential validation and integrity checking in src/wifi/wifi_credential_manager.cpp
- [x] T020 [US3] Implement secure credential wiping functionality in src/wifi/wifi_credential_manager.cpp
- [x] T021 [US3] Add encryption error handling and recovery in src/wifi/wifi_credential_manager.cpp
- [x] T021a [P] [US3] Add WPA2/WPA3 protocol support validation in src/wifi/wifi_connection_manager.cpp

**Checkpoint**: At this point, User Story 3 should be fully functional and testable independently

---

## Phase 4: User Story 1 - Initial WiFi Setup via Web Portal (Priority: P1)

**Goal**: Implement temporary access point and web portal for initial WiFi credential configuration

**Independent Test**: Power on fresh device, connect to its AP, enter credentials via web interface, confirm device connects to home WiFi

### Tests for User Story 1 ⚠️

- [x] T022 [P] [US1] Unit test for setup portal HTTP endpoints in tests/unit/test_setup_portal.cpp
- [x] T023 [P] [US1] Integration test for complete setup flow in tests/integration/test_wifi_setup_flow.cpp
- [x] T024 [P] [US1] Unit test for connection manager state transitions in tests/unit/test_wifi_connection_manager.cpp

### Implementation for User Story 1

- [x] T025 [P] [US1] Create HTML setup portal form in data/setup_portal.html
- [x] T026 [P] [US1] Create CSS styling for setup portal in data/setup_portal.css
- [x] T027 [P] [US1] Create JavaScript form validation in data/setup_portal.js
- [x] T028 [US1] Implement SetupSession entity in src/wifi/wifi_setup_portal.h
- [x] T029 [US1] Implement web server setup portal in src/wifi/wifi_setup_portal.cpp
- [x] T030 [US1] Create WiFiConnectionManager in src/wifi/wifi_connection_manager.h
- [x] T031 [US1] Implement connection management logic in src/wifi/wifi_connection_manager.cpp
- [x] T032 [US1] Implement temporary access point creation in src/wifi/wifi_connection_manager.cpp
- [x] T033 [US1] Add automatic WiFi connection on boot in src/wifi/wifi_connection_manager.cpp
- [x] T034 [US1] Integrate LED feedback with connection states in src/wifi/wifi_connection_manager.cpp

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 5: User Story 2 - WiFi Credential Update via Physical Button (Priority: P2)

**Goal**: Implement physical reset capability to update WiFi credentials when needed

**Independent Test**: Hold button sequence on configured device, confirm it enters setup mode, successfully update credentials

### Tests for User Story 2 ⚠️

- [ ] T035 [P] [US2] Unit test for physical reset button handling in tests/unit/test_physical_reset.cpp
- [ ] T036 [P] [US2] Integration test for credential update flow in tests/integration/test_credential_update.cpp

### Implementation for User Story 2

- [ ] T037 [US2] Implement physical reset button handler in src/wifi/wifi_connection_manager.cpp
- [ ] T038 [US2] Add credential overwrite functionality in src/wifi/wifi_credential_manager.cpp
- [ ] T039 [US2] Implement setup mode timeout handling in src/wifi/wifi_setup_portal.cpp
- [ ] T040 [US2] Add reset button debouncing logic in src/wifi/wifi_connection_manager.cpp

**Checkpoint**: All user stories should now be independently functional

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [x] T041 [P] Update main.cpp integration with WiFi credential system in src/main.cpp
- [ ] T041a [P] Add integration test for main.cpp WiFi system initialization in tests/integration/test_main_wifi_integration.cpp
- [ ] T042 [P] Add comprehensive error logging across all WiFi components with structured format (timestamp, component, level, message)
- [ ] T042a [P] Add unit test for error logging validation in tests/unit/test_wifi_error_logging.cpp
- [ ] T043 [P] Performance optimization for connection timeout handling
- [ ] T044 [P] Update quickstart.md with final testing scenarios in specs/002-secure-wifi-storage/quickstart.md
- [ ] T045 [P] Add memory usage monitoring and optimization across WiFi system
- [x] T046 Run final quickstart.md validation scenarios (Code-level complete, hardware testing pending - See VALIDATION_REPORT.md)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3)
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 3 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 1 (P1)**: Depends on User Story 3 completion (needs secure credential storage)
- **User Story 2 (P2)**: Depends on both User Story 3 and User Story 1 completion

### Within Each User Story

- Tests MUST be written and FAIL before implementation
- Models/entities before services
- Services before web interfaces
- Core implementation before integration
- Story complete before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel
- All Foundational tasks marked [P] can run in parallel (within Phase 2)
- User Story 3 implementation can proceed after foundational completion
- All tests for a user story marked [P] can run in parallel
- Models within a story marked [P] can run in parallel

---

## Parallel Example: User Story 1

```bash
# Launch all tests for User Story 1 together:
Task: "Unit test for setup portal HTTP endpoints in tests/unit/test_setup_portal.cpp"
Task: "Integration test for complete setup flow in tests/integration/test_wifi_setup_flow.cpp" 
Task: "Unit test for connection manager state transitions in tests/unit/test_wifi_connection_manager.cpp"

# Launch all web assets for User Story 1 together:
Task: "Create HTML setup portal form in data/setup_portal.html"
Task: "Create CSS styling for setup portal in data/setup_portal.css"
Task: "Create JavaScript form validation in data/setup_portal.js"
```

---

## Implementation Strategy

### MVP First (User Story 3 + User Story 1)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 3 (Credential Security)
4. Complete Phase 4: User Story 1 (WiFi Setup Portal)
5. **STOP and VALIDATE**: Test complete WiFi setup flow independently
6. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 3 → Test credential security independently
3. Add User Story 1 → Test complete setup flow independently → Deploy/Demo (MVP!)
4. Add User Story 2 → Test credential updates independently → Deploy/Demo
5. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 3 (Security - foundational for others)
   - Prepare for User Story 1 and 2 development
3. After User Story 3:
   - Developer A: User Story 1 (Setup Portal)
   - Developer B: User Story 2 (Physical Reset)

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Security (US3) must complete before setup portal (US1) can function
- Physical reset (US2) requires both security and setup portal to be complete
- FR-007 merged into FR-003 for consolidated credential validation
- Test coverage >80% required per constitutional principle VI