# EasyWiFi - Product Requirements Document

## Project Overview

EasyWiFi is an ESP32-C3 firmware project that provides intelligent WiFi connectivity with automatic fallback to Access Point (AP) mode when no configured WiFi network is available. This enables users to configure WiFi credentials through a web interface without requiring hardcoded credentials or serial console access.

## Goals

1. **Seamless WiFi Management**: Automatically connect to known WiFi networks
2. **Easy Configuration**: Provide web-based interface for WiFi setup when no network is available
3. **User-Friendly**: Display available SSIDs and allow password entry through intuitive UI
4. **Robust Architecture**: Build on proven patterns from GlowBoxen project
5. **Production Ready**: Include persistent storage, logging, and error handling

## Reference Architecture (from GlowBoxen)

The GlowBoxen project demonstrates several key patterns we will adopt:

### Component Architecture
- **Networking**: WiFi management, scanning, connection handling
- **REST**: Web server for API endpoints
- **RunLoop**: Main orchestration layer
- **Logging**: ESP-IDF logging system with configurable levels

### Key Technologies
- **WebServer**: ESP32 WebServer library for HTTP endpoints
- **WiFiMulti**: Multiple SSID support with automatic selection
- **mDNS**: Service discovery and hostname management
- **ESP Logging**: Structured logging with `ESP_LOG*` macros

## Core Features

### 1. WiFi Station Mode (Client)
- Attempt to connect to stored WiFi credentials on boot
- Support multiple stored SSIDs with priority
- Automatic reconnection on disconnect
- Signal strength monitoring
- Connection timeout handling

### 2. WiFi Access Point Mode (Fallback)
- Activate when no stored credentials exist or connection fails
- Configurable SSID (e.g., "EasyWiFi-Setup-XXXX" with device ID)
- Open or WPA2 secured AP
- Captive portal support (optional)
- Auto-disable AP after successful configuration

### 3. WiFi Network Scanning
- Scan for available networks
- Display SSID, signal strength (RSSI), and encryption type
- Sort by signal strength
- Refresh capability
- Filter hidden networks

### 4. Web Configuration Interface
- **Landing Page**: Status display and navigation
- **WiFi Scan Page**: List available networks with signal indicators
- **Configuration Form**: 
  - SSID selection (dropdown from scan results)
  - Password input field
  - Manual SSID entry option
  - Form validation
- **Status Page**: Connection progress and feedback
- **Success Page**: Confirm connection and provide next steps

### 5. Persistent Storage
- Store WiFi credentials in NVS (Non-Volatile Storage)
- Support multiple credential sets (up to 5 networks)
- Clear credentials API endpoint
- Factory reset capability

### 6. REST API
Endpoints for programmatic access:
- `GET /` - Root redirect or status page
- `GET /api/status` - WiFi status and device info
- `GET /api/scan` - Trigger scan and return available networks
- `POST /api/configure` - Save WiFi credentials
- `POST /api/connect` - Attempt connection with provided credentials
- `DELETE /api/credentials` - Clear stored credentials
- `GET /api/reset` - Factory reset (with confirmation)

## Technical Architecture

### File Structure
```
EasyWiFi/
├── platformio.ini              # Build configuration
├── PRD.md                      # This document
├── README.md                   # User documentation
├── src/
│   └── main.cpp               # Application entry point
├── lib/
│   └── EasyWiFi/
│       ├── EasyWiFi.h         # Main include file
│       ├── RunLoop.h/.cpp     # Main orchestration
│       ├── WiFiManager.h/.cpp # WiFi connection & AP management
│       ├── ConfigServer.h/.cpp # Web server for configuration
│       ├── Storage.h/.cpp     # NVS credential storage
│       ├── WebPages.h/.cpp    # HTML/CSS/JS content
│       └── Macros.h           # Utility macros and constants
└── include/
    └── README                 # Include directory info
```

### Class Responsibilities

#### WiFiManager
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
    static const int CONNECTION_TIMEOUT = 10000;
    static const int MAX_STORED_NETWORKS = 5;
};
```

#### ConfigServer
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
    
    // HTML generation
    String generateHomePage();
    String generateScanPage(const std::vector<WiFiNetwork>& networks);
    String generateConfigPage(const String& ssid);
    String generateStatusPage();
    String generateSuccessPage();
};
```

#### Storage
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

#### RunLoop
```cpp
class RunLoop {
public:
    RunLoop();
    void setup();
    void loop();
private:
    WiFiManager wifiManager;
    ConfigServer configServer;
    Storage storage;
    
    enum State {
        INITIALIZING,
        CONNECTING,
        CONNECTED,
        AP_MODE,
        ERROR
    };
    State currentState;
    
    void transitionToState(State newState);
    void handleConnecting();
    void handleConnected();
    void handleAPMode();
};
```

## Implementation Phases

### Phase 1: Core WiFi Management (Foundation)
**Goal**: Basic WiFi connection and scanning functionality

**Tasks**:
1. Set up project structure and platformio.ini dependencies
2. Implement WiFiManager class:
   - WiFi connection to hardcoded credentials (for testing)
   - Network scanning with RSSI and encryption detection
   - Connection status monitoring
3. Implement Storage class:
   - NVS initialization
   - Save/load/clear credentials
   - Multiple network support
4. Basic RunLoop with state machine
5. Serial console logging and status output

**Deliverables**:
- WiFi connects to stored networks automatically
- Scans and displays available networks
- Credentials persist across reboots

### Phase 2: Access Point Mode
**Goal**: Fallback AP mode when no WiFi available

**Tasks**:
1. Extend WiFiManager:
   - Start AP mode with unique SSID
   - Handle AP lifecycle (start/stop)
   - Detect when AP is needed (no credentials or connection failure)
2. Update RunLoop:
   - Add AP_MODE state
   - Transition logic between modes
   - Auto-disable AP after successful configuration
3. mDNS hostname registration
4. LED status indicator (optional, if LED available)

**Deliverables**:
- Device creates AP when no WiFi configured
- AP has discoverable name (e.g., EasyWiFi-AABBCC)
- LED indicates current mode

### Phase 3: Web Server & Configuration UI
**Goal**: Full web-based configuration interface

**Tasks**:
1. Implement ConfigServer class:
   - HTTP server on port 80
   - Route handlers for all endpoints
   - JSON response formatting
2. Implement WebPages class:
   - HTML templates with embedded CSS
   - Responsive design for mobile devices
   - JavaScript for dynamic interactions
   - Form validation
3. Connect WebPages to ConfigServer routes
4. Test configuration flow end-to-end

**Deliverables**:
- Complete web UI accessible in AP mode
- Users can scan, select, and configure WiFi
- Status feedback during connection attempts

### Phase 4: REST API
**Goal**: Programmatic access for advanced users

**Tasks**:
1. JSON API endpoints:
   - `GET /api/status` - Device and WiFi status
   - `GET /api/scan` - Network scan results
   - `POST /api/configure` - Set credentials
   - `POST /api/connect` - Test connection
   - `DELETE /api/credentials` - Clear config
2. API documentation in README.md
3. Error handling and HTTP status codes
4. CORS headers (optional)

**Deliverables**:
- Full REST API documented
- curl examples for testing
- Postman/API test scripts

### Phase 5: Polish & Production Features
**Goal**: Production-ready firmware

**Tasks**:
1. Captive portal support (redirect to config page)
2. Connection timeout and retry logic
3. Watchdog timer for crash recovery
4. Memory monitoring and leak detection
5. Security hardening:
   - Rate limiting on API endpoints
   - CSRF protection (for POST requests)
   - Optional AP password
6. Comprehensive error messages
7. Factory reset button support (GPIO trigger)
8. OTA update capability (optional, from GlowBoxen)

**Deliverables**:
- Robust, production-ready firmware
- User documentation
- Deployment guide

## Configuration & Build

### platformio.ini Updates
```ini
[env:esp32c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
debug_tool = esp-builtin
build_type = debug
build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DCONFIG_LOG_MAXIMUM_LEVEL_VERBOSE=y
    -DCONFIG_LOG_MAXIMUM_LEVEL=5
lib_deps = 
    ArduinoJson
```

### Build Commands
```bash
# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean
```

## User Experience Flow

### First Boot (No Configuration)
1. Device boots and checks for stored credentials
2. None found → transitions to AP mode
3. LED blinks slowly (AP mode indicator)
4. Creates WiFi network "EasyWiFi-Setup-[DeviceID]"
5. User connects to AP from phone/laptop
6. Captive portal auto-redirects to config page (or user visits 192.168.4.1)
7. User clicks "Scan for Networks"
8. Available networks displayed with signal strength
9. User selects network and enters password
10. User clicks "Connect"
11. Device attempts connection with feedback
12. On success: AP shuts down, device connects to WiFi, LED solid
13. On failure: Returns to config page with error message

### Subsequent Boots (Configured)
1. Device boots and loads credentials from NVS
2. Attempts connection to stored networks
3. Connection successful → LED solid, runs main application
4. Connection failed → Falls back to AP mode (repeat first boot flow)

### Reconfiguration
1. User can factory reset via:
   - API endpoint: `GET /api/reset`
   - Button press (if GPIO button connected)
   - Serial command
2. Credentials cleared, device reboots into AP mode

## Web UI Design Considerations

### Responsive Design
- Mobile-first approach
- Touch-friendly buttons (min 44x44px)
- Single-column layout for simplicity
- System font stack (no external dependencies)

### Accessibility
- Semantic HTML
- ARIA labels for screen readers
- Sufficient color contrast
- Keyboard navigation support

### Performance
- Embedded CSS (no external files)
- Minimal JavaScript
- Static HTML generation on ESP32
- Cached responses where appropriate

### Visual Design
- Clean, modern aesthetic
- Signal strength icons (WiFi bars)
- Lock icon for secured networks
- Loading spinners for async operations
- Success/error message styling

## Testing Strategy

### Unit Testing
- WiFiManager credential management
- Storage read/write operations
- State machine transitions
- HTTP request parsing

### Integration Testing
- Complete configuration flow
- AP → Station mode transition
- Multiple network scenarios
- Persistence across reboots

### Manual Testing Scenarios
1. First boot with no credentials
2. Connection to valid network
3. Connection to wrong password
4. Connection to non-existent SSID
5. Network dropout and reconnection
6. Multiple devices connecting to AP
7. Factory reset
8. Power loss during configuration

### Device Testing
- Various ESP32-C3 boards
- Different WiFi routers (2.4GHz)
- Various encryption types (WPA2, WPA3)
- Range testing
- Concurrent connections

## Security Considerations

### Credentials Storage
- Use ESP32 NVS with encryption
- Consider using flash encryption in production
- No credentials in source code
- Clear credentials on factory reset

### AP Mode Security
- Optional WPA2 password for AP
- Disable AP after successful configuration
- Timeout AP mode after N minutes of inactivity

### Web Interface Security
- Rate limiting on credential submission
- Basic CSRF protection
- Input validation and sanitization
- HTTPS optional (more complex, requires cert management)

### Network Security
- Validate SSID and password formats
- Prevent injection attacks
- Clear sensitive data from memory after use

## Success Metrics

### Functional Requirements
- ✅ Connects to WiFi on first attempt (with valid credentials)
- ✅ Falls back to AP mode when connection fails
- ✅ Web UI accessible and functional
- ✅ Credentials persist across reboots
- ✅ Factory reset clears all credentials

### Performance Requirements
- WiFi connection established within 10 seconds
- AP mode activated within 5 seconds of boot
- Network scan completes within 5 seconds
- Web page loads within 2 seconds
- Configuration save completes within 1 second

### Reliability Requirements
- No crashes during 24-hour stress test
- Successful reconnection after network dropout
- Graceful handling of invalid credentials
- Recovery from low-memory conditions

## Future Enhancements (Post-MVP)

1. **Multi-Network Priority**: Allow users to set priority order for multiple networks
2. **Scheduled Connections**: Connect to specific networks at specific times
3. **WiFi Profiles**: Save complete network profiles with static IP, DNS, etc.
4. **Bluetooth Configuration**: Alternative configuration via BLE
5. **Mobile App**: Native iOS/Android app for configuration
6. **Network Monitoring**: Continuous RSSI monitoring and roaming
7. **Guest Network**: Separate AP for guests while connected to primary network
8. **Mesh Networking**: ESP-MESH protocol support
9. **Advanced Logging**: Remote logging to syslog server
10. **MQTT Integration**: Publish status updates via MQTT

## References

### GlowBoxen Project
- **Location**: `../GlowBoxen`
- **Key Files**:
  - `lib/GlowBoxen/Networking.h/cpp` - WiFi management patterns
  - `lib/GlowBoxen/REST.h/cpp` - WebServer implementation
  - `lib/GlowBoxen/RunLoop.h/cpp` - Main orchestration
  - `lib/GlowBoxen/OTA.h/cpp` - Over-the-air updates

### ESP32 Documentation
- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [Arduino WiFi Library](https://www.arduino.cc/reference/en/libraries/wifi/)
- [ESP32 WebServer Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)
- [Preferences (NVS) Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)

### Design Patterns
- State Machine pattern for mode management
- Observer pattern for WiFi events
- Factory pattern for web page generation
- Singleton pattern for storage access

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-12-13 | Initial | Initial PRD creation based on GlowBoxen reference |

---

**Document Status**: Draft  
**Next Review**: After Phase 1 completion  
**Owner**: Brennan  
**Project**: EasyWiFi ESP32-C3 Firmware

