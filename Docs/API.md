# EasyWiFi REST API Documentation

**Version:** 1.0  
**Base URL:** `http://[device-ip]/api/` or `http://[mdns-hostname].local/api/`

## Overview

The EasyWiFi REST API provides programmatic access to configure and monitor WiFi connectivity on ESP32-C3 devices. All endpoints return JSON responses and support CORS for cross-origin requests.

## Authentication

Currently, no authentication is required. Future versions may add API key authentication.

## Response Format

All API responses follow this format:

### Success Response
```json
{
  "success": true,
  "data": { ... },
  "message": "Optional success message"
}
```

### Error Response
```json
{
  "success": false,
  "error": "error_code",
  "message": "Human-readable error message"
}
```

## HTTP Status Codes

| Code | Meaning |
|------|---------|
| 200 | OK - Request successful |
| 201 | Created - Resource created |
| 204 | No Content - Success with no response body |
| 400 | Bad Request - Invalid input |
| 404 | Not Found - Endpoint doesn't exist |
| 500 | Internal Server Error - Server-side error |

---

## Endpoints

### 1. Get Device Status

Get comprehensive device and WiFi status information.

**Endpoint:** `GET /api/status`

**Response:**
```json
{
  "api_version": "1.0",
  "device": {
    "type": "ESP32-C3",
    "firmware": "EasyWiFi v0.1.0",
    "free_heap": 285432,
    "uptime": 3600
  },
  "wifi": {
    "status": "Connected",
    "connected": true,
    "ap_mode": false,
    "configured": true,
    "ip": "192.168.1.100",
    "ssid": "MyNetwork",
    "rssi": -45,
    "signal_strength": "Excellent",
    "mac": "AA:BB:CC:DD:EE:FF",
    "mdns": "easywifi-aabbcc.local"
  },
  "storage": {
    "configured": true,
    "credential_count": 2
  }
}
```

**curl Example:**
```bash
curl http://192.168.4.1/api/status
```

**Response Fields:**
- `device.free_heap` - Available RAM in bytes
- `device.uptime` - Seconds since boot
- `wifi.rssi` - Signal strength in dBm (-100 to 0)
- `wifi.signal_strength` - Human-readable signal quality
- `storage.credential_count` - Number of stored WiFi credentials

---

### 2. Scan WiFi Networks

Scan for available WiFi networks and return results sorted by signal strength.

**Endpoint:** `GET /api/scan`

**Response:**
```json
{
  "success": true,
  "count": 3,
  "networks": [
    {
      "ssid": "HomeNetwork",
      "rssi": -45,
      "encryption": "WPA2-PSK",
      "open": false,
      "signal_strength": "Excellent",
      "signal_bars": 4
    },
    {
      "ssid": "GuestNetwork",
      "rssi": -67,
      "encryption": "Open",
      "open": true,
      "signal_strength": "Fair",
      "signal_bars": 2
    }
  ]
}
```

**curl Example:**
```bash
curl http://192.168.4.1/api/scan
```

**Notes:**
- Scan takes 2-5 seconds to complete
- Results are sorted by RSSI (strongest first)
- `signal_bars` range: 0-4 (for UI display)

---

### 3. Configure WiFi Credentials

Save WiFi SSID and password to device storage.

**Endpoint:** `POST /api/configure`

**Request Body (form-urlencoded):**
```
ssid=MyNetwork&password=MyPassword123
```

**Or JSON:**
```json
{
  "ssid": "MyNetwork",
  "password": "MyPassword123"
}
```

**Success Response:**
```json
{
  "success": true,
  "message": "WiFi credentials saved successfully",
  "ssid": "MyNetwork",
  "next_step": "Device will attempt to connect"
}
```

**Error Response (validation):**
```json
{
  "success": false,
  "error": "validation_error",
  "message": "Password must be at least 8 characters for WPA/WPA2"
}
```

**curl Example:**
```bash
curl -X POST http://192.168.4.1/api/configure \
  -d "ssid=MyNetwork" \
  -d "password=MyPassword123"
```

**Validation Rules:**
- SSID: Required, max 32 characters
- Password: Optional (for open networks), 8-63 characters for secured networks

---

### 4. Initiate Connection

Trigger connection attempt to configured network.

**Endpoint:** `POST /api/connect`

**Response:**
```json
{
  "success": true,
  "message": "Connection attempt will be initiated",
  "note": "Device will attempt to connect to configured network"
}
```

**curl Example:**
```bash
curl -X POST http://192.168.4.1/api/connect
```

**Notes:**
- Requires credentials to be configured first
- Connection happens asynchronously
- Poll `/api/status` to check connection result

---

### 5. Clear WiFi Credentials

Delete all stored WiFi credentials from device.

**Endpoint:** `DELETE /api/credentials`

**Response:**
```json
{
  "success": true,
  "message": "All WiFi credentials cleared",
  "cleared_count": 2
}
```

**curl Example:**
```bash
curl -X DELETE http://192.168.4.1/api/credentials
```

**Notes:**
- Removes all stored networks
- Device will enter AP mode after clearing
- Non-destructive - can reconfigure immediately

---

### 6. Factory Reset

Clear all credentials and reboot device.

**Endpoint:** `GET /api/reset`

**Response:**
```json
{
  "success": true,
  "message": "Factory reset initiated",
  "cleared_count": 2,
  "action": "Device will reboot in 2 seconds"
}
```

**curl Example:**
```bash
curl http://192.168.4.1/api/reset
```

**Notes:**
- Clears all WiFi credentials
- Device reboots automatically after 2 seconds
- Starts in AP mode after reboot
- **Use with caution** - requires physical access to reconfigure

---

## CORS Support

All API endpoints support Cross-Origin Resource Sharing (CORS). This allows web applications hosted on different domains to access the API.

**Preflight Request:**
```bash
curl -X OPTIONS http://192.168.4.1/api/status \
  -H "Origin: http://example.com" \
  -H "Access-Control-Request-Method: GET"
```

**CORS Headers:**
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type`

---

## Usage Examples

### Complete Configuration Workflow

```bash
#!/bin/bash

# 1. Check device status
echo "Checking device status..."
curl http://192.168.4.1/api/status

# 2. Scan for networks
echo -e "\nScanning for networks..."
curl http://192.168.4.1/api/scan

# 3. Configure WiFi
echo -e "\nConfiguring WiFi..."
curl -X POST http://192.168.4.1/api/configure \
  -d "ssid=MyNetwork" \
  -d "password=MyPassword123"

# 4. Initiate connection
echo -e "\nInitiating connection..."
curl -X POST http://192.168.4.1/api/connect

# 5. Wait and check status
echo -e "\nWaiting for connection..."
sleep 5
curl http://192.168.4.1/api/status
```

### JavaScript/Fetch Example

```javascript
// Scan for networks
async function scanNetworks() {
  const response = await fetch('http://192.168.4.1/api/scan');
  const data = await response.json();
  
  if (data.success) {
    console.log(`Found ${data.count} networks:`);
    data.networks.forEach(network => {
      console.log(`  - ${network.ssid} (${network.signal_strength})`);
    });
  }
}

// Configure WiFi
async function configureWiFi(ssid, password) {
  const response = await fetch('http://192.168.4.1/api/configure', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
  });
  
  const data = await response.json();
  
  if (data.success) {
    console.log('WiFi configured successfully!');
    return true;
  } else {
    console.error('Configuration failed:', data.message);
    return false;
  }
}

// Monitor connection status
async function monitorConnection() {
  for (let i = 0; i < 10; i++) {
    const response = await fetch('http://192.168.4.1/api/status');
    const data = await response.json();
    
    if (data.wifi.connected) {
      console.log('Connected to:', data.wifi.ssid);
      console.log('IP Address:', data.wifi.ip);
      return true;
    }
    
    console.log('Connecting... attempt', i + 1);
    await new Promise(resolve => setTimeout(resolve, 2000));
  }
  
  console.error('Connection timeout');
  return false;
}
```

### Python Example

```python
import requests
import time

BASE_URL = "http://192.168.4.1/api"

def scan_networks():
    """Scan for available WiFi networks."""
    response = requests.get(f"{BASE_URL}/scan")
    data = response.json()
    
    if data['success']:
        print(f"Found {data['count']} networks:")
        for network in data['networks']:
            print(f"  - {network['ssid']}: {network['signal_strength']} ({network['rssi']} dBm)")
        return data['networks']
    return []

def configure_wifi(ssid, password):
    """Configure WiFi credentials."""
    response = requests.post(
        f"{BASE_URL}/configure",
        data={'ssid': ssid, 'password': password}
    )
    data = response.json()
    
    if data['success']:
        print(f"WiFi configured: {ssid}")
        return True
    else:
        print(f"Error: {data['message']}")
        return False

def get_status():
    """Get device status."""
    response = requests.get(f"{BASE_URL}/status")
    return response.json()

def wait_for_connection(timeout=20):
    """Wait for WiFi connection."""
    start = time.time()
    
    while time.time() - start < timeout:
        status = get_status()
        
        if status['wifi']['connected']:
            print(f"Connected to {status['wifi']['ssid']}")
            print(f"IP: {status['wifi']['ip']}")
            return True
        
        print("Connecting...")
        time.sleep(2)
    
    print("Connection timeout")
    return False

# Usage
if __name__ == "__main__":
    # Scan networks
    networks = scan_networks()
    
    # Configure WiFi
    if configure_wifi("MyNetwork", "MyPassword123"):
        # Wait for connection
        wait_for_connection()
```

---

## Error Codes

| Error Code | Description | HTTP Status |
|------------|-------------|-------------|
| `validation_error` | Invalid input parameters | 400 |
| `not_found` | Endpoint or resource not found | 404 |
| `storage_error` | Failed to read/write storage | 500 |
| `no_credentials` | No WiFi credentials configured | 400 |

---

## Rate Limiting

Currently no rate limiting is implemented. Future versions may add:
- Max 10 requests per second per IP
- Max 100 requests per minute per IP

---

## Best Practices

1. **Poll with Delays**: When checking connection status, poll every 2-3 seconds
2. **Timeout Handling**: Set reasonable timeouts (10-20 seconds for connections)
3. **Error Handling**: Always check `success` field in responses
4. **Validation**: Validate input client-side before sending to API
5. **HTTPS**: For production, consider adding TLS/HTTPS support

---

## Troubleshooting

### API Not Responding
- Check device is powered on
- Verify you're connected to device's AP (EasyWiFi-Setup-XXXXXX)
- Try default IP: `192.168.4.1`

### CORS Errors in Browser
- CORS is enabled by default
- Check browser console for specific errors
- Verify request origin is allowed

### Connection Failures
- Verify SSID and password are correct
- Check network uses 2.4GHz (ESP32-C3 doesn't support 5GHz)
- Ensure network uses supported encryption (WPA2/WPA3)

---

## Changelog

### Version 1.0 (2025-12-13)
- Initial API release
- All core endpoints implemented
- CORS support added
- Comprehensive error handling

---

## Support

For issues or questions:
- Check the main README.md for setup instructions
- Review PRD.md for architectural details
- Examine source code in `lib/EasyWiFi/ConfigServer.cpp`

---

**License:** MIT  
**Project:** EasyWiFi ESP32-C3 WiFi Configuration System

