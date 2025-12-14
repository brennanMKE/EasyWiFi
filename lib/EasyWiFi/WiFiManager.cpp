#include "WiFiManager.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

static const char *TAG = TAG_WIFI_MANAGER;

WiFiManager::WiFiManager() : apActive(false), mdnsActive(false), dnsServer(nullptr), captivePortalActive(false), errorHandler(TAG_WIFI_MANAGER) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    
    // Configure retry policy for WiFi operations
    RetryPolicy policy;
    policy.maxRetries = 3;
    policy.initialDelayMs = 2000;
    policy.backoffMultiplier = 1.5f;
    policy.maxDelayMs = 15000;
    errorHandler.setRetryPolicy(policy);
}

WiFiError WiFiManager::connectToStoredNetworksEx(Storage& storage, ErrorContext* outError) {
    if (isConnected()) {
        ESP_LOGI(TAG, "Already connected to WiFi");
        return WiFiError::SUCCESS;
    }
    
    // Log WiFi state before connection attempt
    ESP_LOGI(TAG, "========== Pre-Connection Diagnostics ==========");
    logWiFiDiagnostics();
    
    // Load credentials from storage
    std::vector<WiFiCredential> credentials;
    if (!storage.loadCredentials(credentials) || credentials.empty()) {
        ESP_LOGI(TAG, "No stored credentials found");
        errorHandler.recordError((uint32_t)WiFiError::SSID_NOT_FOUND, "No stored credentials");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return WiFiError::SSID_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Attempting to connect to %d stored network(s)", credentials.size());
    
    // === PRE-SCAN: Check which networks are in range ===
    std::vector<bool> networkInRange(credentials.size(), true);  // Assume all in range by default
    
    if (WIFI_PRE_SCAN_ENABLED && credentials.size() > 1) {
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Pre-Scan: Checking which networks are in range");
        ESP_LOGI(TAG, "========================================");
        
        WiFi.mode(WIFI_STA);
        int numScanned = WiFi.scanNetworks(false, false);  // Quick scan, don't show hidden
        
        if (numScanned > 0) {
            ESP_LOGI(TAG, "Found %d network(s) in range", numScanned);
            
            // Build list of available SSIDs
            std::vector<String> availableSSIDs;
            for (int i = 0; i < numScanned; i++) {
                String scannedSSID = WiFi.SSID(i);
                int channel = WiFi.channel(i);
                
                // Only consider 2.4GHz networks (channels 1-14)
                if (channel >= 1 && channel <= 14) {
                    availableSSIDs.push_back(scannedSSID);
                }
            }
            
            ESP_LOGI(TAG, "Found %d 2.4GHz network(s) in range", availableSSIDs.size());
            
            // Check which stored credentials are in range
            for (size_t i = 0; i < credentials.size(); i++) {
                networkInRange[i] = false;  // Assume not in range
                for (const auto& available : availableSSIDs) {
                    if (available == credentials[i].ssid) {
                        networkInRange[i] = true;
                        break;
                    }
                }
                
                ESP_LOGI(TAG, "  %s: %s", credentials[i].ssid.c_str(), 
                         networkInRange[i] ? "✓ In range" : "✗ Out of range");
            }
            
            // Clean up scan results
            WiFi.scanDelete();
        } else {
            ESP_LOGW(TAG, "Pre-scan found no networks or failed - will try all stored networks");
        }
    }
    
    // Log all credentials with in-range status
    for (size_t i = 0; i < credentials.size(); i++) {
        const auto& cred = credentials[i];
        ESP_LOGI(TAG, "  Network %d: '%s' (length: %d) %s", 
                 i + 1, cred.ssid.c_str(), cred.ssid.length(),
                 networkInRange[i] ? "[In Range]" : "[Out of Range - Will Skip]");
        ESP_LOGI(TAG, "    Password length: %d chars", cred.password.length());
        
        // Show password with masking (first 2 and last 2 chars visible)
        String maskedPass = "";
        if (cred.password.length() <= 4) {
            maskedPass = "****";
        } else {
            maskedPass = cred.password.substring(0, 2) + 
                        String("*").substring(0, cred.password.length() - 4) + 
                        cred.password.substring(cred.password.length() - 2);
        }
        ESP_LOGI(TAG, "    Password hint: %s", maskedPass.c_str());
    }
    
    // Configure WiFi for better eero compatibility (BEFORE any connection attempt)
    ESP_LOGI(TAG, "Configuring WiFi radio settings for optimal eero compatibility...");
    
    // Initialize WiFi in STA mode first (required before setting power/protocol)
    WiFi.mode(WIFI_STA);
    ESP_LOGI(TAG, "  ✓ WiFi initialized in STA mode");
    
    // CRITICAL: Lower transmit power for ESP32-C3 + Eero compatibility
    // High power can overload eero's receiver or cause interference with mesh
    if (WiFi.setTxPower(WIFI_POWER_8_5dBm)) {
        ESP_LOGI(TAG, "  ✓ Set transmit power to 8.5dBm (eero compatibility fix)");
    } else {
        ESP_LOGW(TAG, "  ✗ Failed to set transmit power");
    }
    
    // Force 802.11 b/g mode (disable 'n') - helps with some routers
    esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "  ✓ Set WiFi protocol to 802.11b/g (disabled 802.11n)");
    } else {
        ESP_LOGW(TAG, "  ✗ Failed to set WiFi protocol (error: %d)", result);
    }
    
    ESP_LOGI(TAG, "  ✓ eero compatibility settings applied");
    
    // Declare variables at function scope for use across phases
    unsigned long startTime = 0;
    unsigned long elapsedTime = 0;
    uint8_t status = WL_IDLE_STATUS;
    
    // === PHASE 1: Try first network with direct WiFi.begin() (eero compatibility) ===
    {
        bool phase1Skipped = false;
        
        if (networkInRange[0]) {
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Phase 1: Trying first network with eero-compatible settings");
            ESP_LOGI(TAG, "  Network 1/%d: %s", credentials.size(), credentials[0].ssid.c_str());
            ESP_LOGI(TAG, "  Settings: Low power (8.5dBm), 802.11b/g only");
            ESP_LOGI(TAG, "  Timeout: %d seconds", WIFI_CONNECTION_TIMEOUT / 1000);
            ESP_LOGI(TAG, "========================================");
        } else {
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Phase 1: SKIPPED - Network 1 (%s) is out of range", credentials[0].ssid.c_str());
            ESP_LOGI(TAG, "========================================");
            phase1Skipped = true;
        }
        
        if (!phase1Skipped) {
            startTime = millis();
            WiFi.begin(credentials[0].ssid.c_str(), credentials[0].password.c_str());
            
            // Poll WiFi.status() without blocking (check frequently, reset watchdog)
            status = WL_IDLE_STATUS;
            unsigned long lastCheck = millis();
            while (millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
                status = WiFi.status();
                if (status == WL_CONNECTED) {
                    break;
                }
                
                // Reset watchdog every 100ms to prevent timeout
                unsigned long now = millis();
                if (now - lastCheck >= 100) {
                    esp_task_wdt_reset();  // Keep watchdog happy
                    lastCheck = now;
                }
                // Don't use delay() - just loop and check status rapidly
                yield();  // Allow other tasks to run
            }
            
            elapsedTime = millis() - startTime;
            ESP_LOGI(TAG, "Phase 1 completed in %lu ms", elapsedTime);
            
            if (status == WL_CONNECTED) {
                // Phase 1 succeeded!
                ESP_LOGI(TAG, "========== Post-Connection Diagnostics ==========");
                logWiFiDiagnostics();
                ESP_LOGI(TAG, "✓ Phase 1 SUCCESS: Connected to %s", WiFi.SSID().c_str());
                ESP_LOGI(TAG, "  BSSID: %s", WiFi.BSSIDstr().c_str());
                ESP_LOGI(TAG, "  Channel: %d", WiFi.channel());
                ESP_LOGI(TAG, "  IP address: %s", WiFi.localIP().toString().c_str());
                ESP_LOGI(TAG, "  Signal strength: %d dBm", WiFi.RSSI());
                ESP_LOGI(TAG, "  Connection quality: %s", 
                         WiFi.RSSI() > -50 ? "Excellent" : 
                         WiFi.RSSI() > -60 ? "Good" : 
                         WiFi.RSSI() > -70 ? "Fair" : "Weak");
                errorHandler.recordRecovery();
                return WiFiError::SUCCESS;
            }
            
            // Phase 1 failed, log details
            ESP_LOGW(TAG, "✗ Phase 1 FAILED: Network 1 (%s) - %s", 
                     credentials[0].ssid.c_str(), getWiFiStatusName((wl_status_t)status));
        }
    }  // End Phase 1 block
    
    // === PHASE 2: Try remaining networks with WiFiMulti (standard settings) ===
    if (credentials.size() > 1) {
        // Count how many networks are in range for Phase 2
        int inRangeCount = 0;
        for (size_t i = 1; i < credentials.size(); i++) {
            if (networkInRange[i]) inRangeCount++;
        }
        
        if (inRangeCount == 0) {
            ESP_LOGW(TAG, "========================================");
            ESP_LOGW(TAG, "Phase 2: SKIPPED - No remaining networks in range");
            ESP_LOGW(TAG, "All %d stored network(s) are out of range", credentials.size());
            ESP_LOGW(TAG, "========================================");
            
            WiFiError error = WiFiError::SSID_NOT_FOUND;
            errorHandler.recordError((uint32_t)error, "All networks out of range");
            if (outError) {
                *outError = errorHandler.getStats().lastError;
            }
            return error;
        }
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Phase 2: Trying remaining %d in-range network(s) with standard settings", inRangeCount);
        ESP_LOGI(TAG, "  Settings: Full power (19.5dBm), 802.11b/g/n");
        ESP_LOGI(TAG, "========================================");
        
        // Disconnect from failed attempt
        WiFi.disconnect();
        unsigned long disconnectStart = millis();
        while (millis() - disconnectStart < 500) {
            esp_task_wdt_reset();
            yield();
        }
        
        // Reset to standard WiFi settings for better range/speed
        WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Full power
        esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, 
            WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "  ✓ Set WiFi protocol to 802.11b/g/n (full protocol support)");
        }
        ESP_LOGI(TAG, "  ✓ Set transmit power to 19.5dBm (full power)");
        
        // Add remaining networks to WiFiMulti (only those in range)
        // Note: WiFiMulti doesn't have a public clear method, but it will work fine
        int addedCount = 0;
        for (size_t i = 1; i < credentials.size(); i++) {
            if (networkInRange[i]) {
                addedCount++;
                ESP_LOGI(TAG, "  Adding network %d/%d: %s [In Range]", 
                         i + 1, credentials.size(), credentials[i].ssid.c_str());
                wifiMulti.addAP(credentials[i].ssid.c_str(), credentials[i].password.c_str());
            } else {
                ESP_LOGI(TAG, "  Skipping network %d/%d: %s [Out of Range]", 
                         i + 1, credentials.size(), credentials[i].ssid.c_str());
            }
        }
        
        if (addedCount == 0) {
            ESP_LOGW(TAG, "No in-range networks to add to Phase 2");
        }
        
        // Try to connect with WiFiMulti
        ESP_LOGI(TAG, "Starting Phase 2 connection attempt...");
        startTime = millis();
        status = WL_IDLE_STATUS;
        
        while (millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
            status = (wl_status_t)wifiMulti.run(1000);  // Check every second
            if (status == WL_CONNECTED) {
                break;
            }
            esp_task_wdt_reset();
            yield();
        }
        
        elapsedTime = millis() - startTime;
        ESP_LOGI(TAG, "Phase 2 completed in %lu ms", elapsedTime);
        
        if (status == WL_CONNECTED) {
            // Phase 2 succeeded!
            ESP_LOGI(TAG, "========== Post-Connection Diagnostics ==========");
            logWiFiDiagnostics();
            ESP_LOGI(TAG, "✓ Phase 2 SUCCESS: Connected to %s (from networks 2-%d)", 
                     WiFi.SSID().c_str(), credentials.size());
            ESP_LOGI(TAG, "  BSSID: %s", WiFi.BSSIDstr().c_str());
            ESP_LOGI(TAG, "  Channel: %d", WiFi.channel());
            ESP_LOGI(TAG, "  IP address: %s", WiFi.localIP().toString().c_str());
            ESP_LOGI(TAG, "  Signal strength: %d dBm", WiFi.RSSI());
            errorHandler.recordRecovery();
            
            // Move this successful network to first position for next boot
            ESP_LOGI(TAG, "Moving '%s' to priority 1 for faster connection next time", 
                     WiFi.SSID().c_str());
            storage.moveCredentialToFirst(WiFi.SSID());
            
            return WiFiError::SUCCESS;
        }
        
        ESP_LOGW(TAG, "✗ Phase 2 FAILED: All %d networks failed", credentials.size());
    }
    
    // All networks failed
    ESP_LOGE(TAG, "========================================");
    ESP_LOGE(TAG, "CONNECTION FAILED: All %d stored network(s) unreachable", credentials.size());
    ESP_LOGE(TAG, "========================================");
    
    WiFiError error = WiFiError::CONNECT_FAILED;
    if (status == WL_NO_SSID_AVAIL) {
        error = WiFiError::SSID_NOT_FOUND;
    } else if (status == WL_CONNECT_FAILED) {
        error = WiFiError::AUTH_FAILED;
    } else if (elapsedTime >= WIFI_CONNECTION_TIMEOUT - 1000) {
        error = WiFiError::CONNECT_TIMEOUT;
    }
    
    errorHandler.recordError((uint32_t)error, "All stored networks failed");
    if (outError) {
        *outError = errorHandler.getStats().lastError;
    }
    
    return error;
}

// Backward compatible wrapper
bool WiFiManager::connectToStoredNetworks(Storage& storage) {
    WiFiError error = connectToStoredNetworksEx(storage, nullptr);
    return error == WiFiError::SUCCESS;
}

WiFiError WiFiManager::startAccessPointEx(ErrorContext* outError) {
    if (apActive) {
        ESP_LOGI(TAG, "Access Point already active");
        return WiFiError::SUCCESS;
    }
    
    // Generate unique AP SSID
    apSSID = generateAPSSID();
    apPassword = AP_PASSWORD;
    
    ESP_LOGI(TAG, "Starting Access Point: %s", apSSID.c_str());
    
    // CRITICAL: Stop STA mode and disable auto-reconnect to prevent interference
    // The WiFi radio can't properly run AP mode if STA is constantly trying to reconnect
    ESP_LOGI(TAG, "Stopping STA mode and disabling auto-reconnect...");
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true, true);  // disconnect and erase config
    WiFi.mode(WIFI_AP);  // Set to AP-only mode (not STA+AP)
    
    // Give WiFi stack time to switch modes (non-blocking)
    unsigned long modeSwitch = millis();
    while (millis() - modeSwitch < 100) {
        yield();
    }
    
    // Configure static IP for AP
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    local_IP.fromString(AP_IP);
    gateway.fromString(AP_GATEWAY);
    subnet.fromString(AP_SUBNET);
    
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        ESP_LOGE(TAG, "Failed to configure AP IP");
        errorHandler.recordError((uint32_t)WiFiError::AP_START_FAILED, "Failed to configure AP IP");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return WiFiError::AP_START_FAILED;
    }
    
    // Start AP
    bool success;
    if (apPassword.length() > 0) {
        success = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), AP_CHANNEL, 0, AP_MAX_CONNECTIONS);
    } else {
        success = WiFi.softAP(apSSID.c_str(), nullptr, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);
    }
    
    if (!success) {
        ESP_LOGE(TAG, "Failed to start Access Point");
        errorHandler.recordError((uint32_t)WiFiError::AP_START_FAILED, "Failed to start Access Point");
        if (outError) {
            *outError = errorHandler.getStats().lastError;
        }
        return WiFiError::AP_START_FAILED;
    }
    
    apActive = true;
    errorHandler.recordRecovery();
    
    ESP_LOGI(TAG, "Access Point started successfully");
    ESP_LOGI(TAG, "SSID: %s", apSSID.c_str());
    ESP_LOGI(TAG, "IP: %s", WiFi.softAPIP().toString().c_str());
    ESP_LOGI(TAG, "Password: %s", apPassword.length() > 0 ? "Protected" : "Open");
    
    return WiFiError::SUCCESS;
}

// Backward compatible wrapper
bool WiFiManager::startAccessPoint() {
    WiFiError error = startAccessPointEx(nullptr);
    return error == WiFiError::SUCCESS;
}

void WiFiManager::stopAccessPoint() {
    if (!apActive) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping Access Point");
    WiFi.softAPdisconnect(true);
    apActive = false;
    ESP_LOGI(TAG, "Access Point stopped");
}

bool WiFiManager::scanNetworks(std::vector<WiFiNetwork>& results) {
    results.clear();
    
    ESP_LOGI(TAG, "Scanning for WiFi networks...");
    
    int numNetworks = WiFi.scanNetworks();
    
    if (numNetworks == -1) {
        ESP_LOGE(TAG, "WiFi scan failed");
        return false;
    }
    
    ESP_LOGI(TAG, "Found %d network(s)", numNetworks);
    
    int filtered5GHz = 0;
    
    for (int i = 0; i < numNetworks; i++) {
        int channel = WiFi.channel(i);
        
        // ESP32-C3 only supports 2.4GHz WiFi (channels 1-14)
        // 5GHz channels start at 36+
        if (channel >= 36) {
            filtered5GHz++;
            ESP_LOGD(TAG, "  ✗ Filtered 5GHz: %s (channel %d)", 
                     WiFi.SSID(i).c_str(), channel);
            continue;  // Skip 5GHz networks
        }
        
        WiFiNetwork network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.channel = channel;
        network.encryptionType = WiFi.encryptionType(i);
        network.encryptionName = getEncryptionTypeName(network.encryptionType);
        network.signalBars = getSignalBars(network.rssi);
        network.signalStrength = getSignalStrength(network.rssi);
        
        results.push_back(network);
        
        ESP_LOGV(TAG, "  ✓ %d: %s (ch %d, %d dBm) %s [%s]", 
                 i + 1, 
                 network.ssid.c_str(),
                 channel,
                 network.rssi,
                 network.signalStrength.c_str(),
                 network.encryptionName.c_str());
    }
    
    if (filtered5GHz > 0) {
        ESP_LOGI(TAG, "Filtered out %d 5GHz network(s) - ESP32-C3 only supports 2.4GHz", 
                 filtered5GHz);
    }
    
    // Sort by signal strength (strongest first)
    std::sort(results.begin(), results.end(), [](const WiFiNetwork& a, const WiFiNetwork& b) {
        return a.rssi > b.rssi;
    });
    
    ESP_LOGI(TAG, "Returning %d compatible 2.4GHz network(s)", results.size());
    return true;
}

void WiFiManager::addCredentials(const String& ssid, const String& password) {
    wifiMulti.addAP(ssid.c_str(), password.c_str());
    ESP_LOGI(TAG, "Added credentials for immediate connection: %s", ssid.c_str());
}

String WiFiManager::getStatus() {
    if (apActive) {
        return STATUS_AP_MODE;
    }
    
    wl_status_t status = WiFi.status();
    return String(getWiFiStatusName(status));
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::isAPMode() {
    return apActive;
}

String WiFiManager::getAPSSID() {
    return apSSID;
}

String WiFiManager::getLocalIP() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    } else if (apActive) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

const char* WiFiManager::getEncryptionTypeName(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2-PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3-PSK";
        default:
            return "Unknown";
    }
}

const char* WiFiManager::getWiFiStatusName(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS:
            return "Idle";
        case WL_NO_SSID_AVAIL:
            return "No SSID Available";
        case WL_SCAN_COMPLETED:
            return "Scan Completed";
        case WL_CONNECTED:
            return STATUS_CONNECTED;
        case WL_CONNECT_FAILED:
            return "Connection Failed";
        case WL_CONNECTION_LOST:
            return "Connection Lost";
        case WL_DISCONNECTED:
            return STATUS_DISCONNECTED;
        default:
            return "Unknown";
    }
}

String WiFiManager::generateAPSSID() {
    // Get MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // Create SSID with last 3 bytes of MAC
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s%02X%02X%02X", 
             AP_SSID_PREFIX, 
             mac[3], mac[4], mac[5]);
    
    return String(ssid);
}

bool WiFiManager::startMDNS(const String& hostname) {
    if (mdnsActive) {
        ESP_LOGI(TAG, "mDNS already active");
        return true;
    }
    
    // Use provided hostname or generate one
    String mdnsName = hostname;
    if (mdnsName.length() == 0) {
        // Generate hostname from device MAC
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char name[32];
        snprintf(name, sizeof(name), "easywifi-%02x%02x%02x", 
                 mac[3], mac[4], mac[5]);
        mdnsName = String(name);
    }
    
    ESP_LOGI(TAG, "Starting mDNS with hostname: %s", mdnsName.c_str());
    
    // Try to start mDNS with the hostname
    int attempt = 0;
    while (attempt < 100) {
        String attemptedName = attempt == 0 ? mdnsName : mdnsName + String(attempt);
        
        if (MDNS.begin(attemptedName.c_str())) {
            mdnsHostname = attemptedName;
            mdnsActive = true;
            ESP_LOGI(TAG, "mDNS started successfully: %s.local", mdnsHostname.c_str());
            return true;
        } else {
            ESP_LOGV(TAG, "Conflict with hostname: %s", attemptedName.c_str());
            attempt++;
            MDNS.end();
        }
    }
    
    ESP_LOGE(TAG, "Failed to start mDNS after %d attempts", attempt);
    return false;
}

void WiFiManager::announceMDNS() {
    if (!mdnsActive) {
        ESP_LOGW(TAG, "Cannot announce mDNS service - mDNS not active");
        return;
    }
    
    // Announce HTTP service
    if (MDNS.addService("http", "tcp", 80)) {
        ESP_LOGI(TAG, "mDNS HTTP service announced");
    } else {
        ESP_LOGW(TAG, "Failed to announce mDNS HTTP service");
    }
    
    // Add service TXT records
    MDNS.addServiceTxt("http", "tcp", "version", "0.1.0");
    MDNS.addServiceTxt("http", "tcp", "device", "ESP32-C3");
    MDNS.addServiceTxt("http", "tcp", "app", "EasyWiFi");
    
    ESP_LOGI(TAG, "Access device at: http://%s.local/", mdnsHostname.c_str());
}

String WiFiManager::getMDNSHostname() {
    return mdnsHostname;
}

bool WiFiManager::startCaptivePortal() {
    if (captivePortalActive) {
        ESP_LOGI(TAG, "Captive portal already active");
        return true;
    }
    
    if (!apActive) {
        ESP_LOGW(TAG, "Cannot start captive portal - AP not active");
        return false;
    }
    
    ESP_LOGI(TAG, "Starting captive portal DNS server");
    
    // Create DNS server
    dnsServer = new DNSServer();
    
    // Start DNS server on port 53, redirect all requests to AP IP
    IPAddress apIP;
    apIP.fromString(AP_IP);
    
    if (dnsServer->start(53, "*", apIP)) {
        captivePortalActive = true;
        ESP_LOGI(TAG, "Captive portal started successfully");
        ESP_LOGI(TAG, "All DNS requests will redirect to %s", AP_IP);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to start captive portal DNS server");
        delete dnsServer;
        dnsServer = nullptr;
        return false;
    }
}

void WiFiManager::stopCaptivePortal() {
    if (!captivePortalActive) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping captive portal");
    
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    
    captivePortalActive = false;
    ESP_LOGI(TAG, "Captive portal stopped");
}

void WiFiManager::processCaptivePortal() {
    if (captivePortalActive && dnsServer) {
        dnsServer->processNextRequest();
    }
}

bool WiFiManager::isCaptivePortalActive() {
    return captivePortalActive;
}

const char* WiFiManager::getDisconnectReasonName(uint8_t reason) {
    switch (reason) {
        case 1: return "UNSPECIFIED";
        case 2: return "AUTH_EXPIRE";
        case 3: return "AUTH_LEAVE";
        case 4: return "ASSOC_EXPIRE";
        case 5: return "ASSOC_TOOMANY";
        case 6: return "NOT_AUTHED";
        case 7: return "NOT_ASSOCED";
        case 8: return "ASSOC_LEAVE";
        case 9: return "ASSOC_NOT_AUTHED";
        case 10: return "DISASSOC_PWRCAP_BAD";
        case 11: return "DISASSOC_SUPCHAN_BAD";
        case 13: return "IE_INVALID";
        case 14: return "MIC_FAILURE";
        case 15: return "4WAY_HANDSHAKE_TIMEOUT";
        case 16: return "GROUP_KEY_UPDATE_TIMEOUT";
        case 17: return "IE_IN_4WAY_DIFFERS";
        case 18: return "GROUP_CIPHER_INVALID";
        case 19: return "PAIRWISE_CIPHER_INVALID";
        case 20: return "AKMP_INVALID";
        case 21: return "UNSUPP_RSN_IE_VERSION";
        case 22: return "INVALID_RSN_IE_CAP";
        case 23: return "802_1X_AUTH_FAILED";
        case 24: return "CIPHER_SUITE_REJECTED";
        case 34: return "MISSING_ACKS";
        case 39: return "TIMEOUT";
        case 200: return "BEACON_TIMEOUT";
        case 201: return "NO_AP_FOUND";
        case 202: return "AUTH_FAIL";
        case 203: return "ASSOC_FAIL";
        case 204: return "HANDSHAKE_TIMEOUT";
        case 205: return "CONNECTION_FAIL";
        case 206: return "AP_TSF_RESET";
        case 207: return "ROAMING";
        default: return "UNKNOWN";
    }
}

void WiFiManager::logWiFiDiagnostics() {
    ESP_LOGI(TAG, "========== WiFi Diagnostics ==========");
    ESP_LOGI(TAG, "WiFi Mode: %s", WiFi.getMode() == WIFI_STA ? "STA" : 
                                     WiFi.getMode() == WIFI_AP ? "AP" :
                                     WiFi.getMode() == WIFI_AP_STA ? "AP_STA" : "OFF");
    ESP_LOGI(TAG, "WiFi Status: %d (%s)", WiFi.status(), getWiFiStatusName((wl_status_t)WiFi.status()));
    ESP_LOGI(TAG, "Auto-Connect: %s", WiFi.getAutoConnect() ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "Auto-Reconnect: %s", WiFi.getAutoReconnect() ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "WiFi Power: %.1f dBm", WiFi.getTxPower());
    ESP_LOGI(TAG, "Hostname: %s", WiFi.getHostname());
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "Connected SSID: %s", WiFi.SSID().c_str());
        ESP_LOGI(TAG, "BSSID: %s", WiFi.BSSIDstr().c_str());
        ESP_LOGI(TAG, "Channel: %d", WiFi.channel());
        ESP_LOGI(TAG, "RSSI: %d dBm", WiFi.RSSI());
        ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
    }
    ESP_LOGI(TAG, "======================================");
}
