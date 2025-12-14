#include "Storage.h"
#include <esp_log.h>

static const char *TAG = TAG_STORAGE;

Storage::Storage() : errorHandler(TAG_STORAGE) {
    // Constructor
}

StorageError Storage::beginEx(ErrorContext* outError) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    
    bool success = preferences.begin(NVS_NAMESPACE, false); // false = read/write mode
    if (success) {
        ESP_LOGI(TAG, "NVS storage initialized");
        errorHandler.recordRecovery();
        return StorageError::SUCCESS;
    } else {
        ESP_LOGE(TAG, "Failed to initialize NVS storage");
        errorHandler.recordError((uint32_t)StorageError::NVS_INIT_FAILED, "NVS initialization failed");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return StorageError::NVS_INIT_FAILED;
    }
}

bool Storage::begin() {
    StorageError error = beginEx(nullptr);
    return error == StorageError::SUCCESS;
}

StorageError Storage::saveCredentialsEx(const String& ssid, const String& password, ErrorContext* outError) {
    // Validate SSID
    if (ssid.length() == 0) {
        ESP_LOGE(TAG, "Cannot save credentials: SSID is empty");
        errorHandler.recordError((uint32_t)StorageError::INVALID_SSID, "SSID is empty");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return StorageError::INVALID_SSID;
    }
    
    if (ssid.length() > 32) {
        ESP_LOGE(TAG, "Cannot save credentials: SSID too long (%d chars, max 32)", ssid.length());
        errorHandler.recordError((uint32_t)StorageError::INVALID_SSID, "SSID too long");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return StorageError::INVALID_SSID;
    }
    
    // Validate password
    if (password.length() > 63) {
        ESP_LOGE(TAG, "Cannot save credentials: Password too long (%d chars, max 63)", password.length());
        errorHandler.recordError((uint32_t)StorageError::INVALID_PASSWORD, "Password too long");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return StorageError::INVALID_PASSWORD;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Saving WiFi Credentials:");
    ESP_LOGI(TAG, "  SSID: '%s' (length: %d)", ssid.c_str(), ssid.length());
    ESP_LOGI(TAG, "  Password length: %d chars", password.length());
    
    // Show password with masking for diagnostics
    String maskedPass = "";
    if (password.length() <= 4) {
        maskedPass = "****";
    } else if (password.length() > 4) {
        maskedPass = password.substring(0, 2) + 
                    String("********") + 
                    password.substring(password.length() - 2);
    }
    ESP_LOGI(TAG, "  Password hint: %s", maskedPass.c_str());
    ESP_LOGI(TAG, "========================================");
    
    // Load existing credentials
    std::vector<WiFiCredential> credentials;
    loadCredentials(credentials);
    
    // Check if SSID already exists (update password)
    bool found = false;
    for (size_t i = 0; i < credentials.size(); i++) {
        if (credentials[i].ssid == ssid) {
            credentials[i].password = password;
            found = true;
            ESP_LOGI(TAG, "Updating password for existing SSID: %s", ssid.c_str());
            break;
        }
    }
    
    // Add new credential if not found
    if (!found) {
        if (credentials.size() >= MAX_STORED_NETWORKS) {
            ESP_LOGW(TAG, "Maximum credentials reached, removing oldest");
            credentials.erase(credentials.begin()); // Remove oldest (first)
        }
        credentials.push_back(WiFiCredential(ssid, password));
        ESP_LOGI(TAG, "Adding new credentials for SSID: %s", ssid.c_str());
    }
    
    // Clear all credentials first
    clearCredentials();
    
    // Save all credentials
    for (size_t i = 0; i < credentials.size(); i++) {
        String ssidKey = getSSIDKey(i);
        String passKey = getPasswordKey(i);
        
        preferences.putString(ssidKey.c_str(), credentials[i].ssid);
        preferences.putString(passKey.c_str(), credentials[i].password);
        
        ESP_LOGV(TAG, "Saved credential %d: %s", i, credentials[i].ssid.c_str());
    }
    
    // Save count
    preferences.putInt(NVS_KEY_COUNT, credentials.size());
    preferences.putBool(NVS_KEY_CONFIGURED, true);
    
    ESP_LOGI(TAG, "✓ Saved %d credential(s) to NVS", credentials.size());
    errorHandler.recordRecovery();
    return StorageError::SUCCESS;
}

bool Storage::saveCredentials(const String& ssid, const String& password) {
    StorageError error = saveCredentialsEx(ssid, password, nullptr);
    return error == StorageError::SUCCESS;
}

StorageError Storage::loadCredentialsEx(std::vector<WiFiCredential>& credentials, ErrorContext* outError) {
    credentials.clear();
    
    int count = preferences.getInt(NVS_KEY_COUNT, 0);
    ESP_LOGI(TAG, "Loading %d credential(s) from NVS", count);
    
    if (count == 0) {
        errorHandler.recordError((uint32_t)StorageError::CREDENTIALS_NOT_FOUND, "No credentials found");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return StorageError::CREDENTIALS_NOT_FOUND;
    }
    
    for (int i = 0; i < count && i < MAX_STORED_NETWORKS; i++) {
        String ssidKey = getSSIDKey(i);
        String passKey = getPasswordKey(i);
        
        String ssid = preferences.getString(ssidKey.c_str(), "");
        String password = preferences.getString(passKey.c_str(), "");
        
        if (ssid.length() > 0) {
            credentials.push_back(WiFiCredential(ssid, password));
            ESP_LOGV(TAG, "Loaded credential %d: %s", i, ssid.c_str());
        }
    }
    
    ESP_LOGI(TAG, "Loaded %d credential(s)", credentials.size());
    
    if (credentials.size() > 0) {
        errorHandler.recordRecovery();
        return StorageError::SUCCESS;
    } else {
        errorHandler.recordError((uint32_t)StorageError::CREDENTIALS_NOT_FOUND, "No valid credentials found");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return StorageError::CREDENTIALS_NOT_FOUND;
    }
}

bool Storage::loadCredentials(std::vector<WiFiCredential>& credentials) {
    StorageError error = loadCredentialsEx(credentials, nullptr);
    return error == StorageError::SUCCESS;
}

bool Storage::clearCredentials() {
    ESP_LOGI(TAG, "Clearing all credentials from NVS");
    
    // Clear all keys
    preferences.clear();
    
    ESP_LOGI(TAG, "Credentials cleared");
    return true;
}

bool Storage::isConfigured() {
    bool configured = preferences.getBool(NVS_KEY_CONFIGURED, false);
    int count = preferences.getInt(NVS_KEY_COUNT, 0);
    
    bool result = configured && (count > 0);
    ESP_LOGV(TAG, "isConfigured: %s (configured=%d, count=%d)", 
             result ? "true" : "false", configured, count);
    
    return result;
}

int Storage::getCredentialCount() {
    return preferences.getInt(NVS_KEY_COUNT, 0);
}

String Storage::getSSIDKey(int index) {
    return String(NVS_KEY_SSID_PREFIX) + String(index);
}

String Storage::getPasswordKey(int index) {
    return String(NVS_KEY_PASS_PREFIX) + String(index);
}

bool Storage::deleteCredential(const String& ssid) {
    std::vector<WiFiCredential> credentials;
    loadCredentials(credentials);
    
    bool found = false;
    for (auto it = credentials.begin(); it != credentials.end(); ++it) {
        if (it->ssid == ssid) {
            credentials.erase(it);
            found = true;
            ESP_LOGI(TAG, "Deleted credential: %s", ssid.c_str());
            break;
        }
    }
    
    if (!found) {
        ESP_LOGW(TAG, "Credential not found: %s", ssid.c_str());
        return false;
    }
    
    // Clear and re-save remaining credentials
    clearCredentials();
    for (size_t i = 0; i < credentials.size(); i++) {
        String ssidKey = getSSIDKey(i);
        String passKey = getPasswordKey(i);
        preferences.putString(ssidKey.c_str(), credentials[i].ssid);
        preferences.putString(passKey.c_str(), credentials[i].password);
    }
    
    preferences.putInt(NVS_KEY_COUNT, credentials.size());
    preferences.putBool(NVS_KEY_CONFIGURED, credentials.size() > 0);
    
    ESP_LOGI(TAG, "Remaining credentials: %d", credentials.size());
    return true;
}

bool Storage::getCredentialsList(std::vector<String>& ssidList) {
    ssidList.clear();
    std::vector<WiFiCredential> credentials;
    if (loadCredentials(credentials)) {
        for (const auto& cred : credentials) {
            ssidList.push_back(cred.ssid);
        }
        ESP_LOGV(TAG, "Retrieved %d SSID(s)", ssidList.size());
        return true;
    }
    return false;
}

int Storage::getCredentialIndex(const String& ssid) {
    std::vector<WiFiCredential> credentials;
    loadCredentials(credentials);
    
    for (size_t i = 0; i < credentials.size(); i++) {
        if (credentials[i].ssid == ssid) {
            return i;
        }
    }
    return -1;
}

bool Storage::moveCredentialToFirst(const String& ssid) {
    std::vector<WiFiCredential> credentials;
    loadCredentials(credentials);
    
    int index = -1;
    for (size_t i = 0; i < credentials.size(); i++) {
        if (credentials[i].ssid == ssid) {
            index = i;
            break;
        }
    }
    
    if (index <= 0) {
        // Not found or already first
        return false;
    }
    
    // Move to front
    WiFiCredential temp = credentials[index];
    credentials.erase(credentials.begin() + index);
    credentials.insert(credentials.begin(), temp);
    
    // Save reordered list
    clearCredentials();
    for (size_t i = 0; i < credentials.size(); i++) {
        String ssidKey = getSSIDKey(i);
        String passKey = getPasswordKey(i);
        preferences.putString(ssidKey.c_str(), credentials[i].ssid);
        preferences.putString(passKey.c_str(), credentials[i].password);
    }
    
    preferences.putInt(NVS_KEY_COUNT, credentials.size());
    
    ESP_LOGI(TAG, "Moved '%s' to first priority", ssid.c_str());
    return true;
}
