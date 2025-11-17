# Quickstart: Garage Parking Position Sensor

**Project**: ESP32 S3 Nano Parking Sensor with E3JK-RR11 IR Beam Sensor  
**Platform**: PlatformIO  
**Target**: Real-time parking detection with <100ms LED response

## Prerequisites

### Hardware Requirements
- **ESP32-S3-NANO** development board
- **E3JK-RR11** infrared beam sensor (photoelectric sensor)
- **LED** (high-brightness recommended)
- **Resistors**: 220Ω for LED current limiting
- **Power supply**: 5V (sensor) + 3.3V (ESP32) or single 5V with voltage regulator
- **Enclosure**: IP65-rated for garage environment
- **Mounting hardware**: Brackets for sensor alignment

### Software Requirements
- **PlatformIO IDE** (VS Code extension)
- **Python 3.7+** (for PlatformIO)
- **Git** (for version control and OTA updates)
- **WiFi network** (2.4GHz) with internet access

## Quick Setup (5 minutes)

### 1. Clone and Initialize Project
```bash
git clone <repository-url>
cd garage-sensorv2
```

### 2. Open in PlatformIO
```bash
code .
# PlatformIO will auto-detect the project
```

### 3. Hardware Wiring
```
ESP32-S3-NANO    →    E3JK-RR11 Sensor
GPIO 2           →    Signal Output (Brown wire)
3.3V             →    VCC (Red wire)  
GND              →    GND (Blue wire)

ESP32-S3-NANO    →    Parking LED
GPIO 8           →    LED Anode (via 220Ω resistor)
GND              →    LED Cathode

ESP32-S3-NANO    →    Status LED
GPIO 9           →    LED Anode (via 220Ω resistor)
GND              →    LED Cathode
```

### 4. Configure WiFi and Security
Edit `data/config.json`:
```json
{
  "wifi_ssid": "YOUR_NETWORK_NAME",
  "wifi_password": "YOUR_NETWORK_PASSWORD", 
  "device_id": "garage-sensor-01",
  "ota_server": "https://github.com/your-username/garage-sensorv2/releases",
  "security": {
    "https_enabled": true,
    "cert_validation": true,
    "download_timeout_seconds": 300,
    "integrity_check_enabled": true,
    "rollback_enabled": true
  }
}
```

### 5. Build and Upload
```bash
# Upload filesystem (config files)
pio run --target uploadfs

# Build and upload firmware 
pio run --target upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
garage-sensorv2/
├── platformio.ini          # PlatformIO configuration
├── src/                     # Source code
│   ├── main.cpp            # Application entry point
│   ├── sensors/            # E3JK-RR11 sensor driver
│   ├── actuators/          # Parking and Status LED controllers
│   ├── network/            # WiFi and OTA management
│   ├── power/              # Power management
│   └── diagnostics/        # Logging and health monitoring
├── lib/                    # External libraries
├── test/                   # Unit and integration tests  
├── data/                   # SPIFFS filesystem content
│   ├── config.json         # Device configuration
│   └── certs/             # OTA certificates
└── specs/                  # Feature specifications
    └── 001-parking-sensor/ # Current feature
```

## Configuration Options

### Device Settings (`data/config.json`)
```json
{
  "device_id": "garage-sensor-01",
  "wifi_ssid": "your_network",
  "wifi_password": "your_password",
  "led_brightness": 128,
  "sensor_debounce_ms": 50,
  "power_save_timeout": 30,
  "response_time_target": 100,
  "log_level": "info",
  "telemetry_enabled": true,
  "ota_server": "https://github.com/owner/repo/releases"
}
```

### Hardware Pins (`src/config/hardware.h`)
```cpp
#define SENSOR_GPIO_PIN      2    // E3JK-RR11 signal input
#define PARKING_LED_GPIO     8    // Parking indication LED
#define STATUS_LED_GPIO      9    // System status LED
#define PARKING_LED_PWM_CH   0    // PWM channel for parking LED
#define STATUS_LED_PWM_CH    1    // PWM channel for status LED
#define SENSOR_DEBOUNCE_MS   50   // Hardware debouncing
```

## Testing Your Setup

### 1. Power-On Test
- LED should blink twice on startup (healthy initialization)
- Check serial monitor for WiFi connection status
- LED should remain dimly lit (system ready)

### 2. Sensor Functionality Test
```bash
# Break the IR beam with your hand
# LED should illuminate immediately (<100ms)
# Restore beam - LED should turn off immediately
```

### 3. Response Time Validation
```bash
# Monitor serial output while triggering sensor
# Look for response time measurements:
# [INFO] Parking event: beam_broken, response_time: 45ms
```

### 4. WiFi and OTA Test
```bash
# Check device status via web interface
curl http://<device_ip>/api/v1/status

# Check for updates
curl -X POST http://<device_ip>/api/v1/ota/check
```

## Development Workflow

### 1. Make Changes
- Edit source files in `src/`
- Update configuration in `data/config.json` if needed
- Add tests in `test/` directory

### 2. Test Locally
```bash
# Run unit tests
pio test

# Build without uploading (check compilation)
pio run

# Upload and monitor
pio run --target upload && pio device monitor
```

### 3. Deploy Updates
```bash
# Build release firmware
pio run --environment production

# Tag release
git tag v1.0.1
git push origin v1.0.1

# Device will auto-update via OTA
```

## Troubleshooting

### Common Issues

**LED not responding to sensor:**
- Check wiring: GPIO 2 → sensor signal, GPIO 8 → LED
- Verify sensor power: 5V to red wire, GND to blue wire
- Test sensor manually: should switch between 0V/5V when beam breaks

**WiFi connection fails:**
- Check SSID/password in `config.json`
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Verify signal strength in garage location

**Slow response times (>100ms):**
- Reduce `sensor_debounce_ms` setting
- Check for interference from garage door opener
- Verify sensor alignment and cleanliness

**OTA updates fail:**
- Check internet connectivity
- Verify GitHub repository access and release availability
- Ensure sufficient free memory (>1MB)
- Check HTTPS certificate validity (GitHub's CA must be trusted)
- Verify firmware file integrity with SHA256 checksum

### Diagnostic Commands
```bash
# View real-time logs
pio device monitor --filter esp32_exception_decoder

# Check system health
curl http://<device_ip>/api/v1/health

# Download diagnostic logs  
curl http://<device_ip>/api/v1/logs > sensor_logs.json
```

### Performance Monitoring
- Target response time: <100ms (99.9% of events)
- Idle power consumption: <10mA
- WiFi signal strength: >-70 dBm for reliable OTA
- Error rate: <0.1% false positives

## Next Steps

1. **Physical Installation**: Mount sensor at bumper height, align beam across parking path
2. **Calibration**: Adjust sensor position for optimal vehicle detection point
3. **Monitoring**: Set up remote telemetry dashboard (optional)
4. **Maintenance**: Schedule monthly alignment checks and quarterly firmware updates

For detailed technical information, see the [implementation plan](plan.md) and [data model](data-model.md) documentation.