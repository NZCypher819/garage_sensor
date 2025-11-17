# Data Model: Garage Parking Position Sensor

**Feature**: Garage Parking Position Sensor  
**Date**: 2025-11-17  
**Status**: Complete

## Core Entities

### Parking Event
Represents a change in beam state (vehicle entering/leaving optimal position)

**Fields**:
- `timestamp`: ISO 8601 timestamp when event occurred
- `event_type`: enum ["beam_broken", "beam_restored"] 
- `sensor_id`: string identifier for sensor (supports multi-sensor future)
- `response_time_ms`: integer - measured time from sensor trigger to LED response
- `signal_quality`: float 0.0-1.0 - sensor signal strength/reliability indicator

**Validation Rules**:
- `timestamp` must be valid ISO 8601 format
- `event_type` must be one of allowed enum values
- `response_time_ms` must be 0-1000 (sanity check, target <100ms)
- `signal_quality` must be 0.0-1.0 range

**State Transitions**:
- `beam_broken` → `beam_restored` (normal parking cycle)
- Multiple `beam_broken` events indicate sensor issues (should not occur)

### System Status  
Current operational state and health of the parking sensor system

**Fields**:
- `timestamp`: ISO 8601 timestamp of last status update
- `operational_state`: enum ["initializing", "ready", "low_power", "fault", "updating"]
- `wifi_connected`: boolean - WiFi connection status
- `wifi_signal_strength`: integer dBm - WiFi RSSI value
- `power_mode`: enum ["active", "light_sleep", "deep_sleep"]
- `uptime_seconds`: unsigned integer - time since last boot
- `free_memory_bytes`: unsigned integer - available heap memory
- `sensor_health`: enum ["healthy", "degraded", "failed"]
- `led_functional`: boolean - LED operational status
- `last_parking_event`: timestamp - when last parking event occurred
- `firmware_version`: string - current firmware version
- `ota_available`: boolean - whether firmware update is available
- `last_update_check`: timestamp - when OTA update was last checked
- `update_status`: enum ["idle", "checking", "downloading", "installing", "rollback"]

**Validation Rules**:
- All enum fields must contain valid values
- `wifi_signal_strength` range -100 to 0 dBm
- `uptime_seconds` must be positive
- `free_memory_bytes` must be positive and < total memory
- `firmware_version` must follow semantic versioning (x.y.z)

### OTA Update Record
Information about firmware update attempts and status

**Fields**:
- `update_id`: string - unique identifier for update attempt
- `timestamp`: ISO 8601 timestamp when update was initiated
- `source_version`: string - firmware version before update
- `target_version`: string - firmware version being installed
- `download_url`: string - GitHub release download URL
- `file_size_bytes`: integer - expected firmware file size
- `download_progress`: float 0.0-1.0 - download completion percentage
- `status`: enum ["pending", "downloading", "verifying", "installing", "completed", "failed", "rolled_back"]
- `error_message`: string - error description if update failed
- `integrity_verified`: boolean - whether file integrity check passed
- `install_duration_ms`: integer - time taken for installation
- `rollback_reason`: string - reason for rollback if applicable

**Validation Rules**:
- `update_id` must be unique per update attempt
- `source_version` and `target_version` must be valid semantic versions
- `download_progress` must be 0.0-1.0 range
- `file_size_bytes` must be positive
- `status` must progress logically (no invalid state transitions)

**State Transitions**:
- `pending` → `downloading` → `verifying` → `installing` → `completed`
- Any state → `failed` → `rolled_back` (error recovery path)
- `failed` → `pending` (retry attempt)

### Sensor Reading
Raw data from E3JK-RR11 infrared beam sensor

**Fields**:
- `timestamp`: ISO 8601 timestamp of reading
- `beam_state`: enum ["intact", "broken"]
- `raw_signal`: boolean - direct GPIO pin state
- `debounced_signal`: boolean - signal after hardware debouncing  
- `validation_passed`: boolean - whether reading passed software validation
- `consecutive_readings`: integer - number of consecutive similar readings
- `noise_detected`: boolean - whether electrical interference detected

**Validation Rules**:
- `beam_state` derived from `debounced_signal` and `validation_passed`
- `consecutive_readings` must be positive
- `validation_passed` false triggers sensor health degradation
- Readings older than 1 second are considered stale

**State Transitions**:
- `intact` → `broken`: Vehicle enters beam, triggers parking event
- `broken` → `intact`: Vehicle exits beam, triggers restore event  
- Rapid state changes indicate sensor noise/interference

## Configuration Data

### Device Configuration
Persistent settings stored in EEPROM/SPIFFS

**Fields**:
- `device_id`: string - unique identifier for this sensor
- `wifi_ssid`: string - network name (encrypted storage)
- `wifi_password`: string - network password (encrypted storage)
- `ota_server_url`: string - GitHub releases URL or custom server
- `led_brightness`: integer 0-255 - LED PWM brightness level
- `power_save_timeout_seconds`: integer - idle time before entering low power
- `sensor_debounce_ms`: integer - hardware debouncing duration
- `response_time_target_ms`: integer - target response time (default 100)
- `log_level`: enum ["debug", "info", "warn", "error"]
- `telemetry_enabled`: boolean - whether to send data to remote server
- `health_check_interval_seconds`: integer - status update frequency

**Validation Rules**:
- `device_id` must be unique and non-empty
- `wifi_ssid` and `wifi_password` required for connectivity
- `led_brightness` range 0-255
- All timeout values must be positive
- `log_level` must be valid enum value

## Data Relationships

```
Device Configuration (1) ── manages ──→ (1) System Status
                                     ↘
System Status (1) ── tracks ──→ (many) Parking Events
                 ↘
                  └── monitors ──→ (many) Sensor Readings

Sensor Readings (many) ── trigger ──→ (many) Parking Events
```

## Storage Strategy

### Local Storage (SPIFFS)
- Configuration: `/config.json` (encrypted)
- Event log: `/events.log` (rolling, max 1MB)
- Status history: `/status.log` (last 24 hours)
- Certificates: `/certs/` (OTA verification)

### Memory Management
- Current readings: In-memory circular buffer (last 100 readings)
- Event cache: Last 50 parking events in SRAM
- Status: Single current status object in SRAM
- Logs: Buffered writes to SPIFFS (flush every 10 events or 60 seconds)

## Data Integrity

### Validation Layers
1. **Hardware**: Sensor signal conditioning and debouncing
2. **Software**: Range checking and consistency validation  
3. **Application**: Business logic validation (parking event patterns)
4. **Storage**: Checksums and atomic writes for persistence

### Error Handling
- Invalid sensor readings → mark as failed, continue with degraded status
- Storage failures → fallback to in-memory only with alerts
- Network issues → queue telemetry for later transmission
- Memory exhaustion → purge oldest data, maintain core functionality

This data model supports all constitutional requirements for reliability, observability, and data integrity while enabling the responsive (<100ms) parking detection functionality.