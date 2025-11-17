# Garage Parking Sensor v1.0.0

🚗 **ESP32-S3-NANO based parking sensor with secure OTA updates**

[![Build and Release](https://github.com/your-username/garage-sensorv2/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/your-username/garage-sensorv2/actions/workflows/build-and-release.yml)
[![Security Scan](https://github.com/your-username/garage-sensorv2/actions/workflows/security-scan.yml/badge.svg)](https://github.com/your-username/garage-sensorv2/actions/workflows/security-scan.yml)
[![Test Coverage](https://codecov.io/gh/your-username/garage-sensorv2/branch/main/graph/badge.svg)](https://codecov.io/gh/your-username/garage-sensorv2)

## Features

### Core Functionality
- **Real-time parking detection** using E3JK-RR11 infrared sensor
- **Instant feedback** with dual LED status indication (<100ms response)
- **Power efficient** operation with <10mA idle consumption
- **Enterprise-grade reliability** with comprehensive health monitoring

### Security & Updates
- **🔒 Secure OTA updates** with HTTPS and certificate validation
- **🛡️ SHA256 integrity validation** for all firmware downloads
- **🔄 Automatic rollback protection** on update failures
- **📋 Comprehensive logging** of all OTA operations
- **📊 Version management** with build tracking and statistics

## Quick Start

### Hardware Setup
```
ESP32-S3-NANO Connections:
├── GPIO 4  → E3JK-RR11 Signal (White wire - NO contact)
├── GPIO 15 → Parking LED (Green/Red indication)  
├── GPIO 16 → Status LED (System health indication)
├── 5V      → E3JK-RR11 Power (Brown wire)
├── GND     → E3JK-RR11 Ground (Blue wire)
└── USB-C   → Programming/Power
```

### Initial Deployment

1. **Flash initial firmware:**
   ```bash
   # Clone repository
   git clone https://github.com/your-username/garage-sensorv2.git
   cd garage-sensorv2
   
   # Install PlatformIO
   pip install platformio
   
   # Build and upload
   pio run --target upload
   ```

2. **Configure WiFi (first boot):**
   - Device creates setup hotspot: `GarageSensor-Setup`
   - Connect and configure WiFi credentials
   - Device automatically restarts and connects

3. **Verify operation:**
   - Status LED shows startup sequence (blue flashing)
   - Green status indicates healthy operation
   - Parking LED responds to sensor state

## OTA Updates

### Automatic Updates
The device checks for updates every 24 hours and displays available updates via status LED patterns.

### Manual Update Trigger
```bash
# Via web interface (when WiFi connected)
curl -X POST http://device-ip/api/v1/ota/check

# Or via serial command
echo "check_updates" > /dev/ttyUSB0
```

### GitHub Releases
Firmware releases are automatically built and published:
- **Stable releases**: Tagged as `v1.0.0`, `v1.1.0`, etc.
- **Beta releases**: Tagged as `v1.0.0-beta.1`, etc.
- **Development**: Available via Actions artifacts

### Security Features
- ✅ **HTTPS Downloads**: All firmware downloaded over encrypted connections
- ✅ **Certificate Validation**: DigiCert root CA validation
- ✅ **SHA256 Verification**: Cryptographic integrity validation
- ✅ **Rollback Protection**: Automatic recovery from failed updates
- ✅ **Boot Validation**: Failed boots trigger automatic rollback

## API Documentation

The device exposes REST APIs for monitoring and management:

- **Device Status**: `GET /api/v1/status`
- **Configuration**: `GET/PUT /api/v1/config`  
- **OTA Check**: `POST /api/v1/ota/check`
- **OTA Update**: `POST /api/v1/ota/update`

See [`contracts/wifi-api.json`](specs/001-parking-sensor/contracts/wifi-api.json) for complete API specification.

## PlatformIO Project Structure

```
garage-sensorv2/
├── platformio.ini          # PlatformIO configuration with testing environments
├── src/                    # Source code
├── test/                   # Unit and integration tests
├── data/                   # SPIFFS filesystem content
├── lib/                    # External libraries
└── scripts/               # Build and coverage scripts
```

## Build & Test Commands

```bash
# Build for ESP32-S3-NANO
pio run

# Upload firmware
pio run --target upload

# Upload filesystem
pio run --target uploadfs

# Run unit tests (native)
pio test -e test_native

# Run embedded tests
pio test -e test_embedded

# Run with coverage measurement
pio test -e test_coverage

# Monitor serial output
pio device monitor
```

## Development Environment

- **Platform**: ESP32-S3-NANO development board
- **Framework**: Arduino via ESP-IDF core
- **Testing**: PlatformIO Unity framework
- **Coverage**: GCC gcov with lcov reporting (80% minimum per constitution)

## Constitutional Compliance

This project implements all six constitutional principles:
- **I. Reliability-First**: Sensor validation, error handling, graceful degradation
- **II. Power-Aware Design**: <10mA idle consumption with sleep management
- **III. Specification-Driven**: Complete speckit workflow implementation
- **IV. Secure-by-Design**: HTTPS OTA updates with integrity validation
- **V. Observability**: Structured logging and health monitoring
- **VI. Test-Driven Development**: 32 dedicated test tasks with 80% coverage mandate