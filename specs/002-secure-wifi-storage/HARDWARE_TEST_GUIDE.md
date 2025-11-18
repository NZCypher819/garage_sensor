# Hardware Testing Quick Start Guide
**Feature**: 002-secure-wifi-storage  
**Date**: November 18, 2025

## Prerequisites
- ESP32-S3-DevKitC-1 board
- USB cable
- PlatformIO CLI installed
- WiFi network (2.4GHz, WPA2/WPA3)

## Step 1: Flash Fresh Device

```bash
# Navigate to project directory
cd C:\Repos\garage-sensorv2

# Erase flash to simulate fresh device (IMPORTANT!)
python -m platformio run --target erase

# Or use esptool directly
esptool.py --chip esp32s3 --port COM3 erase_flash

# Upload firmware and start serial monitor
python -m platformio run --target upload --target monitor
```

## Step 2: Verify Fresh Device Setup

**Expected Serial Output:**
```
=== Garage Sensor Initialization ===
✓ Foundation initialized
=== WiFi System Initialization ===
WiFi Credential Manager: Initialized successfully
WiFi Connection Manager: Initialized successfully
No stored credentials found
✓ Setup mode activated
✓ WiFi setup portal started
=== WiFi Setup Instructions ===
1. Connect to WiFi network: GarageSensor-XXXXXX
2. Open browser to: http://192.168.4.1/setup
3. Enter your home WiFi credentials
4. Setup session expires in 10 minutes
================================
```

**Action Items:**
- [ ] Verify "GarageSensor-XXXXXX" appears in WiFi networks
- [ ] Note the last 6 hex digits (should match device MAC)
- [ ] LED should be flashing BLUE (setup mode)

## Step 3: Connect to Setup Portal

**On Phone/Laptop:**
1. Open WiFi settings
2. Connect to "GarageSensor-XXXXXX" (no password)
3. Stay connected (may show "No Internet")
4. Open browser to: http://192.168.4.1/setup

**Expected:**
- [ ] Page loads within 3 seconds
- [ ] Setup form with SSID and Password fields visible
- [ ] CSS styling applied correctly
- [ ] "Configure WiFi" button present

## Step 4: Submit WiFi Credentials

**In Setup Form:**
1. Enter your WiFi SSID (case-sensitive!)
2. Enter your WiFi password
3. Click "Configure WiFi"

**Expected Serial Output:**
```
WiFi Setup Portal: Starting credential validation for SSID: YourNetwork
WiFi Connection Manager: Validating credentials for SSID: YourNetwork
WiFi Connection Manager: Validating security for SSID: YourNetwork
WiFi Credential Manager: Credentials stored successfully
WiFi Connection Manager: Credential validation successful
```

**Action Items:**
- [ ] Verify credentials accepted (no plaintext in serial logs!)
- [ ] LED should turn BLUE SOLID (connecting)
- [ ] LED should turn GREEN SOLID (connected) within 30 seconds
- [ ] Setup AP "GarageSensor-XXXXXX" should disappear

## Step 5: Verify Auto-Connect

**Power Cycle Device:**
```bash
# Disconnect/reconnect USB or press reset button
```

**Expected Serial Output:**
```
=== WiFi System Initialization ===
WiFi Credential Manager: Initialized successfully
WiFi Connection Manager: Initialized successfully
Stored credentials found
WiFi Connection Manager: Attempting connection to stored network: YourNetwork
✓ WiFi connected
```

**Action Items:**
- [ ] Device connects within 30 seconds
- [ ] No user interaction required
- [ ] LED GREEN SOLID (connected)
- [ ] No setup AP created

## Step 6: Test WPA Security Validation

**Manual Test (Optional):**
1. Create a WPA (TKIP) network or WEP network
2. Try to connect via setup portal
3. Verify rejection with error message

**Expected:**
```
WiFi Connection Manager: Unsupported security protocol
WiFi Setup Portal: Error response sent - Unsupported security protocol
```

## Step 7: Test Physical Reset

**With Device Connected:**
1. Hold GPIO0 button (usually labeled "BOOT")
2. Keep holding for 10 seconds
3. Release button

**Expected Serial Output:**
```
WiFi Connection Manager: Reset button pressed
WiFi Connection Manager: Physical reset triggered
WiFi Credential Manager: Credentials cleared successfully
WiFi Connection Manager: Entering setup mode
```

**Action Items:**
- [ ] Device enters setup mode after 10-second hold
- [ ] Setup AP reappears
- [ ] LED switches to BLUE FLASHING
- [ ] Can configure new WiFi credentials

## Step 8: Test Session Timeout (10 Minutes)

**Long Test (Optional):**
1. Enter setup mode (fresh device or reset)
2. Wait 10 minutes WITHOUT submitting credentials
3. Observe behavior

**Expected:**
- Session times out after exactly 10 minutes
- New session created automatically
- Setup mode remains active
- Session ID changes (visible in setup portal)

## Troubleshooting

### Setup AP Doesn't Appear
- Wait 2-3 minutes for full boot
- Check serial monitor for errors
- Verify flash was erased
- Power cycle device

### Can't Connect to Setup AP
- Forget network on phone/laptop
- Try different device
- Check distance to ESP32

### Portal Page Won't Load
- Verify connected to correct AP
- Try http://192.168.4.1 (not https)
- Check SPIFFS files uploaded: `pio run --target uploadfs`
- Review serial monitor for SPIFFS mount errors

### WiFi Connection Fails
- Verify SSID is exact (case-sensitive)
- Check password is correct
- Ensure WiFi is 2.4GHz (not 5GHz)
- Verify WPA2/WPA3 security (not WEP/WPA-TKIP)
- Check signal strength

### Auto-Connect Fails After Restart
- Verify credentials were stored (check serial log)
- Check WiFi network is available
- Review NVS partition status
- Try manual connection via setup portal

## Serial Monitor Commands

**View Logs:**
```bash
python -m platformio device monitor --baud 115200
```

**Filter WiFi Logs:**
```bash
python -m platformio device monitor | Select-String "WiFi"
```

## Validation Checklist

- [ ] Fresh device setup flow complete
- [ ] Setup portal accessible and responsive
- [ ] WiFi credentials stored with encryption
- [ ] Auto-connect on device restart works
- [ ] WPA2/WPA3 security validation active
- [ ] Session timeout handling correct
- [ ] LED patterns match specifications
- [ ] Physical reset clears credentials
- [ ] No plaintext credentials in serial logs

## Performance Benchmarks

Record actual measurements:
- Setup portal load time: _______ seconds (target: <3s)
- WiFi connection time: _______ seconds (target: <30s)
- Credential encryption time: _______ ms (target: <100ms)
- Memory usage: _______ % (target: <10% overhead)

## Notes

**Record Any Issues:**
- 
- 
- 

**Success Criteria:**
✅ All 10 validation scenarios pass
✅ No critical errors in serial logs
✅ Performance within constitutional limits
✅ User experience smooth and intuitive

---

**Tested By**: _________________  
**Date**: _________________  
**Device ID**: _________________  
**Firmware Version**: 002-secure-wifi-storage  
**Result**: ⬜ PASS | ⬜ FAIL | ⬜ PARTIAL
