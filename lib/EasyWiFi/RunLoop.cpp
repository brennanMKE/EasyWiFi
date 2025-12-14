#include "RunLoop.h"
#include <esp_task_wdt.h>

static const char *TAG = TAG_RUNLOOP;

// Watchdog timeout in seconds (must be longer than WIFI_CONNECTION_TIMEOUT)
#define WDT_TIMEOUT 60

RunLoop::RunLoop() : 
    storage(),
    wifiManager(),
    configServer(wifiManager, storage),
    statusLED(LED_ENABLED ? LED_PIN : -1),
    currentState(INITIALIZING),
    stateStartTime(0),
    lastStatusLog(0),
    connectionAttempts(0),
    connectionFailed(false),
    consecutiveFailures(0),
    lastConnectionAttemptTime(0),
    connectionRequested(false) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
}

void RunLoop::setup() {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "EasyWiFi Starting");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    
    // Initialize watchdog timer
    ESP_LOGI(TAG, "Initializing watchdog timer (%d seconds)", WDT_TIMEOUT);
    esp_task_wdt_init(WDT_TIMEOUT, true);  // Enable panic on timeout
    esp_task_wdt_add(NULL);  // Add current task
    
    // Suppress verbose hardware logs (LED PWM, WiFi stack details)
    esp_log_level_set("esp32-hal-ledc", ESP_LOG_WARN);  // Only show LED warnings/errors
    esp_log_level_set("wifi", ESP_LOG_INFO);            // Only show WiFi info and above
    
    // Initialize status LED
    statusLED.begin();
    if (statusLED.isEnabled()) {
        statusLED.setBrightness(LED_BRIGHTNESS);
    }
    
    // Wire up RunLoop reference to ConfigServer
    configServer.setRunLoop(this);
    
    transitionToState(INITIALIZING);
}

void RunLoop::loop() {
    // Feed the watchdog
    esp_task_wdt_reset();
    
    // Handle current state
    switch (currentState) {
        case INITIALIZING:
            handleInitializing();
            break;
        case LOADING_CREDENTIALS:
            handleLoadingCredentials();
            break;
        case CONNECTING:
            handleConnecting();
            break;
        case CONNECTED:
            handleConnected();
            break;
        case AP_MODE:
            handleAPMode();
            break;
        case ERROR:
            handleError();
            break;
    }
    
    // Process captive portal DNS requests
    wifiManager.processCaptivePortal();
    
    // Periodic status logging (configurable interval)
    if (millis() - lastStatusLog > STATUS_LOG_INTERVAL) {
        logStatus();
        lastStatusLog = millis();
    }
    
    // Let web server handle requests
    configServer.loop();
    
    // Update status LED
    statusLED.loop();
}

void RunLoop::transitionToState(State newState) {
    ESP_LOGI(TAG, "State transition: %s -> %s", 
             stateToString(currentState).c_str(), 
             stateToString(newState).c_str());
    
    // Reset connection attempts when entering CONNECTING state
    if (newState == CONNECTING) {
        connectionAttempts = 0;
        connectionFailed = false;
    }
    
    // Reset consecutive failures on successful connection
    if (newState == CONNECTED) {
        consecutiveFailures = 0;
        connectionAttempts = 0;
    }
    
    currentState = newState;
    stateStartTime = millis();
}

void RunLoop::handleInitializing() {
    ESP_LOGD(TAG, "Initializing storage...");
    statusLED.setPattern(LED_FAST_BLINK);
    
    if (!storage.begin()) {
        ESP_LOGE(TAG, "Failed to initialize storage");
        transitionToState(ERROR);
        return;
    }
    
    ESP_LOGD(TAG, "Storage initialized successfully");
    transitionToState(LOADING_CREDENTIALS);
}

void RunLoop::handleLoadingCredentials() {
    ESP_LOGD(TAG, "Checking for stored credentials...");
    
    if (storage.isConfigured()) {
        ESP_LOGD(TAG, "Found stored credentials");
        transitionToState(CONNECTING);
    } else {
        ESP_LOGI(TAG, "No credentials found - entering AP mode");
        transitionToState(AP_MODE);
    }
}

void RunLoop::handleConnecting() {
    // Feed watchdog to prevent timeout during long connection attempts
    esp_task_wdt_reset();
    
    ESP_LOGI(TAG, "Attempting to connect to WiFi (attempt %d of %d)...", 
             connectionAttempts + 1, WIFI_CONNECTION_ATTEMPTS);
    statusLED.setPattern(LED_FAST_BLINK);
    connectionFailed = false;
    
    // Try to connect (this is blocking but we feed watchdog inside WiFiManager)
    bool connected = wifiManager.connectToStoredNetworks(storage);
    
    // Feed watchdog after connection attempt
    esp_task_wdt_reset();
    
    if (connected) {
        ESP_LOGI(TAG, "✓ Successfully connected to WiFi!");
        connectionAttempts = 0;
        consecutiveFailures = 0;
        connectionFailed = false;
        lastConnectionAttemptTime = millis();
        transitionToState(CONNECTED);
    } else {
        connectionAttempts++;
        consecutiveFailures++;
        
        ESP_LOGE(TAG, "✗ Failed to connect (attempt %d/%d, total failures: %d)", 
                 connectionAttempts, WIFI_CONNECTION_ATTEMPTS, consecutiveFailures);
        
        if (connectionAttempts >= WIFI_CONNECTION_ATTEMPTS) {
            ESP_LOGW(TAG, "Giving up after %d attempts. Entering AP mode.", connectionAttempts);
            connectionFailed = true;
            connectionAttempts = 0;
            lastConnectionAttemptTime = millis();
            transitionToState(AP_MODE);
        } else {
            ESP_LOGI(TAG, "Will retry connection in 2 seconds...");
            // Wait 2 seconds before retry without blocking
            unsigned long retryStart = millis();
            while (millis() - retryStart < 2000) {
                esp_task_wdt_reset();  // Keep watchdog happy
                statusLED.loop();      // Keep LED blinking
                yield();               // Allow other tasks
            }
        }
    }
}

void RunLoop::handleConnected() {
    // Static flag to ensure we only run setup once
    static bool setupComplete = false;
    
    if (!setupComplete) {
        ESP_LOGI(TAG, "WiFi connected, starting services...");
        
        // Stop AP mode if active
        if (wifiManager.isAPMode()) {
            wifiManager.stopAccessPoint();
        }
        
        // Start mDNS
        if (wifiManager.startMDNS()) {
            wifiManager.announceMDNS();
        }
        
        // Start web server
        configServer.setup();
        
        // Set LED to heartbeat pattern (connected)
        statusLED.setPattern(LED_HEARTBEAT);
        
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "EasyWiFi Ready");
        ESP_LOGI(TAG, "Connected to: %s", WiFi.SSID().c_str());
        ESP_LOGI(TAG, "IP Address: %s", wifiManager.getLocalIP().c_str());
        ESP_LOGI(TAG, "Web UI: http://%s/", wifiManager.getLocalIP().c_str());
        
        if (wifiManager.getMDNSHostname().length() > 0) {
            ESP_LOGI(TAG, "mDNS: http://%s.local/", wifiManager.getMDNSHostname().c_str());
        }
        
        ESP_LOGI(TAG, "========================================");
        
        setupComplete = true;
    }
    
    // Monitor connection
    if (!wifiManager.isConnected()) {
        ESP_LOGW(TAG, "WiFi connection lost");
        transitionToState(CONNECTING);
    }
}

void RunLoop::handleAPMode() {
    // Static flag to ensure we only run setup once
    static bool setupComplete = false;
    
    if (!setupComplete) {
        ESP_LOGI(TAG, "Starting Access Point mode...");
        
        if (!wifiManager.startAccessPoint()) {
            ESP_LOGE(TAG, "Failed to start Access Point");
            transitionToState(ERROR);
            return;
        }
        
        // Start captive portal
        ESP_LOGI(TAG, "Starting captive portal...");
        if (wifiManager.startCaptivePortal()) {
            ESP_LOGI(TAG, "Captive portal active - all DNS requests redirected");
        }
        
        ESP_LOGI(TAG, "Starting web server...");
        configServer.setup();
        
        // Set LED to slow blink (AP mode)
        statusLED.setPattern(LED_SLOW_BLINK);
        
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "EasyWiFi - Configuration Mode");
        ESP_LOGI(TAG, "Connect to: %s", wifiManager.getAPSSID().c_str());
        ESP_LOGI(TAG, "Open browser: http://%s/", wifiManager.getLocalIP().c_str());
        ESP_LOGI(TAG, "Captive portal will auto-redirect");
        ESP_LOGI(TAG, "========================================");
        
        setupComplete = true;
    }
    
    // Check if a connection attempt was requested (new credentials saved)
    if (connectionRequested) {
        ESP_LOGI(TAG, "Reconnection requested - transitioning to CONNECTING state");
        connectionRequested = false;
        
        // Stop captive portal and AP before transitioning
        wifiManager.stopCaptivePortal();
        wifiManager.stopAccessPoint();
        
        // Reset setup flag so AP can be restarted if connection fails
        setupComplete = false;
        
        transitionToState(CONNECTING);
        return;
    }
    
    // Handle web server requests
    configServer.loop();
    wifiManager.processCaptivePortal();
}

void RunLoop::handleError() {
    ESP_LOGE(TAG, "Error state - system halted");
    
    // Set LED to double blink (error)
    static bool ledSet = false;
    if (!ledSet) {
        statusLED.setPattern(LED_DOUBLE_BLINK);
        ledSet = true;
    }
    
    // Log error every 10 seconds (non-blocking)
    static unsigned long lastErrorLog = 0;
    if (millis() - lastErrorLog > 10000) {
        ESP_LOGE(TAG, "System in error state. Please reset device.");
        lastErrorLog = millis();
    }
    
    // Keep watchdog happy and allow other tasks
    esp_task_wdt_reset();
    yield();
}

String RunLoop::stateToString(State state) {
    switch (state) {
        case INITIALIZING: return "INITIALIZING";
        case LOADING_CREDENTIALS: return "LOADING_CREDENTIALS";
        case CONNECTING: return "CONNECTING";
        case CONNECTED: return "CONNECTED";
        case AP_MODE: return "AP_MODE";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void RunLoop::logStatus() {
    size_t freeHeap = esp_get_free_heap_size();
    size_t minHeap = esp_get_minimum_free_heap_size();
    
    ESP_LOGI(TAG, "=== Status Report ===");
    ESP_LOGI(TAG, "State: %s | WiFi: %s", 
             stateToString(currentState).c_str(),
             wifiManager.getStatus().c_str());
    ESP_LOGI(TAG, "Memory: %d bytes free (min: %d bytes)", freeHeap, minHeap);
    
    if (wifiManager.isConnected()) {
        ESP_LOGI(TAG, "Connected: %s (RSSI: %d dBm)", WiFi.SSID().c_str(), WiFi.RSSI());
    }
    
    if (wifiManager.isCaptivePortalActive()) {
        ESP_LOGI(TAG, "Captive Portal: Active");
    }
    
    // Warn if memory is running low
    if (freeHeap < MEMORY_WARNING_THRESHOLD) {
        ESP_LOGW(TAG, "!!! LOW MEMORY WARNING: Only %d bytes free !!!", freeHeap);
    }
    
    ESP_LOGI(TAG, "====================");
}

void RunLoop::requestConnectionAttempt() {
    ESP_LOGI(TAG, "Connection attempt requested (new credentials saved)");
    connectionRequested = true;
    consecutiveFailures = 0;  // Reset failure counter for new credentials
}

