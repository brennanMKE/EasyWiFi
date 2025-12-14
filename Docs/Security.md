# Security Recommendations for EasyWiFi

**Project Type**: Hobby/Personal Use  
**Threat Model**: Home network, trusted physical environment  
**Updated**: December 14, 2024

---

## Overview

This document provides **practical security recommendations** tailored for hobby projects. Unlike the comprehensive `CodeReview.md` which assumes production/commercial use, these recommendations focus on **realistic threats** for a personal ESP32 device on a home network.

---

## Priority Levels

- 🔴 **Critical**: Do these (30 minutes)
- 🟡 **Recommended**: Good practice, easy to implement (1 hour)
- ⚪ **Optional**: Nice to have but not essential
- ⚫ **Skip**: Overkill for hobby projects

---

## 🔴 Critical (Implement These)

### 1. Add Access Point Password

**Current Risk:** Anyone within WiFi range can connect to `EasyWiFi-Setup-XXXXXX` and reconfigure your device.

**Fix:**
```cpp
// In lib/EasyWiFi/Macros.h, change:
#define AP_PASSWORD ""

// To:
#define AP_PASSWORD "setup123"
// Or something more secure: "MyESP32-2024"
```

**Impact:**
- Prevents neighbors from accidentally or maliciously accessing config mode
- Prevents guests on your network from finding the device
- Takes 30 seconds to implement

**Testing:**
1. Build and upload firmware
2. Force device into AP mode (factory reset or no credentials)
3. Try to connect to `EasyWiFi-Setup-XXXXXX`
4. Should prompt for password

---

### 2. Input Validation

**Current Risk:** Entering too-long SSID/password causes undefined behavior or crashes.

**Fix:**
```cpp
// In lib/EasyWiFi/ConfigServer.cpp, add to handleSave() after line ~130:

// Validate SSID
if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID cannot be empty");
    ESP_LOGW(TAG, "Rejected empty SSID");
    return;
}
if (ssid.length() > 32) {
    server.send(400, "text/plain", "SSID too long (max 32 characters)");
    ESP_LOGW(TAG, "Rejected SSID length: %d", ssid.length());
    return;
}

// Validate password
if (password.length() > 0 && password.length() < 8) {
    server.send(400, "text/plain", "Password too short (min 8 characters for WPA2)");
    return;
}
if (password.length() > 63) {
    server.send(400, "text/plain", "Password too long (max 63 characters)");
    ESP_LOGW(TAG, "Rejected password length: %d", password.length());
    return;
}

// Continue with existing save logic...
```

**Impact:**
- Prevents crashes from invalid input
- Provides clear error messages
- Catches your own typos

---

## 🟡 Recommended (Good Practice)

### 3. Optional Web UI Authentication

**Use Case:** If the device will be permanently on your network and you want to prevent accidental reconfiguration.

**Trade-off:** You'll need to remember another password. For devices you only configure once, this might be more annoying than useful.

**Implementation Option A: Basic HTTP Auth (Simple)**
```cpp
// In lib/EasyWiFi/ConfigServer.cpp, add to beginning of each handler:

void ConfigServer::handleRoot() {
    // Add authentication check
    if (!server.authenticate("admin", "your-password-here")) {
        return server.requestAuthentication();
    }
    
    // Existing handler code...
}

// Repeat for: handleScan(), handleSave(), handleReset(), etc.
```

**Implementation Option B: Custom Login Page (Better UX)**
```cpp
// Add session token stored in NVS
// Requires more code but provides better user experience
// See "HTTPS Implementation" section for full example
```

**Recommendation:** 
- **Skip if:** You only configure the device once and it's in a locked location
- **Implement if:** Device is permanently accessible on your network
- **Use Option A** (basic auth) - simpler and sufficient for hobby use

---

### 4. Secure Storage of WiFi Credentials

**Current State:** Passwords stored in plaintext in NVS (Non-Volatile Storage).

**Risk Level for Hobby Use:** **Low**
- Requires physical access to the device
- Requires USB connection and technical knowledge to extract
- If someone has physical access to your home to steal the ESP32, you have bigger problems

**If You Want to Protect Anyway:**

**Option A: NVS Encryption (Recommended)**
```ini
# In platformio.ini, add to build_flags:
build_flags = 
    -DCONFIG_NVS_ENCRYPTION=1
    -DCONFIG_NVS_SEC_KEY="your-32-byte-encryption-key-here"
```

**Option B: Simple Obfuscation (Quick)**
```cpp
// Add to lib/EasyWiFi/Storage.cpp:

String obfuscate(const String& data) {
    String result = data;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] ^= 0xA5;  // XOR with constant
    }
    return result;
}

// When saving:
String obfuscated = obfuscate(password);
preferences.putString(passKey.c_str(), obfuscated);

// When loading:
String obfuscated = preferences.getString(passKey.c_str(), "");
String password = obfuscate(obfuscated);  // XOR again to decode
```

**Note:** Obfuscation is **not encryption** - it just makes casual extraction harder.

**My Recommendation:** **Skip this** unless you're particularly paranoid. For a home hobby project, plaintext is fine.

---

## ⚪ Optional (Nice to Have)

### 5. Rate Limiting on Configuration Endpoints

**Purpose:** Prevent rapid repeated configuration attempts.

**For Hobby Use:** Probably unnecessary - you're not going to DoS yourself.

**If You Want It Anyway:**
```cpp
// Add to ConfigServer.h:
private:
    unsigned long lastSaveAttempt = 0;
    const unsigned long SAVE_RATE_LIMIT_MS = 5000;  // 5 seconds

// In ConfigServer.cpp handleSave():
if (millis() - lastSaveAttempt < SAVE_RATE_LIMIT_MS) {
    server.send(429, "text/plain", "Too many requests, please wait");
    return;
}
lastSaveAttempt = millis();
```

---

### 6. Audit Logging

**Purpose:** Track when configuration changes are made.

**For Hobby Use:** You know when you changed it - it was you!

**Skip unless:** You're building something semi-production or want to learn logging.

---

## ⚫ Skip These (Overkill for Hobby)

### Not Recommended for Hobby Projects:
- ❌ **CSRF Protection** - Unnecessary for single-user devices
- ❌ **Session Management** - Adds complexity without real benefit
- ❌ **HTML Escaping for XSS** - You're not entering malicious data into your own device
- ❌ **SQL Injection Prevention** - No database, no SQL
- ❌ **Penetration Testing** - Save this for your day job
- ❌ **Security Audits** - It's a hobby project!

---

## HTTPS/SSL/TLS for Local mDNS Hostname

### Overview

You asked about supporting `https://easywifi-8774bc.local` instead of `http://easywifi-8774bc.local`.

**Short Answer:** Yes, it's possible but **complex and has limited benefit** for local-only access.

### Why HTTPS is Challenging on ESP32

1. **Certificate Issues**
   - Browsers don't trust self-signed certificates by default
   - Let's Encrypt (free certificates) requires public domain, not `.local`
   - `.local` domains (mDNS) aren't routable on the internet

2. **Memory Constraints**
   - SSL/TLS requires ~40-80KB extra RAM for buffers
   - ESP32-C3 has only 320KB RAM total
   - Our current usage: ~42KB, free: ~220KB
   - HTTPS would consume significant headroom

3. **CPU Overhead**
   - SSL handshake is computationally expensive
   - Slower page loads on ESP32-C3's 160MHz CPU

4. **Library Limitations**
   - Arduino `WebServer` doesn't support HTTPS
   - Need to switch to `ESPAsyncWebServer` or `esp_https_server`

### When HTTPS Makes Sense

✅ **Use HTTPS if:**
- Device is exposed to the internet (port forwarding)
- You're sending credentials over public WiFi (coffee shop hotspot mode)
- You want to learn about ESP32 SSL/TLS implementation

❌ **Skip HTTPS if:**
- Device is only on local network (your use case)
- You access via direct IP or `.local` hostname
- The added complexity isn't worth it

**For your hobby project on home network:** HTTP is **perfectly fine**.

---

## How to Implement HTTPS (If You Really Want To)

### Architecture Options

#### Option 1: Self-Signed Certificate (Simple but Browser Warnings)

**Pros:**
- Works offline
- Free
- Full control

**Cons:**
- Browser shows scary warning every time
- Must manually trust certificate on each device
- Certificate pinning required for security

#### Option 2: Local Certificate Authority (Better UX)

**Pros:**
- No browser warnings after initial setup
- Works for all devices once CA is trusted
- Professional approach

**Cons:**
- Must install CA certificate on all devices (phone, laptop, etc.)
- More complex setup
- Still self-signed, just with extra steps

#### Option 3: Reverse Proxy (Best but Complex)

**Pros:**
- Let another device handle SSL (Raspberry Pi, NAS, router)
- ESP32 runs plain HTTP
- Can use real certificate via Cloudflare Tunnel

**Cons:**
- Requires additional hardware/software
- More moving parts to break
- Overkill for hobby project

---

## Implementation Guide: Self-Signed Certificate

### Step 1: Generate Certificate and Key

Use OpenSSL on your computer:

```bash
# Generate private key
openssl genrsa -out server.key 2048

# Generate certificate (valid for 365 days)
openssl req -new -x509 -key server.key -out server.crt -days 365 \
  -subj "/CN=easywifi-8774bc.local/O=EasyWiFi/C=US"

# Convert to C arrays for embedding in ESP32
xxd -i server.crt > server_crt.h
xxd -i server.key > server_key.h
```

### Step 2: Add ESP HTTPS Server Library

```ini
# In platformio.ini, add dependency:
lib_deps = 
    bblanchon/ArduinoJson@^7.4.2
    esp32_https_server  # Add this
```

### Step 3: Embed Certificates in Code

```cpp
// Create new file: lib/EasyWiFi/certificates.h
#ifndef CERTIFICATES_H
#define CERTIFICATES_H

// Paste contents of server_crt.h here:
const unsigned char server_crt[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, ...
};
const unsigned int server_crt_len = 1234;

// Paste contents of server_key.h here:
const unsigned char server_key[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, ...
};
const unsigned int server_key_len = 1678;

#endif
```

### Step 4: Modify ConfigServer to Use HTTPS

```cpp
// In ConfigServer.h:
#include <HTTPSServer.hpp>  // Instead of <WebServer.h>
#include <SSLCert.hpp>
#include "certificates.h"

class ConfigServer {
private:
    // Change from:
    // WebServer server;
    
    // To:
    httpsserver::HTTPSServer* server;
    httpsserver::SSLCert* cert;
    
public:
    void setup() {
        // Create certificate from embedded data
        cert = new httpsserver::SSLCert(
            server_crt, server_crt_len,
            server_key, server_key_len
        );
        
        // Create HTTPS server on port 443
        server = new httpsserver::HTTPSServer(cert, 443);
        
        // Register handlers (syntax slightly different)
        httpsserver::ResourceNode* nodeRoot = new httpsserver::ResourceNode(
            "/", "GET", 
            [this](httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res) {
                this->handleRoot(req, res);
            }
        );
        server->registerNode(nodeRoot);
        
        // ... register other routes ...
        
        server->start();
    }
};
```

### Step 5: Update Handler Signatures

HTTPS server has different API than WebServer:

```cpp
// Old WebServer style:
void ConfigServer::handleRoot() {
    String html = webPages.generateHomePage();
    server.send(200, "text/html", html);
}

// New HTTPS server style:
void ConfigServer::handleRoot(HTTPRequest* req, HTTPResponse* res) {
    String html = webPages.generateHomePage();
    res->setHeader("Content-Type", "text/html");
    res->print(html.c_str());
}
```

### Step 6: Update mDNS to Advertise HTTPS

```cpp
// In WiFiManager.cpp startMDNS():
MDNS.begin(hostname.c_str());
MDNS.addService("http", "tcp", 80);   // HTTP on port 80
MDNS.addService("https", "tcp", 443); // HTTPS on port 443
```

### Step 7: Trust the Certificate

**On Desktop (Chrome/Edge):**
1. Access `https://easywifi-8774bc.local`
2. Click "Advanced" on security warning
3. Click "Proceed to easywifi-8774bc.local (unsafe)"
4. Or: Import `server.crt` into OS certificate store

**On Mobile:**
1. Transfer `server.crt` to phone
2. Install as certificate authority
3. May require device PIN/password

**On Each Device:**
- You must manually trust the certificate
- Different process for Windows/Mac/Linux/iOS/Android
- Must repeat if you regenerate certificate

---

## Memory Impact of HTTPS

### Before HTTPS (Current):
```
RAM:   12.7% (used 41,772 bytes from 327,680 bytes)
Flash: 73.8% (used 967,166 bytes from 1,310,720 bytes)
Free heap: ~220KB
```

### After HTTPS (Estimated):
```
RAM:   ~25-30% (used ~82KB-98KB from 327,680 bytes)
Flash: ~78-82% (used ~1,020KB-1,075KB from 1,310,720 bytes)
Free heap: ~150-180KB (reduction of ~40-70KB)
```

**Conclusion:** Still fits, but significantly less headroom.

---

## Alternative: Use HTTP with Other Security Measures

Instead of HTTPS, consider these simpler approaches:

### 1. MAC Address Filtering
```cpp
// Only allow your phone/laptop MAC addresses
bool isAllowedClient(IPAddress clientIP) {
    // Get client MAC from ARP table
    // Compare to whitelist
    // Reject if not allowed
}
```

### 2. One-Time Setup Token
```cpp
// Display random PIN on serial console at boot
// Require PIN entry on first web access
// Store "configured" flag in NVS
```

### 3. Physical Button for Config Mode
```cpp
// Only enable web UI when button is held
// Normal operation: web server disabled
// Config mode: hold button to enable web UI
```

### 4. Time-Limited Access
```cpp
// Web UI only available for 5 minutes after boot
// After timeout, requires reboot to access again
```

---

## Recommended Security Posture for Hobby Project

### Minimum (30 minutes):
```cpp
✅ AP password: "setup123"
✅ Input validation for SSID/password length
```

### Recommended (1 hour):
```cpp
✅ AP password (above)
✅ Input validation (above)
✅ Optional: Basic HTTP auth on web UI
```

### If You Want to Learn (4-8 hours):
```cpp
✅ All of the above
✅ Implement HTTPS with self-signed certificate
✅ Create local CA and trust it on your devices
```

### My Actual Recommendation:
**Just add the AP password and input validation.** 

You're spending time on WiFi management, not security research. Save HTTPS for a future project where you specifically want to learn about ESP32 SSL/TLS implementation.

---

## Quick Security Checklist

Before considering your device "secure enough" for hobby use:

- [ ] AP has password (not open)
- [ ] Input validation prevents crashes
- [ ] Web UI has basic auth (optional but recommended)
- [ ] Device is on trusted network only (not exposed to internet)
- [ ] Physical access is controlled (it's in your home)
- [ ] You have a backup of important credentials

If all checked: **You're good for hobby use!** 🎉

---

## When to Revisit Security

Consider upgrading security if:

1. **You expose the device to the internet** (port forwarding)
   - Then: Implement HTTPS, strong auth, rate limiting
   
2. **You share it with others** (friends, family, etc.)
   - Then: Add web authentication, audit logging
   
3. **You use it in public spaces** (makerspace, office, etc.)
   - Then: Consider all production security measures
   
4. **You want to learn security** (educational goal)
   - Then: Implement HTTPS as a learning exercise

For a personal device in your home: **Basic security is sufficient.**

---

## Resources

### Learn More About ESP32 Security:
- [ESP32 Secure Boot](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/secure-boot-v2.html)
- [ESP32 Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html)
- [ESP HTTPS Server](https://github.com/fhessel/esp32_https_server)

### Certificate Generation:
- [OpenSSL Essentials](https://www.digitalocean.com/community/tutorials/openssl-essentials-working-with-ssl-certificates-private-keys-and-csrs)
- [mkcert](https://github.com/FiloSottile/mkcert) - Easy local CA creation

### ESP32 Security Best Practices:
- [ESP32 Security Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/index.html)

---

## Conclusion

**For your hobby project:**
- ✅ Add AP password
- ✅ Add input validation
- ✅ Optional: Basic auth
- ❌ Skip HTTPS unless you want to learn it

**HTTPS on `.local` domain:**
- Technically possible
- Significant complexity
- Limited benefit for local-only access
- Better: Use HTTP + other security measures

**Remember:** Perfect security is the enemy of done. For a hobby project, "good enough" security is... good enough! Focus on building cool stuff, not building Fort Knox. 🔐

---

**Last Updated:** December 14, 2024  
**Next Review:** When project requirements change or device is shared/exposed

