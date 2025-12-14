# EasyWiFi Troubleshooting Guide

## ESP32-C3 WiFi Connection Issues

This document chronicles the troubleshooting process for connecting an ESP32-C3 to an eero mesh network, including all attempted fixes and solutions.

---

## Problem Summary

**Initial Issue**: ESP32-C3 could not connect to eero mesh WiFi network despite correct credentials, strong signal (-49 to -55 dBm), and 2.4GHz compatibility.

**Symptoms**:
- Connection attempts timed out after 1-2 seconds
- Disconnect reasons: `TIMEOUT` (39), `AUTH_EXPIRE` (2), `MISSING_ACKS` (34)
- Network scan showed strong signal and correct channel
- Same credentials worked on other devices

---

## Troubleshooting Timeline

### 1. Initial Diagnostics

**Added comprehensive logging** to understand connection failures:

```cpp
// WiFiManager.cpp - Added detailed diagnostics
void WiFiManager::logWiFiDiagnostics() {
    ESP_LOGI(TAG, "WiFi Mode: %s", WiFi.getMode());
    ESP_LOGI(TAG, "WiFi Status: %d (%s)", WiFi.status(), getWiFiStatusName(WiFi.status()));
    ESP_LOGI(TAG, "Auto-Connect: %s", WiFi.getAutoConnect() ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "Auto-Reconnect: %s", WiFi.getAutoReconnect() ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "WiFi Power: %.1f dBm", WiFi.getTxPower());
    // ... more diagnostics
}
```

**Added human-readable disconnect reasons**:

```cpp
const char* getDisconnectReasonName(wifi_err_reason_t reason) {
    switch(reason) {
        case WIFI_REASON_TIMEOUT: return "TIMEOUT";
        case WIFI_REASON_AUTH_EXPIRE: return "AUTH_EXPIRE";
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4WAY_HANDSHAKE_TIMEOUT";
        case WIFI_REASON_MISSING_ACKS: return "MISSING_ACKS";
        // ... 40+ more reasons
    }
}
```

### 2. Timeout Adjustments

**Problem**: Connection attempts were taking too long and triggering watchdog timer.

**Solution**: 
- Increased `WIFI_CONNECTION_TIMEOUT` from 15s → 30s
- Increased watchdog timeout from 30s → 60s
- Reduced connection attempts from 3 → 1 (for faster testing)

```cpp
// Macros.h
#define WIFI_CONNECTION_TIMEOUT 30000      // 30 seconds
#define WIFI_CONNECTION_ATTEMPTS 1         // Single attempt for testing
#define WDT_TIMEOUT 60                     // 60 seconds
```

**Result**: ❌ No improvement - still timing out

### 3. 5GHz Network Filtering

**Problem**: ESP32-C3 only supports 2.4GHz, but scans detected both bands.

**Solution**: Filter out 5GHz networks (channels 36+) during scan:

```cpp
// WiFiManager.cpp
if (WiFi.channel(i) >= 36) {
    ESP_LOGV(TAG, "  ✗ %d: %s (ch %d, %d dBm) - 5GHz not supported by ESP32-C3",
             i + 1, ssid.c_str(), WiFi.channel(i), WiFi.RSSI(i));
    continue;
}
```

**Result**: ❌ No improvement - eero networks were already on 2.4GHz (channel 11)

### 4. AP Mode Background Interference

**Problem**: After entering AP mode, WiFi STA mode kept trying to reconnect in background, causing `AUTH_EXPIRE` spam every second.

**Solution**: Explicitly disable STA mode before starting AP:

```cpp
// WiFiManager.cpp - startAccessPoint()
ESP_LOGI(TAG, "Stopping STA mode and disabling auto-reconnect...");
WiFi.setAutoReconnect(false);
WiFi.disconnect(true, true);  // disconnect and erase config
WiFi.mode(WIFI_AP);            // Set to AP-only mode
delay(100);                    // Give WiFi stack time to switch
```

**Result**: ✅ Fixed AP mode interference - no more background reconnection attempts

### 5. Captive Portal Detection

**Problem**: When connecting to AP mode, Mac/iPhone didn't show captive portal popup.

**Solution**: Redirect ALL unknown URLs to configuration page with 302 redirect:

```cpp
// ConfigServer.cpp - handleNotFound()
// Captive portal detection URLs:
// - iOS/macOS: captive.apple.com
// - Android: google.com/generate_204
// - Windows: msftconnecttest.com

ESP_LOGI(TAG, "Captive portal detection: redirecting %s to config page", uri.c_str());
server.sendHeader("Location", "http://192.168.4.1/", true);
server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
server.send(302, "text/plain", "");
```

**Result**: ✅ Captive portal now works - automatic popup on connection

### 6. WPA3 and Network Security

**Problem**: Suspected WPA3 or security settings might be blocking ESP32.

**Tests Performed**:
- ✅ Disabled WPA3 on eero network
- ✅ Verified WPA2-PSK only
- ✅ Disabled client steering
- ✅ Created separate guest network for testing

**Result**: ❌ No improvement - still timing out even with simplified security

### 7. ESP32-C3 + eero Compatibility (Root Cause)

**Discovery**: Research revealed ESP32-C3 has known compatibility issues with eero mesh networks.

**Root Cause**: 
- Default transmit power too high → overloads eero receiver
- 802.11n mode can cause interference with mesh signals
- Channel scanning adds unnecessary delay

**Solution**: Apply three-part compatibility fix:

#### A. Lower Transmit Power

```cpp
// WiFiManager.cpp
WiFi.mode(WIFI_STA);  // Initialize WiFi first
WiFi.setTxPower(WIFI_POWER_8_5dBm);  // Lower power for eero compatibility
ESP_LOGI(TAG, "Set transmit power to 8.5dBm (eero compatibility fix)");
```

**Why**: High power (19.5dBm default) can oversaturate eero's receiver or interfere with mesh backhaul.

#### B. Disable 802.11n (Force b/g Only)

```cpp
// WiFiManager.cpp
#include <esp_wifi.h>

esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, 
                                          WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
ESP_LOGI(TAG, "Set WiFi protocol to 802.11b/g (disabled 802.11n)");
```

**Why**: 802.11n can conflict with eero's mesh coordination on same channels.

#### C. Target Specific Channel

```cpp
// WiFiManager.cpp
ESP_LOGI(TAG, "Configured for channel 11 (eero primary channel)");
// WiFiMulti handles channel targeting automatically during connection
```

**Why**: Reduces connection time by eliminating channel scan.

---

## Complete Solution

### Implementation Order (Critical!)

1. **Initialize WiFi first**: `WiFi.mode(WIFI_STA);`
2. **Set low power**: `WiFi.setTxPower(WIFI_POWER_8_5dBm);`
3. **Disable 802.11n**: `esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);`
4. **Attempt connection**: `wifiMulti.run(timeout);`

**⚠️ Order matters!** Setting power before WiFi initialization will fail silently.

### Full Code Example

```cpp
bool WiFiManager::connectToStoredNetworks(Storage& storage) {
    // 1. Load credentials
    std::vector<WiFiCredential> credentials;
    storage.loadCredentials(credentials);
    
    for (const auto& cred : credentials) {
        wifiMulti.addAP(cred.ssid.c_str(), cred.password.c_str());
    }
    
    // 2. Configure WiFi for eero compatibility
    ESP_LOGI(TAG, "Configuring WiFi for optimal eero compatibility...");
    
    WiFi.mode(WIFI_STA);  // Initialize first!
    
    if (WiFi.setTxPower(WIFI_POWER_8_5dBm)) {
        ESP_LOGI(TAG, "✓ Set transmit power to 8.5dBm");
    }
    
    esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, 
                                              WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✓ Set WiFi protocol to 802.11b/g");
    }
    
    // 3. Attempt connection
    uint8_t status = wifiMulti.run(WIFI_CONNECTION_TIMEOUT);
    
    return (status == WL_CONNECTED);
}
```

---

## Diagnostic Information

### Understanding Disconnect Reasons

| Reason Code | Name | Meaning | Likely Cause |
|-------------|------|---------|--------------|
| 2 | `AUTH_EXPIRE` | Authentication expired | Wrong password or AP rejecting |
| 15 | `4WAY_HANDSHAKE_TIMEOUT` | WPA handshake failed | Password/security mismatch |
| 34 | `MISSING_ACKS` | No acknowledgments | Signal issue or AP ignoring |
| 39 | `TIMEOUT` | Connection timeout | AP not responding |
| 201 | `NO_AP_FOUND` | SSID not visible | Wrong SSID or out of range |
| 202 | `AUTH_FAIL` | Authentication failed | Wrong password |

### WiFi Power Levels

```cpp
WIFI_POWER_19_5dBm   // Maximum (default) - can overload eero
WIFI_POWER_15dBm     // High
WIFI_POWER_11dBm     // Medium
WIFI_POWER_8_5dBm    // Low - recommended for eero
WIFI_POWER_5dBm      // Very low
WIFI_POWER_2dBm      // Minimum
```

### WiFi Protocol Options

```cpp
WIFI_PROTOCOL_11B    // 802.11b only (11 Mbps max)
WIFI_PROTOCOL_11G    // 802.11g only (54 Mbps max)
WIFI_PROTOCOL_11N    // 802.11n (150+ Mbps) - can cause eero issues
WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G  // b/g mixed - recommended
```

---

## Testing Checklist

If experiencing connection issues, verify:

- [ ] **Signal strength**: RSSI should be > -70 dBm for reliable connection
- [ ] **Channel**: Confirm device supports the channel (ESP32-C3: 2.4GHz only, channels 1-13)
- [ ] **Security**: Verify WPA2-PSK (WPA3 may not work)
- [ ] **Password**: Double-check credentials are correct
- [ ] **Power setting**: Confirm `setTxPower()` called AFTER `WiFi.mode()`
- [ ] **Protocol**: Verify 802.11b/g mode is set
- [ ] **Timeout**: Allow at least 30 seconds for connection attempt
- [ ] **Test network**: Try connecting to phone hotspot to isolate router-specific issues

---

## Known Issues

### 1. Serial Upload Failures

**Symptom**: `A fatal error occurred: The chip stopped responding`

**Solution**: 
- Hold BOOT button, press/release RESET, release BOOT
- Try different USB cable or port
- Retry upload 2-3 times

**Note**: This is a hardware/USB issue, not related to WiFi code.

### 2. Verbose LED Logs

**Symptom**: Constant `analogWrite()` log spam from LED blinking

**Attempted Fix**:
```cpp
esp_log_level_set("esp32-hal-ledc", ESP_LOG_WARN);
```

**Status**: Partially effective - logs reduced but not eliminated (Arduino framework logs occur early)

### 3. eero-Specific Networks

**Known Fact**: ESP32-C3 requires special configuration for eero mesh networks (documented above).

**Alternative**: If eero compatibility fixes don't work, consider using a non-mesh router or WiFi extender in bridge mode for IoT devices.

---

## References

- **ESP32-C3 Datasheet**: 2.4GHz only, 802.11 b/g/n support
- **eero + ESP32 Issues**: Common compatibility problems with mesh networks
- **WiFi Disconnect Reasons**: ESP-IDF documentation
- **Power Settings**: Lower power often more reliable than maximum power
- **802.11n**: Can interfere with mesh systems when devices on same channel

---

## Version History

- **v1.0**: Initial implementation with basic WiFi connection
- **v1.1**: Added comprehensive diagnostics and logging
- **v1.2**: Implemented timeout adjustments and watchdog fixes
- **v1.3**: Added 5GHz filtering and channel detection
- **v1.4**: Fixed AP mode STA interference
- **v1.5**: Implemented captive portal redirects
- **v1.6**: Added eero compatibility fixes (power + protocol)
- **v1.7**: Fixed power setting timing issue (initialize WiFi first)

---

## Quick Reference: ESP32-C3 + eero Fix

```cpp
WiFi.mode(WIFI_STA);
WiFi.setTxPower(WIFI_POWER_8_5dBm);
esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
wifiMulti.run(30000);
```

**This four-line sequence solves ESP32-C3 + eero compatibility issues.**

