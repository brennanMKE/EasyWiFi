# ⚠️ CRITICAL: DO NOT USE WiFiMulti

## Rule

**NEVER use `WiFiMulti` in this codebase.**

## Why

`WiFiMulti` internally overrides or interferes with the ESP32-C3 + eero compatibility settings required for reliable connections:

- Low transmit power (8.5dBm)
- 802.11b/g only (no 802.11n)

When these settings are changed or overridden, the ESP32-C3 **cannot** connect to eero mesh networks. Symptoms include:
- `MISSING_ACKS` errors
- `TIMEOUT` errors  
- `AUTH_EXPIRE` errors
- Connection failures despite strong signal

## What to Use Instead

### ✅ Correct: Direct WiFi.begin()

```cpp
// 1. Set compatibility settings ONCE
WiFi.mode(WIFI_STA);
WiFi.setTxPower(WIFI_POWER_8_5dBm);
esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);

// 2. Connect using direct WiFi.begin()
WiFi.begin(ssid, password);

// 3. Poll for connection
unsigned long start = millis();
while (millis() - start < 30000) {
    if (WiFi.status() == WL_CONNECTED) {
        break;
    }
    yield();
}
```

### ❌ Wrong: WiFiMulti

```cpp
// DO NOT DO THIS!
WiFiMulti wifiMulti;
wifiMulti.addAP(ssid1, password1);
wifiMulti.addAP(ssid2, password2);
wifiMulti.run();  // This will break eero compatibility!
```

## Multiple Networks

To support multiple networks, use a **sequential loop** with direct `WiFi.begin()`:

```cpp
for (size_t i = 0; i < credentials.size(); i++) {
    WiFi.begin(credentials[i].ssid.c_str(), credentials[i].password.c_str());
    
    unsigned long start = millis();
    while (millis() - start < 30000) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;  // Success!
        }
        yield();
    }
    
    WiFi.disconnect();  // Try next network
}
```

## Prevention Measures

1. **Removed WiFiMulti from headers** - `WiFiManager.h` no longer includes `<WiFiMulti.h>`
2. **Removed WiFiMulti member variable** - No `wifiMulti` instance in `WiFiManager`
3. **Removed addCredentials() method** - This method used `wifiMulti.addAP()`
4. **Added warning comments** - Multiple comments explain why WiFiMulti is forbidden
5. **This document** - Explains the rule and consequences

## Code Review Checklist

Before committing or merging code that touches WiFi functionality:

- [ ] Does the code use `WiFiMulti` anywhere? **If yes, reject!**
- [ ] Does the code call `WiFi.setTxPower()` with anything other than `WIFI_POWER_8_5dBm`? **If yes, review carefully!**
- [ ] Does the code call `esp_wifi_set_protocol()` with `WIFI_PROTOCOL_11N`? **If yes, reject!**
- [ ] Does the code use direct `WiFi.begin()` for connections? **Required!**
- [ ] Are the eero compatibility settings applied ONCE before ANY connections? **Required!**
- [ ] Are the settings preserved across ALL network attempts? **Required!**

## References

- `Fix.md` - Original fix documentation
- `lib/EasyWiFi/WiFiManager.cpp` - Implementation with warning comments
- `lib/EasyWiFi/WiFiManager.h` - Header with warning comments

## Emergency Recovery

If someone accidentally re-introduces `WiFiMulti`:

1. **Stop immediately** - Do not merge or commit
2. **Revert the changes** - Remove all `WiFiMulti` code
3. **Re-read this document and Fix.md**
4. **Re-implement using direct `WiFi.begin()`**
5. **Test on eero network** - Connection must succeed in <5 seconds

## Test Coverage

To verify the fix is working:

1. Device should connect to eero network in **1-5 seconds** (not 30+ seconds)
2. No `MISSING_ACKS`, `TIMEOUT`, or `AUTH_EXPIRE` errors in logs
3. Signal strength should be -40 to -60 dBm (Excellent/Good)
4. IP address assigned and stable
5. mDNS and web server accessible

## Last Words

**If you're tempted to use `WiFiMulti` because it seems simpler:**

- It's not simpler if it doesn't work
- The sequential loop with `WiFi.begin()` is equally simple
- The sequential approach is more reliable for ESP32-C3 + eero
- We spent significant time debugging the `WiFiMulti` issue
- **Don't repeat the mistake**

See `Fix.md` for the complete story of how we discovered this issue and what the correct solution is.

---

**Created:** 2024-12-14  
**Purpose:** Prevent regression of ESP32-C3 + eero compatibility fix

