# Migrating to ESPAsyncWebServer

This document outlines the considerations and steps for migrating EasyWiFi from the synchronous `WebServer` to the asynchronous `ESPAsyncWebServer`.

## Current Implementation

EasyWiFi currently uses the **synchronous WebServer** from the ESP32 Arduino framework:

- **Library**: `WebServer` (built-in to ESP32 Arduino)
- **Architecture**: Blocking - requires `server.handleClient()` in loop
- **Concurrent Requests**: No - handles one request at a time
- **Memory Pattern**: Builds full response strings before sending

## Why Consider AsyncWebServer?

### Benefits

1. **Non-blocking Operation**
   - Doesn't block the main loop while serving requests
   - Other code continues running during HTTP operations
   - Better for real-time applications (LED control, sensors, etc.)

2. **Concurrent Connections**
   - Handle multiple clients simultaneously
   - Users don't experience blocking when another user is active
   - Better scalability for multi-user scenarios

3. **Streaming Support**
   - Send large responses without building full strings in memory
   - Reduced memory pressure for large HTML pages
   - Better for resource-constrained devices

4. **WebSocket Support**
   - Built-in WebSocket implementation
   - Enables real-time bidirectional communication
   - Perfect for live LED control, status updates, etc.

5. **Better Responsiveness**
   - UI remains responsive during long operations (WiFi scans, network connections)
   - Reduced latency for concurrent requests
   - More modern async/await-like patterns

### Trade-offs

1. **API Complexity**
   - Different handler signatures
   - More complex request/response handling
   - Steeper learning curve

2. **Flash Footprint**
   - Slightly larger binary size (~10-20KB)
   - Additional AsyncTCP dependency

3. **Debugging**
   - Different error handling patterns
   - Less straightforward stack traces
   - More complexity in troubleshooting

4. **Memory Usage**
   - Higher base memory usage
   - More efficient for large responses
   - Trade-off depends on use case

## When to Migrate

### Stay with Synchronous WebServer If:

- ✅ Current performance is acceptable
- ✅ Handling < 5-10 requests per second
- ✅ Simplicity is more important than performance
- ✅ Flash space is limited
- ✅ Single-user access is typical
- ✅ No real-time features needed

### Migrate to ESPAsyncWebServer If:

- ✅ Need WebSocket support (real-time LED control, live updates)
- ✅ Experiencing slow page loads with multiple users
- ✅ Want streaming/chunked responses
- ✅ Multiple concurrent users are common
- ✅ Long-running operations (scans) block the UI
- ✅ Building advanced interactive features

## Migration Effort Estimate

| Task | Complexity | Estimated Time |
|------|-----------|----------------|
| Update dependencies | Low | 5 minutes |
| Change headers and types | Low | 15 minutes |
| Update route handlers | Medium | 1-2 hours |
| Update POST/form handling | Medium | 30 minutes |
| Test all endpoints | High | 2-3 hours |
| Fix edge cases | Medium | 1 hour |
| **Total** | | **~3-5 hours** |

## Migration Steps

### 1. Update Dependencies

**platformio.ini:**
```ini
lib_deps = 
    ArduinoJson
    ESPAsyncWebServer
    AsyncTCP  # Required for ESP32
```

### 2. Update Headers

**ConfigServer.h:**
```cpp
// OLD
#include <WebServer.h>

// NEW
#include <ESPAsyncWebServer.h>
```

**ConfigServer class:**
```cpp
// OLD
WebServer server;

// NEW
AsyncWebServer server;
```

### 3. Update Handler Signatures

**OLD - Synchronous:**
```cpp
void handleRoot() {
    String html = webPages.generateHomePage(wifiManager, storage);
    server.send(200, "text/html", html);
}

server.on("/wifi", HTTP_GET, [this]() { handleRoot(); });
```

**NEW - Async:**
```cpp
void handleRoot(AsyncWebServerRequest *request) {
    String html = webPages.generateHomePage(wifiManager, storage);
    request->send(200, "text/html", html);
}

server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) { 
    handleRoot(request); 
});
```

### 4. Update POST Data Handling

**OLD - Synchronous:**
```cpp
void handleSave() {
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        server.send(400, "application/json", "{\"error\":\"Missing parameters\"}");
        return;
    }
    
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    // Process...
}
```

**NEW - Async:**
```cpp
void handleSave(AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
        return;
    }
    
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    
    // Process...
}
```

**Parameter Types:**
- `hasParam(name, true)` - POST parameters
- `hasParam(name, false)` - GET parameters
- `hasParam(name)` - Either GET or POST

### 5. Update JSON Responses

**OLD - Synchronous:**
```cpp
void handleAPIStatus() {
    JsonDocument doc;
    doc["connected"] = wifiManager.isConnected();
    
    String response;
    serializeJson(doc, response);
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
}
```

**NEW - Async:**
```cpp
void handleAPIStatus(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["connected"] = wifiManager.isConnected();
    
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
}
```

### 6. Update Redirects

**OLD - Synchronous:**
```cpp
void handleCaptivePortal() {
    server.sendHeader("Location", "/wifi", true);
    server.send(302, "text/plain", "");
}
```

**NEW - Async:**
```cpp
void handleCaptivePortal(AsyncWebServerRequest *request) {
    request->redirect("/wifi");
}
```

### 7. Remove handleClient() from Loop

**OLD - ConfigServer.cpp loop():**
```cpp
void ConfigServer::loop() {
    if (enabled) {
        server.handleClient();  // REMOVE THIS
    }
}
```

**NEW - ConfigServer.cpp loop():**
```cpp
void ConfigServer::loop() {
    // Server runs automatically in background
    // Loop can be empty or removed entirely
}
```

### 8. Update Custom Page Handler Interface

**CustomPageHandler.h - Update examples:**
```cpp
// OLD example
void handleCustomPage() {
    String html = webPages->getHTMLHeader("Custom Page");
    html += "<h1>Custom Content</h1>";
    html += webPages->getHTMLFooter();
    configServer.getServer().send(200, "text/html", html);
}

// NEW example
void handleCustomPage(AsyncWebServerRequest *request) {
    String html = webPages->getHTMLHeader("Custom Page");
    html += "<h1>Custom Content</h1>";
    html += webPages->getHTMLFooter();
    request->send(200, "text/html", html);
}
```

## Files Requiring Changes

Based on current codebase structure:

1. **ConfigServer.h** - Update includes and class members
2. **ConfigServer.cpp** - Update all ~17 handler methods (21 `server.on()` registrations)
3. **CustomPageHandler.h** - Update interface and examples
4. **RunLoop.cpp** - Remove or simplify loop() call
5. **Documentation** - Update API.md and CustomPages.md examples

## Advanced Async Features

Once migrated, you can leverage advanced features:

### Streaming Responses

```cpp
void handleLargeFile(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse(
        "text/html",
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            // Generate content in chunks
            // Return 0 when done
            return 0;
        }
    );
    request->send(response);
}
```

### WebSocket Support

```cpp
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DATA) {
        // Handle incoming data
    }
}

void setup() {
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
}

// Send to all clients
void broadcastStatus() {
    ws.textAll("{\"status\":\"connected\"}");
}
```

### Server-Sent Events (SSE)

```cpp
AsyncEventSource events("/events");

void setup() {
    events.onConnect([](AsyncEventSourceClient *client) {
        client->send("connected", NULL, millis(), 1000);
    });
    server.addHandler(&events);
}

// Push updates to clients
void sendUpdate() {
    events.send("data", "status", millis());
}
```

## Testing Checklist

After migration, verify all endpoints:

- [ ] `/` - Root page (if custom handler registered)
- [ ] `/wifi` - WiFi configuration home
- [ ] `/wifi/scan` - Network scan page
- [ ] `/wifi/configure` - WiFi configuration form
- [ ] `/wifi/save` - Save WiFi credentials (POST)
- [ ] `/wifi/status` - Connection status page
- [ ] `/wifi/credentials` - Manage stored networks
- [ ] `/wifi/reset` - Factory reset
- [ ] `/wifi/api/status` - Status API (GET)
- [ ] `/wifi/api/scan` - Scan API (GET)
- [ ] `/wifi/api/configure` - Configure API (POST)
- [ ] `/wifi/api/connect` - Connect API (POST)
- [ ] `/wifi/api/credentials` - Credentials API (GET/DELETE)
- [ ] `/wifi/api/reset` - Reset API (GET)
- [ ] `/wifi/api/health` - Health check API (GET)
- [ ] Captive portal detection redirects
- [ ] 404 handler
- [ ] Custom page handlers

## Performance Comparison

> **Note:** The figures below are illustrative estimates for typical ESP32 use, not measurements taken from this project. Benchmark on your target hardware before relying on them.

### Memory Usage

**Synchronous WebServer:**
- Base: ~8KB RAM
- Per request: ~2-4KB (full response in memory)
- Peak: ~15KB during large page generation

**ESPAsyncWebServer:**
- Base: ~12KB RAM
- Per request: ~1-2KB (chunked responses)
- Peak: ~18KB with multiple concurrent connections

### Response Time

**Synchronous (single user):**
- Simple page: ~50-100ms
- Scan page: 3-5 seconds (blocks everything)
- API call: ~20-50ms

**Async (single user):**
- Simple page: ~30-80ms (20-30% faster)
- Scan page: 3-5 seconds (doesn't block other requests)
- API call: ~15-40ms

**Async (multiple concurrent users):**
- All requests handled simultaneously
- No blocking between users
- Better overall throughput

## Recommendation for EasyWiFi

**Current State:** The synchronous WebServer is adequate for:
- Configuration during initial setup (single user)
- Occasional settings changes
- Simple status checks

**Consider Async if adding:**
- Real-time LED control with color picker
- Live WiFi signal strength monitoring
- Multiple devices controlling the same unit
- WebSocket-based live updates
- Streaming large configuration files

**Migration Priority:** Low to Medium
- Current implementation works well
- Migration effort is moderate
- Main benefit would be for future real-time features

## References

- [ESPAsyncWebServer GitHub](https://github.com/me-no-dev/ESPAsyncWebServer)
- [AsyncTCP Library](https://github.com/me-no-dev/AsyncTCP)
- [ESP32 Arduino WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)

## Conclusion

Migrating to ESPAsyncWebServer is worthwhile if you plan to add interactive, real-time features. The effort is moderate (3-5 hours), and the benefits are significant for concurrent usage and responsive UX. However, the current synchronous implementation is adequate for basic WiFi configuration use cases.

**Decision Point:** Evaluate based on your planned features and whether WebSocket/real-time capabilities would enhance the user experience.
