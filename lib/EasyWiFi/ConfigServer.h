#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

#if defined(ESP32)
#include "Arduino.h"
#include <WebServer.h>
#include "WiFiManager.h"
#include "Storage.h"
#include "WebPages.h"
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// Forward declaration to avoid circular dependency
class RunLoop;

class ConfigServer {
public:
    ConfigServer(WiFiManager& wifiManager, Storage& storage);
    
    void setup();
    void loop();
    void enable();
    void disable();
    
    bool isEnabled();
    void setRunLoop(RunLoop* rl);

private:
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
    
    // Route handlers - REST API
    void handleAPIStatus();
    void handleAPIScan();
    void handleAPIConfigure();
    void handleAPIConnect();
    void handleAPICredentials();
    void handleAPIReset();
    
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

