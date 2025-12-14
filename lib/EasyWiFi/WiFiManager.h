#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#if defined(ESP32)
#include "Arduino.h"
#include <WiFi.h>
// NOTE: WiFiMulti.h is intentionally NOT included!
// See Fix.md: WiFiMulti interferes with ESP32-C3 + eero compatibility settings.
// We use direct WiFi.begin() instead to preserve power/protocol settings.
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <vector>
#include "Storage.h"
#include "ErrorHandler.h"
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// Structure to hold WiFi network information
struct WiFiNetwork {
    String ssid;
    int rssi;              // Signal strength in dBm
    int channel;           // WiFi channel (1-14 for 2.4GHz, 36+ for 5GHz)
    wifi_auth_mode_t encryptionType;
    String encryptionName;
    int signalBars;        // 0-4 bars
    String signalStrength; // "Excellent", "Good", etc.
    
    WiFiNetwork() : ssid(""), rssi(0), channel(0), encryptionType(WIFI_AUTH_OPEN), 
                    encryptionName("Open"), signalBars(0), signalStrength("") {}
};

class WiFiManager {
public:
    WiFiManager();
    
    // Connect to stored networks from Storage
    bool connectToStoredNetworks(Storage& storage);
    
    WiFiError connectToStoredNetworksEx(Storage& storage, ErrorContext* outError = nullptr);
    WiFiError startAccessPointEx(const String& deviceName = "EasyWiFi", ErrorContext* outError = nullptr);
    
    // Start Access Point mode
    bool startAccessPoint(const String& deviceName = "EasyWiFi");
    
    // Stop Access Point mode
    void stopAccessPoint();
    
    // Error monitoring
    const ErrorStats& getErrorStats() const { return errorHandler.getStats(); }
    bool isHealthy() const { return errorHandler.isHealthy(); }
    
    // Scan for available WiFi networks
    bool scanNetworks(std::vector<WiFiNetwork>& results);
    
    // Get current WiFi status as string
    String getStatus();
    
    // Check if connected to WiFi
    bool isConnected();
    
    // Check if in AP mode
    bool isAPMode();
    
    // Get AP SSID
    String getAPSSID();
    
    // Get local IP address
    String getLocalIP();
    
    // mDNS support
    bool startMDNS(const String& deviceName = "EasyWiFi");
    void announceMDNS();
    String getMDNSHostname();
    
    // Captive portal support
    bool startCaptivePortal();
    void stopCaptivePortal();
    void processCaptivePortal();
    bool isCaptivePortalActive();

private:
    // ⚠️ WARNING: DO NOT add WiFiMulti member variable!
    // WiFiMulti interferes with eero compatibility settings (see Fix.md)
    // Always use direct WiFi.begin() to preserve power/protocol settings
    
    String apSSID;
    String apPassword;
    bool apActive;
    String mdnsHostname;
    bool mdnsActive;
    DNSServer* dnsServer;
    bool captivePortalActive;
    ErrorHandler errorHandler;
    
    // Get encryption type as human-readable string
    const char* getEncryptionTypeName(wifi_auth_mode_t encryptionType);
    
    // Get WiFi status as human-readable string
    const char* getWiFiStatusName(wl_status_t status);
    
    // Get WiFi disconnect reason as human-readable string
    const char* getDisconnectReasonName(uint8_t reason);
    
    // Generate unique AP SSID from device MAC with custom device name
    String generateAPSSID(const String& deviceName = "EasyWiFi");
    
    // Log detailed WiFi diagnostics
    void logWiFiDiagnostics();
};

#endif // WIFIMANAGER_H
