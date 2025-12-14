#ifndef STORAGE_H
#define STORAGE_H

#if defined(ESP32)
#include "Arduino.h"
#include <Preferences.h>
#include <vector>
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// Structure to hold WiFi credentials
struct WiFiCredential {
    String ssid;
    String password;
    
    WiFiCredential() : ssid(""), password("") {}
    WiFiCredential(const String& s, const String& p) : ssid(s), password(p) {}
};

class Storage {
public:
    Storage();
    
    // Initialize NVS storage
    bool begin();
    
    // Save WiFi credentials (adds to list or updates existing)
    bool saveCredentials(const String& ssid, const String& password);
    
    // Load all stored credentials
    bool loadCredentials(std::vector<WiFiCredential>& credentials);
    
    // Clear all stored credentials
    bool clearCredentials();
    
    // Check if any credentials are configured
    bool isConfigured();
    
    // Get count of stored credentials
    int getCredentialCount();

private:
    Preferences preferences;
    
    // Generate key for SSID at index
    String getSSIDKey(int index);
    
    // Generate key for password at index
    String getPasswordKey(int index);
};

#endif // STORAGE_H

