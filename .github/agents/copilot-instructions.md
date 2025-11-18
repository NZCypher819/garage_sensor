# garage-sensorv2 Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-11-17

## Active Technologies
- C++ (Arduino framework via PlatformIO, ESP-IDF core) + WiFi library, ArduinoOTA, Arduino JSON, ESP32 deep sleep libraries, HTTPS client library (001-parking-sensor)
- SPIFFS/LittleFS for configuration, EEPROM for persistent settings, local logging (001-parking-sensor)
- C++ with Arduino framework for ESP32 + Arduino WiFi, ESP32 NVS, WebServer, AES encryption library (002-secure-wifi-storage)
- ESP32 Non-Volatile Storage (NVS) partition for encrypted credentials (002-secure-wifi-storage)
- C++ (Arduino framework for ESP32) + ESP32 Arduino Core, ESPAsyncWebServer, AESLib for encryption (002-secure-wifi-storage)
- ESP32 NVS (Non-Volatile Storage) partition for encrypted credential persistence (002-secure-wifi-storage)

- C++ (Arduino framework via PlatformIO, ESP-IDF core) + WiFi library, ArduinoOTA, Arduino JSON, ESP32 deep sleep libraries (001-parking-sensor)

## Project Structure

```text
src/
tests/
```

## Commands

# Add commands for C++ (Arduino framework via PlatformIO, ESP-IDF core)

## Code Style

C++ (Arduino framework via PlatformIO, ESP-IDF core): Follow standard conventions

## Recent Changes
- 002-secure-wifi-storage: Added C++ (Arduino framework for ESP32) + ESP32 Arduino Core, ESPAsyncWebServer, AESLib for encryption
- 002-secure-wifi-storage: Added C++ with Arduino framework for ESP32 + Arduino WiFi, ESP32 NVS, WebServer, AES encryption library
- 001-parking-sensor: Added C++ (Arduino framework via PlatformIO, ESP-IDF core) + WiFi library, ArduinoOTA, Arduino JSON, ESP32 deep sleep libraries, HTTPS client library


<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
