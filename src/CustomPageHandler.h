#ifndef CUSTOMPAGEHANDLER_H
#define CUSTOMPAGEHANDLER_H

#if defined(ESP32)
#include "Arduino.h"
#include <WebServer.h>
#include <functional>
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// Forward declarations to avoid circular dependencies
class WiFiManager;
class Storage;
class WebPages;
class ConfigServer;

// Callback type for route handlers
typedef std::function<void()> RouteHandler;

/**
 * @brief Base class for custom page handlers
 * 
 * This class provides a clean interface for adding custom web pages to your device
 * while keeping that code separate from the EasyWiFi library.
 * 
 * Benefits:
 * - Access to WiFiManager, Storage, and WebPages components
 * - Shared HTML/CSS styling via WebPages helpers
 * - Clean separation between WiFi config and custom functionality
 * 
 * Usage:
 *   1. Create a subclass that inherits from CustomPageHandler
 *   2. Override registerRoutes() to register your custom URLs
 *   3. Implement handler methods for your routes
 *   4. Register your handler with ConfigServer in main.cpp
 * 
 * Example:
 *   class MyPages : public CustomPageHandler {
 *   public:
 *       MyPages(ConfigServer& cs) : configServer(cs) {}
 *       
 *       void registerRoutes() override {
 *           WebServer& server = configServer.getServer();
 *           server.on("/custom", HTTP_GET, 
 *               std::bind(&MyPages::handleCustomPage, this));
 *       }
 *       
 *   private:
 *       ConfigServer& configServer;
 *       void handleCustomPage() {
 *           String html = webPages->getHTMLHeader("Custom Page");
 *           html += "<body><h1>Custom Content</h1>";
 *           html += webPages->getHTMLFooter();
 *           configServer.getServer().send(200, "text/html", html);
 *       }
 *   };
 */
class CustomPageHandler {
public:
    virtual ~CustomPageHandler() {}
    
    /**
     * @brief Override this method to register your custom routes
     * 
     * This method is called during setup after EasyWiFi has registered
     * its routes. Use configServer.getServer().on() to add your routes.
     * 
     * Example:
     *   void registerRoutes() override {
     *       WebServer& server = configServer.getServer();
     *       server.on("/mypage", HTTP_GET, 
     *           std::bind(&MyClass::handleMyPage, this));
     *   }
     */
    virtual void registerRoutes() = 0;
    
protected:
    /**
     * @brief Access to EasyWiFi components (set by ConfigServer)
     * 
     * These pointers are set by ConfigServer.registerCustomHandler()
     * and provide access to the WiFi system components.
     * 
     * - wifiManager: Check connection status, signal strength, etc.
     * - storage: Read/write persistent data, access stored credentials
     * - webPages: Use HTML helpers for consistent styling
     */
    WiFiManager* wifiManager = nullptr;
    Storage* storage = nullptr;
    WebPages* webPages = nullptr;
    
    // ConfigServer can set the component pointers
    friend class ConfigServer;
};

#endif // CUSTOMPAGEHANDLER_H

