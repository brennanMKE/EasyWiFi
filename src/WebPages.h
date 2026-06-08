#ifndef WEBPAGES_H
#define WEBPAGES_H

#if defined(ESP32)
#include "Arduino.h"
#include <vector>
#include "WiFiManager.h"
#include "Storage.h"
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

class WebPages {
public:
    WebPages();
    
    // Set device name for display in pages
    void setDeviceName(const String& name) { deviceName = name; }
    
    // Generate HTML pages
    String generateHomePage(WiFiManager& wifiManager, Storage& storage);
    String generateScanPage(const std::vector<WiFiNetwork>& networks, bool success);
    String generateConfigPage(const String& ssid);
    String generateStatusPage(WiFiManager& wifiManager);
    String generateSuccessPage();
    String generateErrorPage(const String& errorMessage);
    String generateResetPage();
    String generateCredentialsPage(Storage& storage, const String& currentSSID);
    String generate404Page(const String& requestedPath);

    // Helper methods for custom pages (shared styling)
    String getHTMLHeader(const String& title);
    String getHTMLFooter();
    String getCSS();

private:
    String deviceName;
    String getSignalBarsHTML(int bars);
    String getLockIconHTML();
};

#endif // WEBPAGES_H
