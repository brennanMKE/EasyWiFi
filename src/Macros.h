#ifndef EWIFI_MACROS_H
#define EWIFI_MACROS_H

#include "Arduino.h"

// WiFi Configuration Constants
#define EWIFI_WIFI_CONNECTION_TIMEOUT 30000      // Milliseconds to wait for WiFi connection (30 seconds)
#define EWIFI_WIFI_CONNECTION_ATTEMPTS 1         // Number of connection attempts
#define EWIFI_WIFI_CONNECTION_RETRY_DELAY 250    // Delay between connection attempts (ms)
#define EWIFI_WIFI_SCAN_TIMEOUT 5000            // Timeout for WiFi scan (ms)
#define EWIFI_MAX_STORED_NETWORKS 5              // Maximum number of stored WiFi credentials
#define EWIFI_WIFI_PRE_SCAN_ENABLED true        // Scan before connecting to filter out-of-range networks
#define EWIFI_CONNECTION_STATE_TIMEOUT 30000     // Max time in CONNECTING state (30 seconds)
#define EWIFI_AP_MODE_RETRY_DELAY 60000          // Wait 60 seconds in AP mode before auto-retry (ms)
#define EWIFI_MAX_CONSECUTIVE_FAILURES 3         // Max failures before staying in AP mode permanently

// Access Point Configuration
#define EWIFI_AP_SSID_PREFIX "EasyWiFi-Setup-"   // AP SSID prefix
#define EWIFI_AP_PASSWORD ""                      // AP password (empty = open network)
#define EWIFI_AP_CHANNEL 1                        // WiFi channel for AP
#define EWIFI_AP_MAX_CONNECTIONS 4                // Maximum simultaneous connections to AP
#define EWIFI_AP_IP "192.168.4.1"                // Static IP for AP
#define EWIFI_AP_GATEWAY "192.168.4.1"           // Gateway IP for AP
#define EWIFI_AP_SUBNET "255.255.255.0"          // Subnet mask for AP
#define EWIFI_AP_TIMEOUT 300000                   // AP auto-disable timeout (5 minutes, ms)

// LED Configuration
#define EWIFI_LED_PIN 8                           // Status LED pin (ESP32-C3 builtin LED)
#define EWIFI_LED_ENABLED true                    // Enable status LED
#define EWIFI_LED_BRIGHTNESS 128                  // LED brightness (0-255)

// Logging Configuration
#define EWIFI_STATUS_LOG_INTERVAL 120000          // Status logging interval (2 minutes, ms)
#define EWIFI_MEMORY_WARNING_THRESHOLD 50000      // Low memory warning threshold (bytes)

// Web Server Configuration
#define EWIFI_WEB_SERVER_PORT 80                  // HTTP server port
#define EWIFI_WEB_SERVER_STACK_SIZE 8192         // Server task stack size

// REST API Endpoints
// WiFi configuration is now under /wifi to allow custom pages at root /
#define EWIFI_ENDPOINT_ROOT "/wifi"
#define EWIFI_ENDPOINT_SCAN "/wifi/scan"
#define EWIFI_ENDPOINT_CONFIGURE "/wifi/configure"
#define EWIFI_ENDPOINT_SAVE "/wifi/save"
#define EWIFI_ENDPOINT_STATUS "/wifi/status"
#define EWIFI_ENDPOINT_RESET "/wifi/reset"
#define EWIFI_ENDPOINT_API_STATUS "/wifi/api/status"
#define EWIFI_ENDPOINT_API_SCAN "/wifi/api/scan"
#define EWIFI_ENDPOINT_API_CONFIGURE "/wifi/api/configure"
#define EWIFI_ENDPOINT_API_CONNECT "/wifi/api/connect"
#define EWIFI_ENDPOINT_API_CREDENTIALS "/wifi/api/credentials"
#define EWIFI_ENDPOINT_API_RESET "/wifi/api/reset"
#define EWIFI_ENDPOINT_API_HEALTH "/wifi/api/health"
#define EWIFI_ENDPOINT_CREDENTIALS "/wifi/credentials"

// Storage Configuration (NVS)
#define EWIFI_NVS_NAMESPACE "easywifi"
#define EWIFI_NVS_KEY_COUNT "wifi_count"
#define EWIFI_NVS_KEY_SSID_PREFIX "ssid_"
#define EWIFI_NVS_KEY_PASS_PREFIX "pass_"
#define EWIFI_NVS_KEY_CONFIGURED "configured"

// Logging Tags
#define EWIFI_TAG_MAIN "MAIN"
#define EWIFI_TAG_RUNLOOP "RUNLOOP"
#define EWIFI_TAG_WIFI_MANAGER "WIFI_MGR"
#define EWIFI_TAG_CONFIG_SERVER "CONFIG_SRV"
#define EWIFI_TAG_STORAGE "STORAGE"
#define EWIFI_TAG_WEB_PAGES "WEB_PAGES"

// Status Messages
#define EWIFI_STATUS_INITIALIZING "Initializing"
#define EWIFI_STATUS_LOADING_CREDENTIALS "Loading credentials"
#define EWIFI_STATUS_CONNECTING "Connecting to WiFi"
#define EWIFI_STATUS_CONNECTED "Connected"
#define EWIFI_STATUS_AP_MODE "Access Point mode"
#define EWIFI_STATUS_ERROR "Error"
#define EWIFI_STATUS_DISCONNECTED "Disconnected"

// HTTP Status Codes
#define EWIFI_HTTP_STATUS_OK 200
#define EWIFI_HTTP_STATUS_CREATED 201
#define EWIFI_HTTP_STATUS_NO_CONTENT 204
#define EWIFI_HTTP_STATUS_REDIRECT 302
#define EWIFI_HTTP_STATUS_BAD_REQUEST 400
#define EWIFI_HTTP_STATUS_NOT_FOUND 404
#define EWIFI_HTTP_STATUS_SERVER_ERROR 500

// Utility Macros
#define EWIFI_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define EWIFI_MIN(a, b) ((a) < (b) ? (a) : (b))
#define EWIFI_MAX(a, b) ((a) > (b) ? (a) : (b))

// WiFi Signal Strength Ranges (RSSI in dBm)
#define EWIFI_RSSI_EXCELLENT -50
#define EWIFI_RSSI_GOOD -60
#define EWIFI_RSSI_FAIR -70
#define EWIFI_RSSI_WEAK -80

// Helper function to get signal strength description
inline const char* getSignalStrength(int rssi) {
    if (rssi >= EWIFI_RSSI_EXCELLENT) return "Excellent";
    if (rssi >= EWIFI_RSSI_GOOD) return "Good";
    if (rssi >= EWIFI_RSSI_FAIR) return "Fair";
    if (rssi >= EWIFI_RSSI_WEAK) return "Weak";
    return "Very Weak";
}

// Helper function to get signal bars (0-4)
inline int getSignalBars(int rssi) {
    if (rssi >= EWIFI_RSSI_EXCELLENT) return 4;
    if (rssi >= EWIFI_RSSI_GOOD) return 3;
    if (rssi >= EWIFI_RSSI_FAIR) return 2;
    if (rssi >= EWIFI_RSSI_WEAK) return 1;
    return 0;
}

// Error Handling Configuration
#define EWIFI_ERROR_RECOVERY_ENABLED true        // Enable automatic error recovery
#define EWIFI_MAX_RECOVERY_ATTEMPTS 5            // Max recovery attempts before giving up
#define EWIFI_ERROR_RECOVERY_TIMEOUT 300000      // Max time in error state (5 minutes)
#define EWIFI_RETRY_BACKOFF_BASE_MS 5000         // Base delay for exponential backoff
#define EWIFI_MAX_CONSECUTIVE_ERRORS_BEFORE_AP 3 // Consecutive errors before forcing AP mode
#define EWIFI_ERROR_LOG_RATE_LIMIT_MS 60000      // Rate limit repeated error logs
#define EWIFI_ERROR_HANDLE_RATE_LIMIT_MS 1000    // Minimum time between error handling attempts (prevents tight loops)
#define EWIFI_FORCE_AP_ON_NO_NETWORKS true       // Auto-enter AP mode when no networks available

// Captive Portal Detection Strings
// When devices connect to WiFi, they check for internet by requesting known URLs
// These are common URLs that various OSes use for captive portal detection
static const char* CAPTIVE_PORTAL_DETECTION_STRINGS[] = {
    "generate_204",      // Android
    "hotspot-detect",    // iOS/macOS
    "connecttest",       // Windows
    "success.txt",       // Firefox
    "canonical.html",    // Ubuntu
    "ncsi.txt",          // Windows Network Connectivity Status Indicator
    "redirect"           // Generic
};

// Helper function to check if a URI matches captive portal detection patterns
inline bool isCaptivePortalDetection(const String& uri) {
    for (size_t i = 0; i < EWIFI_ARRAY_SIZE(CAPTIVE_PORTAL_DETECTION_STRINGS); i++) {
        if (uri.indexOf(CAPTIVE_PORTAL_DETECTION_STRINGS[i]) >= 0) {
            return true;
        }
    }
    return false;
}

#endif // EWIFI_MACROS_H
