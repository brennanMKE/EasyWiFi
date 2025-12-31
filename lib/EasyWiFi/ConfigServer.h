#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

#if defined(ESP32)
#include "Arduino.h"
#include <WebServer.h>
#include "WiFiManager.h"
#include "Storage.h"
#include "WebPages.h"
#include "Macros.h"
#include <functional>
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// Forward declarations to avoid circular dependencies
class RunLoop;
class CustomPageHandler;

// Callback type for route handlers
typedef std::function<void()> RouteHandler;

class ConfigServer {
public:
    ConfigServer(WiFiManager& wifiManager, Storage& storage);
    
    void setup();
    void loop();
    void enable();
    void disable();
    
    bool isEnabled();
    void setRunLoop(RunLoop* rl);
    void setDeviceName(const String& name);
    
    // Custom page handler support
    void registerCustomHandler(CustomPageHandler* handler);
    void on(const String& uri, HTTPMethod method, RouteHandler handler);
    
    // Access to components (for custom pages)
    WiFiManager& getWiFiManager() { return wifiManager; }
    Storage& getStorage() { return storage; }
    WebPages& getWebPages() { return webPages; }
    WebServer& getServer() { return server; }

private:
    String deviceName;
    WebServer server;
    WiFiManager& wifiManager;
    Storage& storage;
    WebPages webPages;
    bool enabled;
    RunLoop* runLoop;
    
    // Route handlers - Web UI
    void handleRoot();
    void handleScan();
    void handleConfigure();
    void handleSave();
    void handleStatus();
    void handleReset();
    void handleCredentials();
    
    // Route handlers - REST API
    void handleAPIStatus();
    void handleAPIScan();
    void handleAPIConfigure();
    void handleAPIConnect();
    void handleAPICredentials();
    void handleAPIReset();
    void handleAPIHealth();
    void handleAPICredentialsDelete();
    
    // Utility handlers
    void handleNotFound();
    
    // Helper methods
    void sendJSON(int statusCode, const String& json);
    void sendHTML(int statusCode, const String& html);
    String urlDecode(const String& encoded);
    void enableCORS();
    void handleOptions();
};

#endif // CONFIGSERVER_H

