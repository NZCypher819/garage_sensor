<!--
Sync Impact Report:
- Version change: 1.0.0 → 1.1.0 (added VI. Test-Driven Development principle)
- Added sections: Principle VI with testing requirements, updated Development Workflow
- Templates requiring updates: ✅ plan.md templates need constitution check for testing principle
- Follow-up TODOs: Review existing tasks.md to ensure test tasks are included per new principle
-->

# Garage Sensor V2 Constitution

## Core Principles

### I. Reliability-First
Hardware and software must prioritize reliability over features. All sensor readings MUST be validated and error-handled. System MUST gracefully degrade when components fail. Data integrity is non-negotiable - corrupted readings are worse than no readings.

**Rationale**: IoT sensor systems operate unattended and must function reliably in varying environmental conditions. False readings can trigger incorrect actions.

### II. Power-Aware Design
All features MUST consider power consumption impact. Battery-powered components require explicit power budgeting. Sleep modes and efficient communication protocols are mandatory. Power consumption must be measured and documented for each feature.

**Rationale**: Garage sensors often run on battery power and must operate for months without intervention. Poor power management leads to frequent maintenance and system unreliability.

### III. Specification-Driven Development
All features start with user scenarios in spec.md. Technical plans in plan.md are mandatory before implementation. User stories must be independently testable and prioritized. No code without approved specifications.

**Rationale**: IoT systems have complex integration requirements. Clear specifications prevent scope creep and ensure features solve real user problems.

### IV. Secure-by-Design
All communication MUST use encryption. Device authentication is mandatory. Local network segmentation required. Regular security audits and updates are non-negotiable. Default credentials prohibited.

**Rationale**: IoT devices are common attack vectors. Garage sensors may control physical access and must maintain security even when compromised networks exist.

### V. Observability and Diagnostics
Structured logging at all system levels is mandatory. Device health metrics must be exposed. Remote diagnostics capability required. Error states must be clearly communicated to users and systems.

**Rationale**: Remote sensor systems are difficult to debug physically. Comprehensive observability enables rapid issue resolution and predictive maintenance.

### VI. Test-Driven Development
Every function, module, and feature MUST have automated tests. Unit tests are mandatory for all business logic. Integration tests required for hardware interfaces and communication protocols. Test coverage must be measured and maintained above 80%. No code may be deployed without passing tests.

**Rationale**: IoT devices operate autonomously in remote environments where debugging is difficult. Comprehensive automated testing prevents runtime failures, ensures reliability under varying conditions, and enables confident remote updates. Untested code in critical systems is a liability.

## Hardware Standards

All sensor hardware MUST meet IP65 rating minimum for garage environments. Operating temperature range: -20°C to 60°C. Communication range testing required in realistic conditions. Electromagnetic interference testing mandatory for garage door opener compatibility.

## Development Workflow

Feature implementation follows speckit workflow: specify → plan → tasks → implement → test → checklist. Each user story must be independently deployable and testable. Automated test suites must be written alongside implementation code. Code reviews mandatory for all changes. Integration testing required for sensor communication and power management changes. Test coverage reports must accompany all pull requests.

## Governance

This constitution supersedes all other development practices. Amendments require documented justification and migration plan. All pull requests must verify compliance with applicable principles. Complexity must be justified against reliability requirements.

**Version**: 1.1.0 | **Ratified**: 2025-11-17 | **Last Amended**: 2025-11-17
