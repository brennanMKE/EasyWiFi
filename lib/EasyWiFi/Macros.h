#ifndef MACROS_H
#define MACROS_H

#include "Arduino.h"

// WiFi Configuration Constants
#define WIFI_CONNECTION_TIMEOUT 30000      // Milliseconds to wait for WiFi connection (30 seconds)
#define WIFI_CONNECTION_ATTEMPTS 1         // Number of connection attempts
#define WIFI_CONNECTION_RETRY_DELAY 250    // Delay between connection attempts (ms)
#define WIFI_SCAN_TIMEOUT 5000            // Timeout for WiFi scan (ms)
#define MAX_STORED_NETWORKS 5              // Maximum number of stored WiFi credentials
#define WIFI_PRE_SCAN_ENABLED true        // Scan before connecting to filter out-of-range networks
#define CONNECTION_STATE_TIMEOUT 30000     // Max time in CONNECTING state (30 seconds)
#define AP_MODE_RETRY_DELAY 60000          // Wait 60 seconds in AP mode before auto-retry (ms)
#define MAX_CONSECUTIVE_FAILURES 3         // Max failures before staying in AP mode permanently

// Access Point Configuration
#define AP_SSID_PREFIX "EasyWiFi-Setup-"   // AP SSID prefix
#define AP_PASSWORD ""                      // AP password (empty = open network)
#define AP_CHANNEL 1                        // WiFi channel for AP
#define AP_MAX_CONNECTIONS 4                // Maximum simultaneous connections to AP
#define AP_IP "192.168.4.1"                // Static IP for AP
#define AP_GATEWAY "192.168.4.1"           // Gateway IP for AP
#define AP_SUBNET "255.255.255.0"          // Subnet mask for AP
#define AP_TIMEOUT 300000                   // AP auto-disable timeout (5 minutes, ms)

// LED Configuration
#define LED_PIN 8                           // Status LED pin (ESP32-C3 builtin LED)
#define LED_ENABLED true                    // Enable status LED
#define LED_BRIGHTNESS 128                  // LED brightness (0-255)

// Logging Configuration
#define STATUS_LOG_INTERVAL 120000          // Status logging interval (2 minutes, ms)
#define MEMORY_WARNING_THRESHOLD 50000      // Low memory warning threshold (bytes)

// Web Server Configuration
#define WEB_SERVER_PORT 80                  // HTTP server port
#define WEB_SERVER_STACK_SIZE 8192         // Server task stack size

// REST API Endpoints
#define ENDPOINT_ROOT "/"
#define ENDPOINT_SCAN "/scan"
#define ENDPOINT_CONFIGURE "/configure"
#define ENDPOINT_SAVE "/save"
#define ENDPOINT_STATUS "/status"
#define ENDPOINT_RESET "/reset"
#define ENDPOINT_API_STATUS "/api/status"
#define ENDPOINT_API_SCAN "/api/scan"
#define ENDPOINT_API_CONFIGURE "/api/configure"
#define ENDPOINT_API_CONNECT "/api/connect"
#define ENDPOINT_API_CREDENTIALS "/api/credentials"
#define ENDPOINT_API_RESET "/api/reset"
#define ENDPOINT_API_HEALTH "/api/health"
#define ENDPOINT_CREDENTIALS "/credentials"

// Storage Configuration (NVS)
#define NVS_NAMESPACE "easywifi"
#define NVS_KEY_COUNT "wifi_count"
#define NVS_KEY_SSID_PREFIX "ssid_"
#define NVS_KEY_PASS_PREFIX "pass_"
#define NVS_KEY_CONFIGURED "configured"

// Logging Tags
#define TAG_MAIN "MAIN"
#define TAG_RUNLOOP "RUNLOOP"
#define TAG_WIFI_MANAGER "WIFI_MGR"
#define TAG_CONFIG_SERVER "CONFIG_SRV"
#define TAG_STORAGE "STORAGE"
#define TAG_WEB_PAGES "WEB_PAGES"

// Status Messages
#define STATUS_INITIALIZING "Initializing"
#define STATUS_LOADING_CREDENTIALS "Loading credentials"
#define STATUS_CONNECTING "Connecting to WiFi"
#define STATUS_CONNECTED "Connected"
#define STATUS_AP_MODE "Access Point mode"
#define STATUS_ERROR "Error"
#define STATUS_DISCONNECTED "Disconnected"

// HTTP Status Codes
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_CREATED 201
#define HTTP_STATUS_NO_CONTENT 204
#define HTTP_STATUS_REDIRECT 302
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_SERVER_ERROR 500

// Utility Macros
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// WiFi Signal Strength Ranges (RSSI in dBm)
#define RSSI_EXCELLENT -50
#define RSSI_GOOD -60
#define RSSI_FAIR -70
#define RSSI_WEAK -80

// Helper function to get signal strength description
inline const char* getSignalStrength(int rssi) {
    if (rssi >= RSSI_EXCELLENT) return "Excellent";
    if (rssi >= RSSI_GOOD) return "Good";
    if (rssi >= RSSI_FAIR) return "Fair";
    if (rssi >= RSSI_WEAK) return "Weak";
    return "Very Weak";
}

// Helper function to get signal bars (0-4)
inline int getSignalBars(int rssi) {
    if (rssi >= RSSI_EXCELLENT) return 4;
    if (rssi >= RSSI_GOOD) return 3;
    if (rssi >= RSSI_FAIR) return 2;
    if (rssi >= RSSI_WEAK) return 1;
    return 0;
}

// Error Handling Configuration
#define ERROR_RECOVERY_ENABLED true        // Enable automatic error recovery
#define MAX_RECOVERY_ATTEMPTS 5            // Max recovery attempts before giving up
#define ERROR_RECOVERY_TIMEOUT 300000      // Max time in error state (5 minutes)
#define RETRY_BACKOFF_BASE_MS 5000         // Base delay for exponential backoff
#define MAX_CONSECUTIVE_ERRORS_BEFORE_AP 3 // Consecutive errors before forcing AP mode
#define ERROR_LOG_RATE_LIMIT_MS 60000      // Rate limit repeated error logs
#define ERROR_HANDLE_RATE_LIMIT_MS 1000    // Minimum time between error handling attempts (prevents tight loops)
#define FORCE_AP_ON_NO_NETWORKS true       // Auto-enter AP mode when no networks available

#endif // MACROS_H
