# Setup Portal REST API Contract

**Feature**: 002-secure-wifi-storage  
**API Version**: 1.0  
**Base URL**: http://192.168.4.1 (device access point)

## Overview

REST API for WiFi credential setup served by ESP32 during setup mode. API is only available when device is in setup mode (no stored credentials or after physical reset).

## Authentication

No authentication required - physical access to device implies authorization. API automatically times out after 10 minutes for security.

## Endpoints

### GET /setup

Returns the WiFi configuration form.

**Request:**
```http
GET /setup HTTP/1.1
Host: 192.168.4.1
Accept: text/html
```

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 3847
Cache-Control: no-cache

<!DOCTYPE html>
<html>
<head>
    <title>Garage Sensor WiFi Setup</title>
    <style>/* Inline CSS */</style>
</head>
<body>
    <form id="wifiForm" action="/configure" method="POST">
        <label for="ssid">WiFi Network Name:</label>
        <input type="text" id="ssid" name="ssid" maxlength="32" required>
        
        <label for="password">WiFi Password:</label>  
        <input type="password" id="password" name="password" maxlength="63" required>
        
        <label for="confirm_password">Confirm Password:</label>
        <input type="password" id="confirm_password" name="confirm_password" maxlength="63" required>
        
        <button type="submit">Configure WiFi</button>
    </form>
    <script>/* Inline JavaScript */</script>
</body>
</html>
```

**Error Cases:**
- 503 Service Unavailable - Setup mode not active
- 408 Request Timeout - Session expired

### POST /configure

Submit WiFi credentials for validation and storage.

**Request:**
```http
POST /configure HTTP/1.1
Host: 192.168.4.1
Content-Type: application/x-www-form-urlencoded
Content-Length: 65

ssid=MyHomeWiFi&password=mypassword123&confirm_password=mypassword123
```

**Success Response:**
```http
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 134

{
  "status": "success",
  "message": "WiFi credentials saved successfully",
  "connection_test": "success",
  "redirect_in_seconds": 5,
  "device_will_restart": true
}
```

**Validation Error Response:**
```http
HTTP/1.1 400 Bad Request
Content-Type: application/json

{
  "status": "error", 
  "message": "WiFi connection failed - please check network name and password",
  "connection_test": "failed",
  "errors": [
    "Unable to connect to network 'MyHomeWiFi'",
    "Please verify the password is correct"
  ]
}
```

**Field Validation Errors:**
```http
HTTP/1.1 422 Unprocessable Entity
Content-Type: application/json

{
  "status": "error",
  "message": "Invalid input data",
  "errors": [
    "SSID is required and must be 1-32 characters",
    "Password is required and must be 1-63 characters", 
    "Passwords do not match"
  ]
}
```

### GET /status

Get current device and setup status.

**Request:**
```http
GET /status HTTP/1.1  
Host: 192.168.4.1
Accept: application/json
```

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "device_id": "GarageSensor-A1B2C3",
  "connection_state": "disconnected",
  "setup_active": true,
  "session_remaining_seconds": 547,
  "portal_version": "1.0",
  "device_uptime_seconds": 127
}
```

**Connection States:**
- `"disconnected"` - Not connected to any WiFi
- `"connecting"` - Attempting to connect  
- `"connected"` - Successfully connected
- `"failed"` - Connection attempt failed

## Error Handling

All errors return appropriate HTTP status codes with JSON error details:

```json
{
  "status": "error",
  "message": "Human readable error description", 
  "code": "ERROR_CODE_CONSTANT",
  "details": "Additional technical details"
}
```

## Rate Limiting

- Maximum 10 requests per minute per IP
- Maximum 3 failed credential attempts before 5-minute lockout
- Setup session automatically expires after 10 minutes

## Security Considerations  

- API only available during setup mode
- No credential information returned in any response
- HTTPS not available (local AP only)
- Session timeout enforces physical access requirement
- Rate limiting prevents brute force attacks