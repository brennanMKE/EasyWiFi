#ifndef STORAGE_H
#define STORAGE_H

#if defined(ESP32)
#include "Arduino.h"
#include <Preferences.h>
#include <vector>
#include "ErrorHandler.h"
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
    
    StorageError beginEx(ErrorContext* outError = nullptr);
    StorageError saveCredentialsEx(const String& ssid, const String& password, ErrorContext* outError = nullptr);
    StorageError loadCredentialsEx(std::vector<WiFiCredential>& credentials, ErrorContext* outError = nullptr);
    
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
    
    // Credential management
    bool deleteCredential(const String& ssid);
    bool getCredentialsList(std::vector<String>& ssidList);
    int getCredentialIndex(const String& ssid);
    bool moveCredentialToFirst(const String& ssid);
    
    // Error monitoring
    const ErrorStats& getErrorStats() const { return errorHandler.getStats(); }
    bool isHealthy() const { return errorHandler.isHealthy(); }

private:
    Preferences preferences;
    ErrorHandler errorHandler;
    
    // Generate key for SSID at index
    String getSSIDKey(int index);
    
    // Generate key for password at index
    String getPasswordKey(int index);
};

#endif // STORAGE_H
