#include "ConfigServer.h"
#include "RunLoop.h"
#include "CustomPageHandler.h"
#include <esp_log.h>
#include <ArduinoJson.h>

static const char *TAG = EWIFI_TAG_CONFIG_SERVER;

ConfigServer::ConfigServer(WiFiManager& wifiManager, Storage& storage) 
    : deviceName("EasyWiFi"),
      server(EWIFI_WEB_SERVER_PORT), 
      wifiManager(wifiManager), 
      storage(storage),
      webPages(),
      enabled(false),
      runLoop(nullptr) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
}

void ConfigServer::setRunLoop(RunLoop* rl) {
    runLoop = rl;
}

void ConfigServer::setDeviceName(const String& name) {
    deviceName = name;
    webPages.setDeviceName(name);
}

void ConfigServer::on(const String& uri, HTTPMethod method, RouteHandler handler) {
    server.on(uri.c_str(), method, handler);
    ESP_LOGI(TAG, "Registered custom route: %s %s", 
             method == HTTP_GET ? "GET" : method == HTTP_POST ? "POST" : "?", 
             uri.c_str());
}

void ConfigServer::registerCustomHandler(CustomPageHandler* handler) {
    if (!handler) {
        ESP_LOGW(TAG, "Cannot register null custom handler");
        return;
    }
    
    ESP_LOGI(TAG, "Registering custom page handler...");
    
    // Provide access to EasyWiFi components
    handler->wifiManager = &wifiManager;
    handler->storage = &storage;
    handler->webPages = &webPages;
    
    // Let handler register its routes
    handler->registerRoutes();
    
    ESP_LOGI(TAG, "Custom page handler registered successfully");
}

void ConfigServer::enableCORS() {
    // Enable CORS for all API endpoints
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void ConfigServer::setup() {
    ESP_LOGI(TAG, "Setting up web server on port %d", EWIFI_WEB_SERVER_PORT);
    
    // Web UI routes
    server.on(EWIFI_ENDPOINT_ROOT, HTTP_GET, std::bind(&ConfigServer::handleRoot, this));
    server.on(EWIFI_ENDPOINT_SCAN, HTTP_GET, std::bind(&ConfigServer::handleScan, this));
    server.on(EWIFI_ENDPOINT_CONFIGURE, HTTP_GET, std::bind(&ConfigServer::handleConfigure, this));
    server.on(EWIFI_ENDPOINT_SAVE, HTTP_POST, std::bind(&ConfigServer::handleSave, this));
    server.on(EWIFI_ENDPOINT_STATUS, HTTP_GET, std::bind(&ConfigServer::handleStatus, this));
    server.on(EWIFI_ENDPOINT_RESET, HTTP_GET, std::bind(&ConfigServer::handleReset, this));
    server.on(EWIFI_ENDPOINT_CREDENTIALS, HTTP_GET, std::bind(&ConfigServer::handleCredentials, this));
    
    // REST API routes
    server.on(EWIFI_ENDPOINT_API_STATUS, HTTP_GET, std::bind(&ConfigServer::handleAPIStatus, this));
    server.on(EWIFI_ENDPOINT_API_SCAN, HTTP_GET, std::bind(&ConfigServer::handleAPIScan, this));
    server.on(EWIFI_ENDPOINT_API_CONFIGURE, HTTP_POST, std::bind(&ConfigServer::handleAPIConfigure, this));
    server.on(EWIFI_ENDPOINT_API_CONNECT, HTTP_POST, std::bind(&ConfigServer::handleAPIConnect, this));
    server.on(EWIFI_ENDPOINT_API_CREDENTIALS, HTTP_DELETE, std::bind(&ConfigServer::handleAPICredentials, this));
    server.on(EWIFI_ENDPOINT_API_CREDENTIALS, HTTP_GET, std::bind(&ConfigServer::handleAPICredentialsDelete, this));
    server.on(EWIFI_ENDPOINT_API_RESET, HTTP_GET, std::bind(&ConfigServer::handleAPIReset, this));
    server.on(EWIFI_ENDPOINT_API_HEALTH, HTTP_GET, std::bind(&ConfigServer::handleAPIHealth, this));
    
    // CORS preflight handler
    server.on(EWIFI_ENDPOINT_API_STATUS, HTTP_OPTIONS, std::bind(&ConfigServer::handleOptions, this));
    server.on(EWIFI_ENDPOINT_API_SCAN, HTTP_OPTIONS, std::bind(&ConfigServer::handleOptions, this));
    server.on(EWIFI_ENDPOINT_API_CONFIGURE, HTTP_OPTIONS, std::bind(&ConfigServer::handleOptions, this));
    server.on(EWIFI_ENDPOINT_API_CONNECT, HTTP_OPTIONS, std::bind(&ConfigServer::handleOptions, this));
    server.on(EWIFI_ENDPOINT_API_CREDENTIALS, HTTP_OPTIONS, std::bind(&ConfigServer::handleOptions, this));
    server.on(EWIFI_ENDPOINT_API_RESET, HTTP_OPTIONS, std::bind(&ConfigServer::handleOptions, this));
    
    // 404 handler
    server.onNotFound(std::bind(&ConfigServer::handleNotFound, this));
    
    server.begin();
    enabled = true;
    
    ESP_LOGI(TAG, "Web server started");
}

void ConfigServer::loop() {
    if (enabled) {
        server.handleClient();
    }
}

void ConfigServer::enable() {
    if (!enabled) {
        ESP_LOGI(TAG, "Enabling web server");
        server.begin();
        enabled = true;
    }
}

void ConfigServer::disable() {
    if (enabled) {
        ESP_LOGI(TAG, "Disabling web server");
        server.stop();
        enabled = false;
    }
}

bool ConfigServer::isEnabled() {
    return enabled;
}

// ==================== Web UI Handlers ====================

void ConfigServer::handleRoot() {
    ESP_LOGD(TAG, "Request: GET %s", EWIFI_ENDPOINT_ROOT);
    String html = webPages.generateHomePage(wifiManager, storage);
    sendHTML(EWIFI_HTTP_STATUS_OK, html);
}

void ConfigServer::handleScan() {
    ESP_LOGI(TAG, "Scanning WiFi networks...");
    
    std::vector<WiFiNetwork> networks;
    bool success = wifiManager.scanNetworks(networks);
    
    if (success) {
        ESP_LOGI(TAG, "Scan complete: found %d networks", networks.size());
    }
    
    String html = webPages.generateScanPage(networks, success);
    sendHTML(EWIFI_HTTP_STATUS_OK, html);
}

void ConfigServer::handleConfigure() {
    ESP_LOGD(TAG, "Request: GET %s", EWIFI_ENDPOINT_CONFIGURE);
    
    String ssid = server.arg("ssid");
    String html = webPages.generateConfigPage(ssid);
    sendHTML(EWIFI_HTTP_STATUS_OK, html);
}

void ConfigServer::handleSave() {
    ESP_LOGI(TAG, "Request: POST %s", EWIFI_ENDPOINT_SAVE);
    
    String ssid = urlDecode(server.arg("ssid"));
    String password = urlDecode(server.arg("password"));
    
    ESP_LOGI(TAG, "Saving credentials for SSID: %s", ssid.c_str());
    
    if (ssid.length() == 0) {
        String html = webPages.generateErrorPage("SSID cannot be empty");
        sendHTML(EWIFI_HTTP_STATUS_BAD_REQUEST, html);
        return;
    }
    
    // Save credentials
    bool saved = storage.saveCredentials(ssid, password);
    
    if (!saved) {
        String html = webPages.generateErrorPage("Failed to save credentials");
        sendHTML(EWIFI_HTTP_STATUS_SERVER_ERROR, html);
        return;
    }
    
    // Request RunLoop to attempt connection with new credentials
    if (runLoop != nullptr) {
        runLoop->requestConnectionAttempt();
    }
    
    // Redirect to status page
    server.sendHeader("Location", EWIFI_ENDPOINT_STATUS);
    server.send(EWIFI_HTTP_STATUS_REDIRECT, "text/plain", "Redirecting...");
}

void ConfigServer::handleStatus() {
    ESP_LOGD(TAG, "Request: GET %s", EWIFI_ENDPOINT_STATUS);
    
    String html = webPages.generateStatusPage(wifiManager);
    sendHTML(EWIFI_HTTP_STATUS_OK, html);
}

void ConfigServer::handleReset() {
    ESP_LOGI(TAG, "Request: GET %s", EWIFI_ENDPOINT_RESET);
    
    ESP_LOGW(TAG, "Factory reset requested");
    storage.clearCredentials();
    
    String html = webPages.generateResetPage();
    sendHTML(EWIFI_HTTP_STATUS_OK, html);
    
    // Reboot after 2 seconds (non-blocking wait to allow response to send)
    unsigned long rebootTime = millis();
    while (millis() - rebootTime < 2000) {
        yield();
    }
    ESP.restart();
}

// ==================== REST API Handlers ====================

void ConfigServer::handleAPIStatus() {
    ESP_LOGD(TAG, "API: GET %s", EWIFI_ENDPOINT_API_STATUS);
    
    enableCORS();
    
    String json = "{";
    json += "\"api_version\":\"1.0\",";
    json += "\"device\":{";
    json += "\"type\":\"ESP32-C3\",";
    json += "\"firmware\":\"EasyWiFi v0.1.0\",";
    json += "\"free_heap\":" + String(esp_get_free_heap_size()) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "},";
    json += "\"wifi\":{";
    json += "\"status\":\"" + wifiManager.getStatus() + "\",";
    json += "\"connected\":" + String(wifiManager.isConnected() ? "true" : "false") + ",";
    json += "\"ap_mode\":" + String(wifiManager.isAPMode() ? "true" : "false") + ",";
    json += "\"configured\":" + String(storage.isConfigured() ? "true" : "false") + ",";
    json += "\"connection_failed\":" + String("false") + ",";
    json += "\"ip\":\"" + wifiManager.getLocalIP() + "\"";
    
    if (wifiManager.isConnected()) {
        json += ",\"ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"signal_strength\":\"" + String(getSignalStrength(WiFi.RSSI())) + "\",";
        json += "\"mac\":\"" + WiFi.macAddress() + "\"";
        
        if (wifiManager.getMDNSHostname().length() > 0) {
            json += ",\"mdns\":\"" + wifiManager.getMDNSHostname() + ".local\"";
        }
    }
    
    if (wifiManager.isAPMode()) {
        json += ",\"ap_ssid\":\"" + wifiManager.getAPSSID() + "\",";
        json += "\"ap_ip\":\"" + wifiManager.getLocalIP() + "\"";
    }
    
    json += "},";
    json += "\"storage\":{";
    json += "\"configured\":" + String(storage.isConfigured() ? "true" : "false") + ",";
    json += "\"credential_count\":" + String(storage.getCredentialCount());
    json += "}";
    json += "}";
    
    sendJSON(EWIFI_HTTP_STATUS_OK, json);
}

void ConfigServer::handleAPIScan() {
    ESP_LOGI(TAG, "API: Scanning networks");
    
    enableCORS();
    
    std::vector<WiFiNetwork> networks;
    bool success = wifiManager.scanNetworks(networks);
    
    if (!success) {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"WiFi scan failed\",";
        errorJson += "\"message\":\"Unable to scan for WiFi networks. Please try again.\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_SERVER_ERROR, errorJson);
        return;
    }
    
    String json = "{";
    json += "\"success\":true,";
    json += "\"count\":" + String(networks.size()) + ",";
    json += "\"networks\":[";
    
    for (size_t i = 0; i < networks.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + networks[i].ssid + "\",";
        json += "\"rssi\":" + String(networks[i].rssi) + ",";
        json += "\"encryption\":\"" + networks[i].encryptionName + "\",";
        json += "\"open\":" + String(networks[i].encryptionType == WIFI_AUTH_OPEN ? "true" : "false") + ",";
        json += "\"signal_strength\":\"" + networks[i].signalStrength + "\",";
        json += "\"signal_bars\":" + String(networks[i].signalBars);
        json += "}";
    }
    
    json += "]";
    json += "}";
    
    sendJSON(EWIFI_HTTP_STATUS_OK, json);
}

void ConfigServer::handleAPIConfigure() {
    ESP_LOGI(TAG, "Request: POST %s", EWIFI_ENDPOINT_API_CONFIGURE);
    
    enableCORS();
    
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    // Validate SSID
    if (ssid.length() == 0) {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"validation_error\",";
        errorJson += "\"message\":\"SSID is required\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_BAD_REQUEST, errorJson);
        return;
    }
    
    if (ssid.length() > 32) {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"validation_error\",";
        errorJson += "\"message\":\"SSID must be 32 characters or less\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_BAD_REQUEST, errorJson);
        return;
    }
    
    // Validate password
    if (password.length() > 0 && password.length() < 8) {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"validation_error\",";
        errorJson += "\"message\":\"Password must be at least 8 characters for WPA/WPA2\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_BAD_REQUEST, errorJson);
        return;
    }
    
    if (password.length() > 63) {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"validation_error\",";
        errorJson += "\"message\":\"Password must be 63 characters or less\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_BAD_REQUEST, errorJson);
        return;
    }
    
    bool saved = storage.saveCredentials(ssid, password);
    
    if (saved) {
        String successJson = "{";
        successJson += "\"success\":true,";
        successJson += "\"message\":\"WiFi credentials saved successfully\",";
        successJson += "\"ssid\":\"" + ssid + "\",";
        successJson += "\"next_step\":\"Device will attempt to connect\"";
        successJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_OK, successJson);
    } else {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"storage_error\",";
        errorJson += "\"message\":\"Failed to save credentials to storage\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_SERVER_ERROR, errorJson);
    }
}

void ConfigServer::handleAPIConnect() {
    ESP_LOGI(TAG, "Request: POST %s", EWIFI_ENDPOINT_API_CONNECT);
    
    enableCORS();
    
    if (!storage.isConfigured()) {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"no_credentials\",";
        errorJson += "\"message\":\"No WiFi credentials configured\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_BAD_REQUEST, errorJson);
        return;
    }
    
    String json = "{";
    json += "\"success\":true,";
    json += "\"message\":\"Connection attempt will be initiated\",";
    json += "\"note\":\"Device will attempt to connect to configured network\"";
    json += "}";
    
    sendJSON(EWIFI_HTTP_STATUS_OK, json);
}

void ConfigServer::handleAPICredentials() {
    ESP_LOGI(TAG, "Request: DELETE %s", EWIFI_ENDPOINT_API_CREDENTIALS);
    
    enableCORS();
    
    int count = storage.getCredentialCount();
    bool cleared = storage.clearCredentials();
    
    if (cleared) {
        String json = "{";
        json += "\"success\":true,";
        json += "\"message\":\"All WiFi credentials cleared\",";
        json += "\"cleared_count\":" + String(count);
        json += "}";
        sendJSON(EWIFI_HTTP_STATUS_OK, json);
    } else {
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"storage_error\",";
        errorJson += "\"message\":\"Failed to clear credentials\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_SERVER_ERROR, errorJson);
    }
}

void ConfigServer::handleAPIReset() {
    ESP_LOGI(TAG, "Request: GET %s", EWIFI_ENDPOINT_API_RESET);
    
    enableCORS();
    
    int count = storage.getCredentialCount();
    storage.clearCredentials();
    
    String json = "{";
    json += "\"success\":true,";
    json += "\"message\":\"Factory reset initiated\",";
    json += "\"cleared_count\":" + String(count) + ",";
    json += "\"action\":\"Device will reboot in 2 seconds\"";
    json += "}";
    
    sendJSON(EWIFI_HTTP_STATUS_OK, json);
    
    // Reboot after 2 seconds (non-blocking wait to allow response to send)
    unsigned long rebootTime = millis();
    while (millis() - rebootTime < 2000) {
        yield();
    }
    ESP.restart();
}

void ConfigServer::handleOptions() {
    ESP_LOGD(TAG, "CORS preflight request");
    enableCORS();
    server.send(EWIFI_HTTP_STATUS_NO_CONTENT);
}

void ConfigServer::handleNotFound() {
    String uri = server.uri();
    ESP_LOGV(TAG, "Request for: %s (from %s)", uri.c_str(), server.hostHeader().c_str());
    
    // Check if this is an API endpoint request
    if (uri.startsWith("/api/")) {
        enableCORS();
        String errorJson = "{";
        errorJson += "\"success\":false,";
        errorJson += "\"error\":\"not_found\",";
        errorJson += "\"message\":\"API endpoint not found\",";
        errorJson += "\"path\":\"" + uri + "\"";
        errorJson += "}";
        sendJSON(EWIFI_HTTP_STATUS_NOT_FOUND, errorJson);
        return;
    }
    
    // Captive Portal Detection:
    // When devices connect to WiFi, they check for internet by requesting known URLs.
    // We need to redirect to our config page to trigger the captive portal popup.
    
    // Check if this looks like a captive portal detection request
    if (isCaptivePortalDetection(uri)) {
        ESP_LOGI(TAG, "Captive portal detection: redirecting %s to /wifi", uri.c_str());
        server.sendHeader("Location", "/wifi", true);
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "0");
        server.send(302, "text/plain", "");
    } else {
        // Show helpful 404 page for genuine navigation errors
        ESP_LOGI(TAG, "Showing 404 page for: %s", uri.c_str());
        String html = webPages.generate404Page(uri);
        server.send(404, "text/html", html);
    }
}

// ==================== Helper Methods ====================

void ConfigServer::sendJSON(int statusCode, const String& json) {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(statusCode, "application/json; charset=utf-8", json);
}

void ConfigServer::sendHTML(int statusCode, const String& html) {
    server.send(statusCode, "text/html", html);
}

String ConfigServer::urlDecode(const String& encoded) {
    String decoded = encoded;
    decoded.replace("+", " ");
    decoded.replace("%20", " ");
    decoded.replace("%21", "!");
    decoded.replace("%22", "\"");
    decoded.replace("%23", "#");
    decoded.replace("%24", "$");
    decoded.replace("%25", "%");
    decoded.replace("%26", "&");
    decoded.replace("%27", "'");
    decoded.replace("%28", "(");
    decoded.replace("%29", ")");
    decoded.replace("%2A", "*");
    decoded.replace("%2B", "+");
    decoded.replace("%2C", ",");
    decoded.replace("%2D", "-");
    decoded.replace("%2E", ".");
    decoded.replace("%2F", "/");
    return decoded;
}

void ConfigServer::handleAPIHealth() {
    ESP_LOGD(TAG, "GET /api/health");
    enableCORS();
    
    const ErrorStats& wifiStats = wifiManager.getErrorStats();
    const ErrorStats& storageStats = storage.getErrorStats();
    
    // Create JSON response using ArduinoJson
    JsonDocument doc;
    doc["healthy"] = wifiManager.isHealthy() && storage.isHealthy();
    doc["uptime"] = millis() / 1000;
    
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["totalErrors"] = wifiStats.totalErrors;
    wifi["connectionErrors"] = wifiStats.connectionErrors;
    wifi["recoveredErrors"] = wifiStats.recoveredErrors;
    wifi["consecutiveErrors"] = wifiStats.consecutiveErrors;
    wifi["lastErrorTime"] = wifiStats.lastErrorTime;
    wifi["healthy"] = wifiManager.isHealthy();
    
    JsonObject storageObj = doc["storage"].to<JsonObject>();
    storageObj["totalErrors"] = storageStats.totalErrors;
    storageObj["storageErrors"] = storageStats.storageErrors;
    storageObj["consecutiveErrors"] = storageStats.consecutiveErrors;
    storageObj["healthy"] = storage.isHealthy();
    
    JsonObject memory = doc["memory"].to<JsonObject>();
    memory["free"] = esp_get_free_heap_size();
    memory["minimum"] = esp_get_minimum_free_heap_size();
    
    String response;
    serializeJson(doc, response);
    sendJSON(EWIFI_HTTP_STATUS_OK, response);
}

void ConfigServer::handleCredentials() {
    ESP_LOGI(TAG, "GET /credentials");
    
    String currentSSID = WiFi.SSID();
    String html = webPages.generateCredentialsPage(storage, currentSSID);
    sendHTML(EWIFI_HTTP_STATUS_OK, html);
}

void ConfigServer::handleAPICredentialsDelete() {
    ESP_LOGD(TAG, "GET /api/credentials (delete via query)");
    
    if (!server.hasArg("ssid") || !server.hasArg("action")) {
        sendHTML(EWIFI_HTTP_STATUS_BAD_REQUEST, webPages.generateErrorPage("Missing parameters"));
        return;
    }
    
    String ssid = server.arg("ssid");
    String action = server.arg("action");
    
    if (action == "delete") {
        // Don't allow deleting the currently connected network if it's the only one
        std::vector<String> ssidList;
        storage.getCredentialsList(ssidList);
        
        if (ssid == WiFi.SSID() && ssidList.size() == 1) {
            sendHTML(EWIFI_HTTP_STATUS_BAD_REQUEST, 
                    webPages.generateErrorPage("Cannot delete the only network while connected to it. Add another network first."));
            return;
        }
        
        if (storage.deleteCredential(ssid)) {
            ESP_LOGI(TAG, "Deleted credential: %s", ssid.c_str());
            
            // If we just deleted the currently connected network, disconnect and reconnect
            if (ssid == WiFi.SSID()) {
                ESP_LOGI(TAG, "Deleted the currently connected network - triggering reconnection");
                WiFi.disconnect();
                
                // Request RunLoop to attempt connection with remaining credentials
                if (runLoop != nullptr) {
                    runLoop->requestConnectionAttempt();
                }
            }
            
            // Redirect back to credentials page
            server.sendHeader("Location", "/credentials", true);
            server.send(EWIFI_HTTP_STATUS_REDIRECT, "text/plain", "");
        } else {
            sendHTML(EWIFI_HTTP_STATUS_NOT_FOUND, webPages.generateErrorPage("Network not found"));
        }
    } else if (action == "join") {
        // Verify the credential exists
        std::vector<String> ssidList;
        storage.getCredentialsList(ssidList);
        
        bool found = false;
        for (const auto& storedSSID : ssidList) {
            if (storedSSID == ssid) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            sendHTML(EWIFI_HTTP_STATUS_NOT_FOUND, webPages.generateErrorPage("Network not found"));
            return;
        }
        
        // Don't try to join if already connected
        if (ssid == WiFi.SSID() && WiFi.status() == WL_CONNECTED) {
            sendHTML(EWIFI_HTTP_STATUS_BAD_REQUEST, 
                    webPages.generateErrorPage("Already connected to " + ssid));
            return;
        }
        
        ESP_LOGI(TAG, "Manual join requested for: %s", ssid.c_str());
        
        // Move this network to priority 1 for immediate connection
        storage.moveCredentialToFirst(ssid);
        
        // Disconnect from current network
        if (WiFi.status() == WL_CONNECTED) {
            ESP_LOGI(TAG, "Disconnecting from current network: %s", WiFi.SSID().c_str());
            WiFi.disconnect();
        }
        
        // Request RunLoop to attempt connection
        if (runLoop != nullptr) {
            runLoop->requestConnectionAttempt();
        }
        
        // Redirect to status page to show connection progress
        server.sendHeader("Location", EWIFI_ENDPOINT_STATUS, true);
        server.send(EWIFI_HTTP_STATUS_REDIRECT, "text/plain", "");
    } else {
        sendHTML(EWIFI_HTTP_STATUS_BAD_REQUEST, webPages.generateErrorPage("Invalid action"));
    }
}
