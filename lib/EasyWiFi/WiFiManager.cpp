#include "WiFiManager.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

static const char *TAG = TAG_WIFI_MANAGER;

WiFiManager::WiFiManager() : apActive(false), mdnsActive(false), dnsServer(nullptr), captivePortalActive(false) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
}

bool WiFiManager::connectToStoredNetworks(Storage& storage) {
    if (isConnected()) {
        ESP_LOGI(TAG, "Already connected to WiFi");
        return true;
    }
    
    // Log WiFi state before connection attempt
    ESP_LOGI(TAG, "========== Pre-Connection Diagnostics ==========");
    logWiFiDiagnostics();
    
    // Load credentials from storage
    std::vector<WiFiCredential> credentials;
    if (!storage.loadCredentials(credentials) || credentials.empty()) {
        ESP_LOGI(TAG, "No stored credentials found");
        return false;
    }
    
    ESP_LOGI(TAG, "Attempting to connect to %d stored network(s)", credentials.size());
    
    // Log all credentials
    for (const auto& cred : credentials) {
        ESP_LOGI(TAG, "  SSID: '%s' (length: %d)", cred.ssid.c_str(), cred.ssid.length());
        ESP_LOGI(TAG, "  Password length: %d chars", cred.password.length());
        
        // Show password with masking (first 2 and last 2 chars visible)
        String maskedPass = "";
        if (cred.password.length() <= 4) {
            maskedPass = "****";
        } else {
            maskedPass = cred.password.substring(0, 2) + 
                        String("*").substring(0, cred.password.length() - 4) + 
                        cred.password.substring(cred.password.length() - 2);
        }
        ESP_LOGI(TAG, "  Password hint: %s", maskedPass.c_str());
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
    
    // Use direct WiFi.begin() like the working ConnectWiFi code, not WiFiMulti
    // WiFiMulti may override our carefully configured settings
    ESP_LOGI(TAG, "Starting connection attempt (timeout: %d seconds)...", WIFI_CONNECTION_TIMEOUT / 1000);
    ESP_LOGI(TAG, "Using direct WiFi.begin() (ConnectWiFi method)");
    
    unsigned long startTime = millis();
    WiFi.begin(credentials[0].ssid.c_str(), credentials[0].password.c_str());
    
    // Poll WiFi.status() without blocking (check frequently, reset watchdog)
    uint8_t status = WL_IDLE_STATUS;
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
    
    unsigned long elapsedTime = millis() - startTime;
    ESP_LOGI(TAG, "Connection attempt completed in %lu ms", elapsedTime);
    
    // Log WiFi state after connection attempt
    ESP_LOGI(TAG, "========== Post-Connection Diagnostics ==========");
    logWiFiDiagnostics();
    
    if (status == WL_CONNECTED) {
        ESP_LOGI(TAG, "✓ Successfully connected to %s", WiFi.SSID().c_str());
        ESP_LOGI(TAG, "  BSSID: %s", WiFi.BSSIDstr().c_str());
        ESP_LOGI(TAG, "  Channel: %d", WiFi.channel());
        ESP_LOGI(TAG, "  IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "  Signal strength: %d dBm", WiFi.RSSI());
        ESP_LOGI(TAG, "  Connection quality: %s", 
                 WiFi.RSSI() > -50 ? "Excellent" : 
                 WiFi.RSSI() > -60 ? "Good" : 
                 WiFi.RSSI() > -70 ? "Fair" : "Weak");
        return true;
    } else {
        ESP_LOGW(TAG, "✗ Connection attempt failed");
        ESP_LOGW(TAG, "  WiFi status: %d (%s)", status, getWiFiStatusName((wl_status_t)status));
        ESP_LOGW(TAG, "  Time elapsed: %lu ms (timeout was %d ms)", elapsedTime, WIFI_CONNECTION_TIMEOUT);
        
        // Check if we timed out
        if (elapsedTime >= WIFI_CONNECTION_TIMEOUT - 1000) {
            ESP_LOGW(TAG, "  Connection timed out - network may be unreachable or too slow to respond");
        }
        
        return false;
    }
}

bool WiFiManager::startAccessPoint() {
    if (apActive) {
        ESP_LOGI(TAG, "Access Point already active");
        return true;
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
        return false;
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
        return false;
    }
    
    apActive = true;
    
    ESP_LOGI(TAG, "Access Point started successfully");
    ESP_LOGI(TAG, "SSID: %s", apSSID.c_str());
    ESP_LOGI(TAG, "IP: %s", WiFi.softAPIP().toString().c_str());
    ESP_LOGI(TAG, "Password: %s", apPassword.length() > 0 ? "Protected" : "Open");
    
    return true;
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

