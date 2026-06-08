#include "LanternsPages.h"
#include <ArduinoJson.h>

// NOTE: This is an EXAMPLE implementation
// Customize this file for your specific device functionality

LanternsPages::LanternsPages(ConfigServer& cs) : configServer(cs) {
}

void LanternsPages::registerRoutes() {
    // Get reference to web server
    WebServer& server = configServer.getServer();
    
    // Register custom web UI pages
    // Root "/" is now the main Lanterns home page (WiFi config moved to /wifi)
    server.on("/", HTTP_GET, 
        std::bind(&LanternsPages::handleLanternsHome, this));
    server.on("/control", HTTP_GET, 
        std::bind(&LanternsPages::handleLanternsControl, this));
    server.on("/settings", HTTP_GET, 
        std::bind(&LanternsPages::handleLanternsSettings, this));
    
    // Register custom API endpoints
    server.on("/api/lanterns/status", HTTP_GET, 
        std::bind(&LanternsPages::handleAPILanternsStatus, this));
    server.on("/api/lanterns/control", HTTP_POST, 
        std::bind(&LanternsPages::handleAPILanternsControl, this));
}

// ============================================================================
// Web UI Handlers
// ============================================================================

void LanternsPages::handleLanternsHome() {
    // Use WebPages helpers for consistent styling
    String html = webPages->getHTMLHeader("Lanterns Control");
    html += "<body>";
    
    html += "<div class='card'>";
    html += "<h1>🏮 Lanterns</h1>";
    
    // Show WiFi status (access to wifiManager)
    if (wifiManager->isConnected()) {
        html += "<div class='status success'>";
        html += "<span style='font-size:24px;margin-right:8px;'>✓</span>";
        html += "<strong>Connected</strong> to " + WiFi.SSID();
        html += "</div>";
    } else {
        html += "<div class='status warning'>";
        html += "<strong>WiFi Disconnected</strong>";
        html += "</div>";
    }
    
    // Example: Get stored credentials count
    std::vector<WiFiCredential> credentials;
    storage->loadCredentials(credentials);
    
    html += "<div style='background:#f8f9fa;padding:16px;border-radius:8px;margin:16px 0;'>";
    html += "<p style='margin:8px 0;'><strong>System Status:</strong></p>";
    html += "<p style='margin:8px 0;'>WiFi Networks Configured: " + String(credentials.size()) + "</p>";
    html += "<p style='margin:8px 0;'>Signal Strength: " + String(WiFi.RSSI()) + " dBm</p>";
    html += "<p style='margin:8px 0;'>IP Address: " + WiFi.localIP().toString() + "</p>";
    html += "</div>";
    
    // Example: Custom lantern controls
    html += "<h2>Quick Controls</h2>";
    html += "<div class='button-group'>";
    html += "<a href='/control' class='button primary'>🎨 Control Lights</a>";
    html += "<a href='/settings' class='button'>⚙️ Settings</a>";
    html += "<a href='/wifi' class='button'>🌐 WiFi Setup</a>";
    html += "</div>";
    
    html += "</div>";
    html += webPages->getHTMLFooter();
    
    configServer.getServer().send(200, "text/html", html);
}

void LanternsPages::handleLanternsControl() {
    String html = webPages->getHTMLHeader("Lanterns Control");
    html += "<body>";
    
    html += "<div class='card'>";
    html += "<h1>🎨 Light Control</h1>";
    
    // TODO: Add your custom controls here
    // Examples:
    // - Color picker
    // - Brightness slider
    // - On/off toggle
    // - Pattern selection
    
    html += "<p>This is where your custom lantern controls would go.</p>";
    html += "<p>Add HTML forms, JavaScript, color pickers, sliders, etc.</p>";
    
    // Example control form
    html += "<form action='/api/lanterns/control' method='POST' style='margin:16px 0;'>";
    html += "<label style='display:block;margin:8px 0;'>Brightness:</label>";
    html += "<input type='range' name='brightness' min='0' max='255' value='128' ";
    html += "style='width:100%;'>";
    html += "<br><br>";
    html += "<button type='submit' class='button primary'>Apply</button>";
    html += "</form>";
    
    html += "<div style='margin-top:24px;'>";
    html += "<a href='/' class='button small'>← Back</a>";
    html += "</div>";
    
    html += "</div>";
    html += webPages->getHTMLFooter();
    
    configServer.getServer().send(200, "text/html", html);
}

void LanternsPages::handleLanternsSettings() {
    String html = webPages->getHTMLHeader("Lanterns Settings");
    html += "<body>";
    
    html += "<div class='card'>";
    html += "<h1>⚙️ Lantern Settings</h1>";
    
    // TODO: Add your custom settings here
    // Examples:
    // - Default brightness
    // - Auto-off timer
    // - Animation speed
    // - Device-specific configuration
    
    html += "<p>This is where your lantern settings would go.</p>";
    html += "<p>You can use Storage to save/load custom settings:</p>";
    
    html += "<pre style='background:#f8f9fa;padding:12px;border-radius:4px;overflow:auto;'>";
    html += "// In your code:\n";
    html += "Preferences prefs;\n";
    html += "prefs.begin(\"lanterns\", false);\n";
    html += "int brightness = prefs.getInt(\"brightness\", 128);\n";
    html += "prefs.putInt(\"brightness\", newValue);\n";
    html += "prefs.end();\n";
    html += "</pre>";
    
    html += "<div style='margin-top:24px;'>";
    html += "<a href='/' class='button small'>← Back</a>";
    html += "</div>";
    
    html += "</div>";
    html += webPages->getHTMLFooter();
    
    configServer.getServer().send(200, "text/html", html);
}

// ============================================================================
// REST API Handlers
// ============================================================================

void LanternsPages::handleAPILanternsStatus() {
    // Example API endpoint returning JSON
    StaticJsonDocument<256> doc;
    
    doc["success"] = true;
    doc["device"] = "Lanterns";
    doc["version"] = "0.1.0";
    
    // Example: Include WiFi status
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["connected"] = wifiManager->isConnected();
    wifi["ssid"] = wifiManager->isConnected() ? WiFi.SSID() : "";
    wifi["rssi"] = wifiManager->isConnected() ? WiFi.RSSI() : 0;
    
    // TODO: Add your custom status data here
    // Examples:
    doc["brightness"] = 128;  // Example value
    doc["power"] = true;      // Example value
    doc["mode"] = "auto";     // Example value
    
    String response;
    serializeJson(doc, response);
    
    configServer.getServer().sendHeader("Access-Control-Allow-Origin", "*");
    configServer.getServer().send(200, "application/json", response);
}

void LanternsPages::handleAPILanternsControl() {
    WebServer& server = configServer.getServer();
    
    // Example: Parse POST parameters
    if (!server.hasArg("brightness")) {
        server.send(400, "application/json", "{\"error\":\"Missing brightness parameter\"}");
        return;
    }
    
    int brightness = server.arg("brightness").toInt();
    
    // TODO: Implement your control logic here
    // Examples:
    // - Set LED brightness
    // - Change color
    // - Update animation pattern
    // - Save settings to storage
    
    // Example response
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["brightness"] = brightness;
    doc["message"] = "Brightness updated";
    
    String response;
    serializeJson(doc, response);
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
}

// ============================================================================
// Helper Methods
// ============================================================================

String LanternsPages::getLanternStatusHTML() {
    // Example helper method for generating status display
    String html = "<div style='background:#e8f5e9;padding:12px;border-radius:8px;'>";
    html += "<strong>Lantern Status:</strong> ";
    html += "ON";  // TODO: Get actual status
    html += "</div>";
    return html;
}

