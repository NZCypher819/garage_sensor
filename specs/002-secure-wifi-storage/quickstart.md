# Quick Start Guide: Secure WiFi Credential Storage

**Feature**: 002-secure-wifi-storage  
**Last Updated**: November 18, 2025

## For Developers

### Prerequisites

- PlatformIO CLI installed
- ESP32-S3 development board
- USB cable for programming and monitoring

### Development Setup

1. **Clone and build the project**
   ```bash
   cd garage-sensorv2
   platformio run --target upload --target monitor
   ```

2. **Run unit tests**
   ```bash
   platformio test -e esp32-s3-devkitc-1 -f test_wifi_credential_manager
   platformio test -e esp32-s3-devkitc-1 -f test_credential_encryption  
   platformio test -e esp32-s3-devkitc-1 -f test_wifi_connection_manager
   ```

3. **Run integration tests**
   ```bash
   platformio test -e esp32-s3-devkitc-1 -f test_setup_portal
   platformio test -e esp32-s3-devkitc-1 -f test_wifi_flow
   ```

### Testing on Fresh Device

1. **Flash firmware to device with no stored credentials**
   ```bash
   # Erase flash to simulate fresh device
   esptool.py --chip esp32s3 --port COM3 erase_flash
   platformio run --target upload
   ```

2. **Verify setup mode activation**
   - Device should create "GarageSensor-XXXXXX" access point
   - Connect to AP with phone/laptop
   - Navigate to http://192.168.4.1/setup
   - Verify setup form loads within 3 seconds

3. **Test credential storage**
   - Enter test WiFi credentials in form
   - Submit form and verify connection attempt
   - Check serial monitor for encryption/storage logs
   - Restart device and verify automatic reconnection

### Development Configuration

**WiFi Credentials for Testing:**
```cpp
// Use these test credentials during development
// Never commit real credentials to git!
#define TEST_SSID "TestNetwork"
#define TEST_PASSWORD "testpassword123"
```

**Debugging Settings:**
```cpp
// Enable verbose WiFi logging in main.cpp
#define WIFI_DEBUG_LEVEL 4
#define CREDENTIAL_DEBUG_ENABLED true
```

### Reset Credentials During Development

```bash
# Method 1: Serial command
echo "RESET_WIFI" > /dev/ttyUSB0

# Method 2: Hold GPIO0 button for 10 seconds

# Method 3: Flash erase  
esptool.py --chip esp32s3 --port COM3 erase_flash
```

---

## For Users

### First Time Setup

1. **Power on your garage sensor**
   - Plug device into power in garage
   - Wait 30 seconds for device to boot

2. **Connect to device WiFi**
   - On phone/laptop, look for "GarageSensor-XXXXXX" network
   - Connect to this network (no password required)
   - If prompted, stay connected even without internet

3. **Configure your home WiFi**
   - Open web browser on connected device
   - Navigate to **192.168.4.1** 
   - Enter your home WiFi name and password
   - Click "Configure WiFi" button

4. **Wait for connection**
   - Device will test your WiFi credentials
   - LED will turn solid green when connected
   - Setup network will automatically disappear
   - Device is now connected to your home WiFi

### Changing WiFi Settings

**When you change your WiFi password or move the device:**

1. **Reset the device**
   - Locate small reset button on device
   - Hold button for 10 seconds until LED starts flashing blue
   - Release button

2. **Repeat setup process**
   - Follow "First Time Setup" steps above
   - Enter new WiFi credentials
   - Wait for solid green LED confirmation

### LED Status Guide

- **Blue Flashing**: Setup mode - connect to device WiFi for configuration
- **Blue Solid**: Connecting to your WiFi network
- **Green Solid**: Successfully connected to WiFi  
- **Red Flashing**: WiFi connection failed - press reset button to try again
- **Off**: Normal operation - device connected and working

### Troubleshooting

**Setup network doesn't appear:**
- Wait 2-3 minutes for device to fully boot
- Power cycle the device
- Make sure you're looking for "GarageSensor-" followed by numbers/letters

**Can't connect to setup network:**  
- Forget/forget the network on your device and retry
- Try connecting with a different phone/laptop
- Power cycle the garage sensor device

**WiFi setup fails:**
- Double-check your WiFi network name (case sensitive)
- Verify your WiFi password is correct  
- Make sure your WiFi uses WPA2 or WPA3 security
- Ensure device is within range of your WiFi router

**Device won't connect after setup:**
- Check if your WiFi network is 2.4GHz (device doesn't support 5GHz)
- Verify router is broadcasting network name (not hidden)
- Try moving device closer to WiFi router temporarily

### Security Notes

- Your WiFi credentials are encrypted and stored only on the device
- No credentials are transmitted to any external servers
- Setup mode automatically times out after 10 minutes for security
- Only someone with physical access can reconfigure WiFi

### Support

If you continue experiencing issues:
1. Note the LED pattern behavior
2. Try the setup process on a different phone/computer  
3. Ensure your home WiFi meets requirements (WPA2/WPA3, 2.4GHz)
4. Contact support with device ID shown in setup portal