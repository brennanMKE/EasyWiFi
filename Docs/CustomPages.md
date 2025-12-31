# Custom Web Pages Guide

This guide explains how to add custom web pages to your EasyWiFi-based device for your device-specific functionality.

## Overview

EasyWiFi provides a plugin architecture that allows you to add custom web routes while keeping your device-specific code separate from the WiFi configuration library.

**Key Concept:** Your custom pages are the primary functionality at the root URL (`/`), while WiFi configuration is available at `/wifi`. This puts your device's main features front and center, with WiFi setup as a supporting service.

Your custom pages will:

- Be served at the root URL (`/`, `/control`, `/settings`, etc.)
- Share the same HTML/CSS styling as the WiFi config pages
- Have access to WiFi status, storage, and other EasyWiFi components
- Be served from the same web server (no port conflicts)
- Coexist with the built-in WiFi configuration routes at `/wifi`

## Architecture

```
┌─────────────────────────────────────────┐
│           Your Custom Code              │
│         (src/LanternsPages.cpp)         │
│                                         │
│  - Implements CustomPageHandler         │
│  - Registers custom routes at root      │
│  - Uses WebPages helpers for styling    │
│  - Accesses WiFi/Storage components     │
└──────────────┬──────────────────────────┘
               │
               │ registers with
               ▼
┌─────────────────────────────────────────┐
│         EasyWiFi Library                │
│                                         │
│  ConfigServer ─┬─ Your Custom Routes    │
│                │  (/, /control, etc)    │
│                │                         │
│                └─ WiFi Config Routes     │
│                   (/wifi, /wifi/scan)   │
└─────────────────────────────────────────┘
```

## Quick Start

### 1. Create Your Custom Handler Class

Create `src/MyPages.h`:

```cpp
#ifndef MYPAGES_H
#define MYPAGES_H

#include <CustomPageHandler.h>
#include <ConfigServer.h>
#include <WebServer.h>

class MyPages : public CustomPageHandler {
public:
    MyPages(ConfigServer& configServer);
    void registerRoutes() override;
    
private:
    ConfigServer& configServer;
    void handleHomePage();
};

#endif
```

### 2. Implement Your Handler

Create `src/MyPages.cpp`:

```cpp
#include "MyPages.h"

MyPages::MyPages(ConfigServer& cs) : configServer(cs) {}

void MyPages::registerRoutes() {
    WebServer& server = configServer.getServer();
    
    // Register your custom routes at root (primary device functionality)
    server.on("/", HTTP_GET, 
        std::bind(&MyPages::handleHomePage, this));
    server.on("/control", HTTP_GET, 
        std::bind(&MyPages::handleControl, this));
}

void MyPages::handleHomePage() {
    // Use WebPages helpers for consistent styling
    String html = webPages->getHTMLHeader("My Device");
    html += "<body>";
    
    html += "<div class='card'>";
    html += "<h1>My Custom Page</h1>";
    
    // Access WiFi status
    if (wifiManager->isConnected()) {
        html += "<p>WiFi: " + WiFi.SSID() + "</p>";
    }
    
    // Your custom content here
    html += "<a href='/control' class='button primary'>Control</a>";
    html += "<a href='/wifi' class='button small'>WiFi Setup</a>";
    html += "</div>";
    
    html += webPages->getHTMLFooter();
    
    configServer.getServer().send(200, "text/html", html);
}
```

### 3. Register in main.cpp

Update `src/main.cpp`:

```cpp
#include <Arduino.h>
#include <EasyWiFi.h>
#include "MyPages.h"

RunLoop runloop;
MyPages* myPages = nullptr;

void setup() {
    Serial.begin(115200);
    
    // Initialize EasyWiFi
    runloop.setup("MyDevice");
    
    // Register custom pages
    myPages = new MyPages(runloop.getConfigServer());
    runloop.getConfigServer().registerCustomHandler(myPages);
}

void loop() {
    runloop.loop();
}
```

## Available Components

Your custom handler has access to these protected members:

### wifiManager

Check WiFi status and signal strength:

```cpp
if (wifiManager->isConnected()) {
    String ssid = WiFi.SSID();
    int rssi = WiFi.RSSI();
    IPAddress ip = WiFi.localIP();
}
```

### storage

Read/write persistent data:

```cpp
// Load WiFi credentials
std::vector<WiFiCredential> credentials;
storage->loadCredentials(credentials);

// Access Preferences directly for custom settings
Preferences prefs;
prefs.begin("mydevice", false);
int value = prefs.getInt("setting", 0);
prefs.putInt("setting", 42);
prefs.end();
```

### webPages

Use HTML helpers for consistent styling:

```cpp
String html = webPages->getHTMLHeader("Page Title");
// ... add your content ...
html += webPages->getHTMLFooter();
```

## HTML Styling

Use these CSS classes for consistent styling:

### Cards and Containers

```html
<div class='card'>
    <h1>Title</h1>
    <p>Content</p>
</div>
```

### Buttons

```html
<!-- Primary action button -->
<a href='/action' class='button primary'>Primary Action</a>

<!-- Regular button -->
<a href='/page' class='button'>Go to Page</a>

<!-- Small button -->
<a href='/back' class='button small'>Back</a>

<!-- Button group (horizontal layout) -->
<div class='button-group'>
    <a href='/option1' class='button'>Option 1</a>
    <a href='/option2' class='button'>Option 2</a>
    <a href='/option3' class='button'>Option 3</a>
</div>
```

### Status Messages

```html
<!-- Success message (green) -->
<div class='status success'>✓ Connected</div>

<!-- Warning message (yellow) -->
<div class='status warning'>⚠ Warning</div>

<!-- Error message (red) -->
<div class='status error'>✗ Error</div>
```

## REST API Endpoints

Create JSON API endpoints alongside your web pages:

```cpp
void MyPages::registerRoutes() {
    WebServer& server = configServer.getServer();
    
    // Web UI - at root for primary functionality
    server.on("/", HTTP_GET, 
        std::bind(&MyPages::handleHomePage, this));
    server.on("/control", HTTP_GET, 
        std::bind(&MyPages::handleControlPage, this));
    
    // REST API
    server.on("/api/mydevice/status", HTTP_GET, 
        std::bind(&MyPages::handleAPIStatus, this));
    server.on("/api/mydevice/control", HTTP_POST, 
        std::bind(&MyPages::handleAPIControl, this));
}

void MyPages::handleAPIStatus() {
    JsonDocument doc;
    doc["success"] = true;
    doc["value"] = 42;
    
    String response;
    serializeJson(doc, response);
    
    configServer.getServer().sendHeader("Access-Control-Allow-Origin", "*");
    configServer.getServer().send(200, "application/json", response);
}
```

## URL Structure

After setup, your device will have these routes:

### Your Custom Routes (at root - primary functionality)
- `/` - Your device home page
- `/control` - Your control page  
- `/settings` - Your settings page
- `/api/mydevice/status` - Your status API
- (any other routes you register)

### EasyWiFi Routes (under /wifi - supporting service)
- `/wifi` - WiFi configuration home
- `/wifi/scan` - Scan for networks
- `/wifi/credentials` - Manage stored networks
- `/wifi/api/status` - WiFi status API
- `/wifi/api/health` - System health API
- `/wifi/api/scan` - Scan networks API
- `/wifi/api/credentials` - Manage credentials API

## Example: Complete Custom Handler

See `src/LanternsPages.h` and `src/LanternsPages.cpp` for a complete example implementation with:

- Multiple web UI pages
- REST API endpoints with JSON responses
- WiFi status display
- Navigation between pages
- Form handling

## Tips and Best Practices

### 1. Keep Device Code Separate

Your custom functionality lives in `src/`, while EasyWiFi stays in `lib/EasyWiFi/`. This keeps the library reusable.

### 2. Use Consistent Styling

Use the WebPages helpers (`getHTMLHeader()`, `getHTMLFooter()`) so your pages match the WiFi config UI.

### 3. Link to WiFi Configuration

Always provide a way to access WiFi configuration from your custom pages:

```html
<a href='/wifi' class='button small'>🌐 WiFi Setup</a>
```

### 4. Non-Blocking Operations

Follow the same non-blocking patterns as EasyWiFi:
- Use `millis()` for timing instead of `delay()`
- Call `yield()` in long loops
- Keep handler methods quick

### 5. Memory Management

ESP32-C3 has limited RAM. Be mindful when building large HTML strings:

```cpp
// Reserve space to reduce fragmentation
String html;
html.reserve(2048);
html = webPages->getHTMLHeader("Title");
// ... build page ...
```

### 6. CORS for APIs

For REST API endpoints that may be called from JavaScript:

```cpp
void handleAPI() {
    configServer.getServer().sendHeader("Access-Control-Allow-Origin", "*");
    // ... send response ...
}
```

### 7. Root Route Priority

Your custom handler should register a route for `/` to serve as the device's home page. This will be the first page users see when they navigate to your device.

## Testing Your Custom Pages

1. Flash your firmware to the ESP32
2. Connect to your WiFi network (or the device's AP)
3. Navigate to:
   - `http://your-device-name-xxxxxx.local/` (mDNS) - Your custom home page
   - `http://192.168.x.x/` (direct IP) - Your custom home page
   - `http://your-device-name-xxxxxx.local/wifi` - WiFi configuration
   - `http://192.168.x.x/wifi` - WiFi configuration

## Troubleshooting

### Custom routes not working

- Verify `registerCustomHandler()` is called in `setup()` AFTER `runloop.setup()`
- Check that routes are registered in `registerRoutes()`
- Your custom routes can use any path except those under `/wifi` and `/api`
- The root `/` should be registered by your custom handler for your device home page

### Web pages not styled correctly

- Make sure you're using `webPages->getHTMLHeader()` and `getHTMLFooter()`
- The header includes all CSS and page structure

### Can't access WiFi status

- Verify your class inherits from `CustomPageHandler`
- Check that `wifiManager`, `storage`, and `webPages` pointers are not null
- These are set by `ConfigServer::registerCustomHandler()`

## Advanced: Multiple Custom Handlers

You can register multiple handlers for different features:

```cpp
void setup() {
    runloop.setup("MyDevice");
    
    // Register multiple features
    lightsPages = new LightsPages(runloop.getConfigServer());
    runloop.getConfigServer().registerCustomHandler(lightsPages);
    
    sensorsPages = new SensorsPages(runloop.getConfigServer());
    runloop.getConfigServer().registerCustomHandler(sensorsPages);
}
```

Each handler manages its own set of routes and functionality.

---

For more examples, see the complete `LanternsPages` implementation in `src/`.

