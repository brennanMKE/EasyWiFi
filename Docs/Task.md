# EasyWiFi Implementation Task List

This document tracks the implementation tasks for the EasyWiFi project. Tasks are organized by implementation phase as outlined in PRD.md.

**Status Legend:**
- ⬜ Not Started
- 🔄 In Progress
- ✅ Complete
- ❌ Blocked/Cancelled

---

## Phase 1: Library Foundation

### Task 1: Create Library Structure
**Status:** ✅ Complete

Create the EasyWiFi library directory structure in `lib/EasyWiFi/`:
```
lib/EasyWiFi/
├── library.json
├── EasyWiFi.h
├── Macros.h
├── Storage.h
├── Storage.cpp
├── WiFiManager.h
├── WiFiManager.cpp
├── ConfigServer.h
├── ConfigServer.cpp
├── WebPages.h
├── WebPages.cpp
├── RunLoop.h
└── RunLoop.cpp
```

**Acceptance Criteria:**
- [ ] Directory structure created
- [ ] All header files have include guards
- [ ] Basic file structure follows GlowBoxen patterns

---

### Task 2: Create library.json Manifest
**Status:** ✅ Complete

Create `lib/EasyWiFi/library.json` with library metadata.

**Contents:**
```json
{
  "name": "EasyWiFi",
  "version": "0.1.0",
  "description": "WiFi configuration library with AP fallback and web interface",
  "keywords": "esp32, wifi, configuration, access-point, web-server",
  "authors": {
    "name": "Brennan"
  },
  "license": "MIT",
  "frameworks": "arduino",
  "platforms": "espressif32",
  "dependencies": {
    "ArduinoJson": "*"
  }
}
```

**Acceptance Criteria:**
- [ ] library.json created with correct metadata
- [ ] Dependencies specified
- [ ] Version follows semantic versioning

---

### Task 3: Create Macros.h
**Status:** ✅ Complete

Create `lib/EasyWiFi/Macros.h` with constants and utility macros.

**Contents:**
- WiFi configuration constants (timeouts, retry counts)
- AP configuration (SSID prefix, password)
- Server configuration (port, endpoints)
- Logging tags
- Status codes

**Acceptance Criteria:**
- [ ] All constants defined
- [ ] Proper include guards
- [ ] Documentation comments for each constant

---

## Phase 2: Core Components - Storage

### Task 4: Implement Storage Class
**Status:** ✅ Complete

Create `lib/EasyWiFi/Storage.h` and `Storage.cpp` for NVS credential management.

**Class Interface:**
```cpp
class Storage {
public:
    Storage();
    bool begin();
    bool saveCredentials(const String& ssid, const String& password);
    bool loadCredentials(std::vector<WiFiCredential>& credentials);
    bool clearCredentials();
    bool isConfigured();
private:
    Preferences preferences;
    static const char* NAMESPACE;
    static const char* KEY_COUNT;
    static const char* KEY_SSID_PREFIX;
    static const char* KEY_PASS_PREFIX;
};
```

**Acceptance Criteria:**
- [ ] Class compiles without errors
- [ ] begin() initializes NVS
- [ ] saveCredentials() stores to NVS
- [ ] loadCredentials() retrieves from NVS
- [ ] clearCredentials() wipes NVS
- [ ] isConfigured() checks for existing credentials
- [ ] Proper error handling and logging

---

## Phase 3: Core Components - WiFiManager

### Task 5: Implement WiFiManager Class - Basic Structure
**Status:** ✅ Complete

Create `lib/EasyWiFi/WiFiManager.h` and `WiFiManager.cpp` with basic structure.

**Class Interface:**
```cpp
class WiFiManager {
public:
    WiFiManager();
    bool connectToStoredNetworks();
    bool startAccessPoint();
    void stopAccessPoint();
    bool scanNetworks(std::vector<WiFiNetwork>& results);
    bool addCredentials(const String& ssid, const String& password);
    bool clearCredentials();
    String getStatus();
    bool isConnected();
    bool isAPMode();
private:
    WiFiMulti wifiMulti;
    String apSSID;
    String apPassword;
    bool apActive;
};
```

**Acceptance Criteria:**
- [ ] Header and implementation files created
- [ ] All public methods stubbed
- [ ] Class compiles
- [ ] Constructor initializes member variables

---

### Task 6: Add WiFi Scanning Capability
**Status:** ✅ Complete

Implement `scanNetworks()` method in WiFiManager.

**Requirements:**
- Scan for available networks
- Return SSID, RSSI, encryption type
- Sort by signal strength
- Handle scan errors
- Based on GlowBoxen's `Networking::scanForWiFiNetworks()`

**Acceptance Criteria:**
- [ ] scanNetworks() returns vector of WiFiNetwork structs
- [ ] Results sorted by RSSI (strongest first)
- [ ] Encryption type correctly identified
- [ ] Scan timeout handled
- [ ] Logging for scan results

---

### Task 7: Add WiFi Connection Logic
**Status:** ✅ Complete

Implement `connectToStoredNetworks()` method in WiFiManager.

**Requirements:**
- Use WiFiMulti for multiple network support
- Connection timeout handling
- Retry logic with configurable attempts
- Status logging
- Based on GlowBoxen's `Networking::connect()`

**Acceptance Criteria:**
- [ ] connectToStoredNetworks() attempts connection
- [ ] Timeout after configured duration
- [ ] Returns true/false for success/failure
- [ ] Logs connection attempts and results
- [ ] isConnected() accurately reflects status

---

### Task 8: Add Access Point Mode
**Status:** ✅ Complete

Implement `startAccessPoint()` and `stopAccessPoint()` methods.

**Requirements:**
- Generate unique AP SSID (e.g., "EasyWiFi-Setup-AABBCC")
- Configure AP with optional password
- Set static IP (192.168.4.1)
- Enable/disable AP mode
- Track AP state

**Acceptance Criteria:**
- [ ] startAccessPoint() creates WiFi AP
- [ ] Unique SSID generated from device MAC
- [ ] Static IP assigned correctly
- [ ] stopAccessPoint() disables AP
- [ ] isAPMode() accurately reflects status
- [ ] AP accessible from client devices

---

## Phase 4: Web Interface

### Task 9: Implement ConfigServer Class - Basic Structure
**Status:** ✅ Complete

Create `lib/EasyWiFi/ConfigServer.h` and `ConfigServer.cpp` for web server.

**Class Interface:**
```cpp
class ConfigServer {
public:
    ConfigServer(WiFiManager& wifiManager);
    void setup();
    void loop();
    void enable();
    void disable();
private:
    WebServer server;
    WiFiManager& wifiManager;
    
    // Route handlers
    void handleRoot();
    void handleScan();
    void handleConfigure();
    void handleStatus();
    void handleReset();
    void handleNotFound();
};
```

**Acceptance Criteria:**
- [ ] Class structure created
- [ ] WebServer instance on port 80
- [ ] All route handlers stubbed
- [ ] setup() and loop() methods implemented
- [ ] Based on GlowBoxen's REST class pattern

---

### Task 10: Create WebPages Class
**Status:** ✅ Complete

Create `lib/EasyWiFi/WebPages.h` and `WebPages.cpp` with HTML templates.

**Pages Required:**
1. Home/Landing page
2. Network scan page with SSID list
3. Configuration form page
4. Status/progress page
5. Success page
6. Error page

**Acceptance Criteria:**
- [ ] All HTML templates created as String generators
- [ ] Embedded CSS for styling
- [ ] Responsive design (mobile-friendly)
- [ ] Signal strength indicators for WiFi networks
- [ ] Form validation JavaScript (optional)
- [ ] Clean, modern UI design

---

### Task 11: Add Route Handlers to ConfigServer
**Status:** ✅ Complete

Implement all route handlers in ConfigServer.

**Routes:**
- `GET /` - Landing page
- `GET /scan` - Network scan page
- `GET /configure` - Configuration form
- `POST /save` - Save credentials
- `GET /status` - Connection status
- `GET /reset` - Factory reset

**Acceptance Criteria:**
- [ ] All routes registered in setup()
- [ ] Handlers call appropriate WiFiManager methods
- [ ] Handlers return appropriate WebPages HTML
- [ ] POST handler validates input
- [ ] Error handling for all routes
- [ ] Logging for all requests

---

## Phase 5: Integration

### Task 12: Implement RunLoop Class
**Status:** ✅ Complete

Create `lib/EasyWiFi/RunLoop.h` and `RunLoop.cpp` with state machine.

**State Machine:**
```
INITIALIZING → LOADING_CREDENTIALS → CONNECTING → CONNECTED
                                   ↓              ↓ (on failure)
                                   └→ AP_MODE ←───┘
```

**Acceptance Criteria:**
- [ ] State machine implemented
- [ ] Transitions between states work correctly
- [ ] setup() initializes all components
- [ ] loop() calls appropriate component loops
- [ ] Orchestrates WiFiManager, ConfigServer, and Storage
- [ ] Based on GlowBoxen's RunLoop pattern

---

### Task 13: Create EasyWiFi.h Main Include
**Status:** ✅ Complete

Create `lib/EasyWiFi/EasyWiFi.h` as the main include file.

**Contents:**
```cpp
#ifndef EASYWIFI_H
#define EASYWIFI_H

#include "Arduino.h"
#include "RunLoop.h"
#include "WiFiManager.h"
#include "ConfigServer.h"
#include "Storage.h"
#include "WebPages.h"
#include "Macros.h"

#endif // EASYWIFI_H
```

**Acceptance Criteria:**
- [ ] Includes all public headers
- [ ] Include guards correct
- [ ] Compiles without errors
- [ ] Users only need to include this one file

---

### Task 14: Update platformio.ini
**Status:** ✅ Complete

Update `platformio.ini` with required dependencies.

**Changes:**
```ini
lib_deps = 
    ArduinoJson
```

**Acceptance Criteria:**
- [ ] ArduinoJson added to lib_deps
- [ ] Build flags remain unchanged
- [ ] Project compiles successfully

---

### Task 15: Update main.cpp
**Status:** ✅ Complete

Update `src/main.cpp` to use EasyWiFi library.

**Implementation:**
```cpp
#include <Arduino.h>
#include <EasyWiFi.h>

RunLoop runloop;

void setup() {
    Serial.begin(115200);
    delay(500);
    runloop.setup();
}

void loop() {
    runloop.loop();
}
```

**Acceptance Criteria:**
- [ ] main.cpp includes EasyWiFi.h
- [ ] RunLoop instantiated
- [ ] setup() calls runloop.setup()
- [ ] loop() calls runloop.loop()
- [ ] Compiles and uploads successfully
- [ ] Basic functionality works end-to-end

---

## Phase 6: Advanced Features

### Task 16: Add REST API Endpoints
**Status:** ✅ Complete

Add JSON API endpoints to ConfigServer.

**Endpoints:**
- `GET /api/status` - Device and WiFi status
- `GET /api/scan` - Network scan results (JSON)
- `POST /api/configure` - Set credentials (JSON body)
- `POST /api/connect` - Test connection
- `DELETE /api/credentials` - Clear config

**Acceptance Criteria:**
- [ ] All endpoints return valid JSON
- [ ] POST endpoints accept JSON body
- [ ] Proper HTTP status codes
- [ ] Error responses in JSON format
- [ ] Content-Type headers set correctly
- [ ] Documented in README.md with curl examples

---

### Task 17: Implement Captive Portal
**Status:** ⬜ Not Started

Add captive portal redirect logic to ConfigServer.

**Requirements:**
- Redirect DNS queries to ESP32 IP
- Redirect HTTP requests to config page
- Handle common captive portal detection URLs
- Works on iOS, Android, Windows

**Acceptance Criteria:**
- [ ] DNSServer configured in AP mode
- [ ] All DNS queries return ESP32 IP
- [ ] HTTP requests redirect to config page
- [ ] Captive portal triggers on iOS devices
- [ ] Captive portal triggers on Android devices
- [ ] Can disable captive portal via config

---

### Task 18: Add Factory Reset Capability
**Status:** ✅ Complete (via web UI and API)

Implement factory reset functionality.

**Features:**
- Web UI button for reset
- API endpoint `/api/reset`
- Optional GPIO button trigger
- Confirmation mechanism
- Clear all stored data

**Acceptance Criteria:**
- [ ] Reset endpoint requires confirmation
- [ ] Clears all NVS credentials
- [ ] Reboots device after reset
- [ ] Starts in AP mode after reset
- [ ] GPIO button optional (if pin defined)
- [ ] Logging for reset events

---

## Phase 7: Documentation & Testing

### Task 19: Create README.md
**Status:** ⬜ Not Started

Create comprehensive user documentation.

**Sections:**
1. Overview and features
2. Hardware requirements
3. Installation and setup
4. First boot instructions
5. Configuration guide
6. API documentation
7. Troubleshooting
8. Examples

**Acceptance Criteria:**
- [ ] All sections written
- [ ] Screenshots/diagrams included (optional)
- [ ] Code examples provided
- [ ] API endpoints documented with curl examples
- [ ] Troubleshooting common issues

---

### Task 20: Build and Test Complete Flow
**Status:** ⬜ Not Started

End-to-end testing of the complete WiFi configuration flow.

**Test Scenarios:**
1. First boot with no credentials → AP mode
2. Scan networks → select SSID → enter password → connect
3. Successful connection → store credentials → reboot
4. Reboot → auto-connect to stored network
5. Stored network unavailable → fallback to AP mode
6. Wrong password → error message → retry
7. Factory reset → clear credentials → AP mode
8. Multiple stored networks → connect to strongest
9. API endpoint testing (all endpoints)
10. Captive portal detection

**Acceptance Criteria:**
- [ ] All test scenarios pass
- [ ] No crashes or reboots
- [ ] Memory usage acceptable
- [ ] Performance meets requirements
- [ ] Logs provide useful debugging info
- [ ] Ready for production use

---

## Progress Summary

**Total Tasks:** 20

**By Status:**
- ⬜ Not Started: 4
- 🔄 In Progress: 0
- ✅ Complete: 16
- ❌ Blocked: 0

**By Phase:**
- Phase 1 (Foundation): 3 tasks ✅ COMPLETE
- Phase 2 (Storage): 1 task ✅ COMPLETE
- Phase 3 (WiFiManager): 3 tasks ✅ COMPLETE
- Phase 4 (Web Interface): 3 tasks ✅ COMPLETE
- Phase 5 (Integration): 4 tasks ✅ COMPLETE
- Phase 6 (Advanced): 3 tasks (2 remaining)
- Phase 7 (Documentation): 2 tasks

---

## Notes

- Tasks should be completed in order within each phase
- Some tasks can be parallelized across phases
- Mark tasks as 🔄 In Progress when starting
- Mark tasks as ✅ Complete when all acceptance criteria met
- Update this document as tasks progress
- Add notes or blockers as needed

---

**Last Updated:** 2025-12-13  
**Project:** EasyWiFi ESP32-C3 Firmware  
**Reference:** See PRD.md for full requirements

