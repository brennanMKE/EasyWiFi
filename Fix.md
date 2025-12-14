# ESP32-C3 + eero WiFi Connection Fix

## Problem

ESP32-C3 could not connect to eero mesh WiFi networks. Connection attempts resulted in:
- Disconnect reason: `TIMEOUT` (39)
- Disconnect reason: `AUTH_EXPIRE` (2)  
- Disconnect reason: `MISSING_ACKS` (34)
- Connection failed after 1-2 seconds despite strong signal (-41 to -55 dBm)

## Root Cause

**Using `WiFiMulti.run()` interferes with ESP32-C3 + eero compatibility settings.**

The `WiFiMulti` library's internal connection logic overrides or conflicts with the low-power and protocol settings required for eero mesh compatibility.

## Solution

### Use Direct `WiFi.begin()` Instead of `WiFiMulti`

Replace `WiFiMulti.run()` with direct `WiFi.begin()` and apply three eero compatibility settings:

```cpp
#include <WiFi.h>
#include <esp_wifi.h>

bool connectToNetwork(const char* ssid, const char* password) {
    // 1. Initialize WiFi in STA mode
    WiFi.mode(WIFI_STA);
    
    // 2. Lower transmit power (ESP32-C3 + eero compatibility)
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    
    // 3. Disable 802.11n (use only 802.11b/g)
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
    
    // 4. Connect using direct WiFi.begin() - NOT WiFiMulti!
    WiFi.begin(ssid, password);
    
    // 5. Poll for connection (non-blocking)
    unsigned long start = millis();
    while (millis() - start < 30000) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        yield();
    }
    
    return false;
}
```

## Why This Works

### 1. Lower Transmit Power (8.5dBm)

**Default**: 19.5dBm (very high power)  
**Problem**: High power oversaturates eero's receiver and interferes with mesh backhaul  
**Solution**: 8.5dBm provides sufficient signal without overload

### 2. Disable 802.11n

**Default**: 802.11b/g/n enabled  
**Problem**: 802.11n on same channel can interfere with eero's mesh coordination  
**Solution**: Force 802.11b/g only for cleaner, more compatible connection

### 3. Direct WiFi.begin()

**Alternative**: `WiFiMulti.run()`  
**Problem**: WiFiMulti may reset or override power/protocol settings internally  
**Solution**: Direct `WiFi.begin()` preserves all configured settings

## Results

**Before Fix:**
- Connection failed: `TIMEOUT` after 30+ seconds
- Multiple disconnect/reconnect attempts
- Never obtained IP address

**After Fix:**
- ✅ Connected in **1.3 seconds**
- ✅ Obtained IP: 192.168.4.199
- ✅ Signal: -41 dBm (Excellent)
- ✅ Stable connection
- ✅ mDNS and web server working

## Order of Operations (Critical!)

The sequence matters:

```cpp
// 1. FIRST: Initialize WiFi
WiFi.mode(WIFI_STA);

// 2. THEN: Configure settings
WiFi.setTxPower(WIFI_POWER_8_5dBm);
esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);

// 3. FINALLY: Connect
WiFi.begin(ssid, password);
```

**⚠️ Setting power/protocol BEFORE `WiFi.mode()` will fail silently!**

## Additional Best Practices

### Remove Blocking Delays

Replace all `delay()` calls with non-blocking alternatives:

```cpp
// Bad:
delay(1000);

// Good:
unsigned long start = millis();
while (millis() - start < 1000) {
    esp_task_wdt_reset();  // Keep watchdog happy
    yield();               // Allow other tasks
}
```

### Feed Watchdog During Long Operations

```cpp
while (waitingForSomething) {
    esp_task_wdt_reset();  // Prevent watchdog timeout
    yield();
}
```

## Hardware Note

**This fix is specific to ESP32-C3 + eero mesh networks.**

Other ESP32 variants (ESP32, ESP32-S2, ESP32-S3) or other routers may not need these settings. Always test with different power levels and protocols if experiencing connection issues.

## Testing on Other Networks

To verify the ESP32 hardware is working, test on:
- Phone WiFi hotspot (2.4GHz)
- Non-mesh router
- Guest network with simpler security

If it connects elsewhere but not to eero, the issue is eero-specific compatibility.

## Quick Reference

**ESP32-C3 + eero**: Use this exact sequence:

```cpp
WiFi.mode(WIFI_STA);
WiFi.setTxPower(WIFI_POWER_8_5dBm);
esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
WiFi.begin(ssid, password);
```

**Connection time**: ~1-2 seconds (vs 30+ second timeouts with WiFiMulti)

---

## Files Modified

- `lib/EasyWiFi/WiFiManager.cpp`: Changed from WiFiMulti to WiFi.begin()
- `lib/EasyWiFi/WiFiManager.h`: Added `#include <esp_wifi.h>`
- `lib/EasyWiFi/RunLoop.cpp`: Removed blocking delays
- `lib/EasyWiFi/ConfigServer.cpp`: Removed blocking delays
- `lib/EasyWiFi/Macros.h`: Reduced connection attempts to 1

## Credits

Solution derived from working `ConnectWiFi` test code and ESP32-C3 + eero compatibility research.

**Key insight**: Always use the simplest WiFi connection method (`WiFi.begin()`) before trying more complex wrappers like `WiFiMulti`.

