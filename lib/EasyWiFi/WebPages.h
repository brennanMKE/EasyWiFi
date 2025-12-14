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
    
    // Generate HTML pages
    String generateHomePage(WiFiManager& wifiManager, Storage& storage);
    String generateScanPage(const std::vector<WiFiNetwork>& networks, bool success);
    String generateConfigPage(const String& ssid);
    String generateStatusPage(WiFiManager& wifiManager);
    String generateSuccessPage();
    String generateErrorPage(const String& errorMessage);
    String generateResetPage();
    String generateCredentialsPage(Storage& storage, const String& currentSSID);

private:
    // Common HTML elements
    String getHTMLHeader(const String& title);
    String getHTMLFooter();
    String getCSS();
    String getSignalBarsHTML(int bars);
    String getLockIconHTML();
};

#endif // WEBPAGES_H
