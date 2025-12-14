#include "RunLoop.h"
#include <esp_task_wdt.h>
#include <esp_wifi.h>

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
    connectionRequested(false),
    errorStateEnteredTime(0),
    errorRecoveryAttempts(0),
    lastErrorHandleTime(0) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
}

void RunLoop::setup(const String& name) {
    // Store device name for use throughout the system
    deviceName = name;
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "%s Starting", deviceName.c_str());
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
    
    // Wire up RunLoop reference and device name to ConfigServer
    configServer.setRunLoop(this);
    configServer.setDeviceName(deviceName);
    
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
    
    // Try to connect and capture error context for better error handling
    ErrorContext errorContext;
    WiFiError result = wifiManager.connectToStoredNetworksEx(storage, &errorContext);
    bool connected = (result == WiFiError::SUCCESS);
    
    // Store error context for use in error recovery
    if (!connected) {
        lastWiFiError = errorContext;
    }
    
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
            ESP_LOGW(TAG, "Giving up after %d attempts.", connectionAttempts);
            connectionFailed = true;
            connectionAttempts = 0;
            lastConnectionAttemptTime = millis();
            
            // Check if we should try recovery or go straight to AP mode
            if (consecutiveFailures >= MAX_CONSECUTIVE_ERRORS_BEFORE_AP) {
                ESP_LOGE(TAG, "Too many consecutive failures - entering AP mode");
                transitionToState(AP_MODE);
            } else {
                ESP_LOGI(TAG, "Entering error recovery mode");
                transitionToState(ERROR);
            }
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
        
        // Start mDNS with custom device name
        if (wifiManager.startMDNS(deviceName)) {
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
        
        if (!wifiManager.startAccessPoint(deviceName)) {
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
        ESP_LOGI(TAG, "%s - Configuration Mode", deviceName.c_str());
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
    // Rate limiting to prevent tight loops - CRITICAL FIX
    unsigned long now = millis();
    if (now - lastErrorHandleTime < ERROR_HANDLE_RATE_LIMIT_MS) {
        esp_task_wdt_reset();
        yield();
        return;
    }
    
    // First time entering error state?
    static bool errorStateInitialized = false;
    if (!errorStateInitialized) {
        errorStateEnteredTime = millis();
        errorRecoveryAttempts = 0;
        errorStateInitialized = true;
        statusLED.setPattern(LED_DOUBLE_BLINK);
        ESP_LOGI(TAG, "Entering error recovery mode");
    }
    
    // Only log state changes, not every iteration (prevents log spam)
    static int lastLoggedAttempt = -1;
    if (lastLoggedAttempt != errorRecoveryAttempts) {
        ESP_LOGD(TAG, "Error state - attempt %d", errorRecoveryAttempts);
        lastLoggedAttempt = errorRecoveryAttempts;
    }
    
    // Check for unrecoverable error: All networks not found (NO_AP_FOUND)
    // If FORCE_AP_ON_NO_NETWORKS is enabled and this is SSID_NOT_FOUND, transition to AP mode
    if (FORCE_AP_ON_NO_NETWORKS && 
        lastWiFiError.code == (uint32_t)WiFiError::SSID_NOT_FOUND) {
        if (errorRecoveryAttempts >= MAX_CONSECUTIVE_ERRORS_BEFORE_AP) {
            ESP_LOGW(TAG, "All configured networks not found (NO_AP_FOUND) - transitioning to AP mode");
            ESP_LOGI(TAG, "User can connect to AP to reconfigure networks");
            transitionToState(AP_MODE);
            errorStateInitialized = false;
            lastErrorHandleTime = now;
            return;
        }
    }
    
    // Give up after MAX_RECOVERY_ATTEMPTS or ERROR_RECOVERY_TIMEOUT
    if (errorRecoveryAttempts >= MAX_RECOVERY_ATTEMPTS || 
        (millis() - errorStateEnteredTime) > ERROR_RECOVERY_TIMEOUT) {
        ESP_LOGE(TAG, "Max recovery attempts reached or timeout - entering safe mode (AP)");
        transitionToState(AP_MODE);
        errorStateInitialized = false;
        lastErrorHandleTime = now;
        return;
    }
    
    // Wait between recovery attempts (exponential backoff)
    static unsigned long lastRecoveryAttempt = 0;
    unsigned long backoffDelay = RETRY_BACKOFF_BASE_MS * (1 << errorRecoveryAttempts);  // 5s, 10s, 20s, 40s, 80s
    if (backoffDelay > 60000) backoffDelay = 60000;  // Cap at 60 seconds
    
    if (millis() - lastRecoveryAttempt < backoffDelay) {
        esp_task_wdt_reset();
        yield();
        return;
    }
    
    lastRecoveryAttempt = millis();
    lastErrorHandleTime = now;
    errorRecoveryAttempts++;
    
    // Determine and apply recovery strategy
    RecoveryStrategy strategy = determineRecoveryStrategy();
    ESP_LOGI(TAG, "Attempting recovery strategy: %d (attempt %d/%d)", 
             strategy, errorRecoveryAttempts, MAX_RECOVERY_ATTEMPTS);
    
    if (attemptRecovery(strategy)) {
        ESP_LOGI(TAG, "Recovery successful!");
        errorStateInitialized = false;
        lastLoggedAttempt = -1;  // Reset log throttle
        transitionToState(LOADING_CREDENTIALS);
    } else {
        ESP_LOGW(TAG, "Recovery failed, will retry with backoff");
    }
    
    esp_task_wdt_reset();
    yield();
}

RunLoop::RecoveryStrategy RunLoop::determineRecoveryStrategy() {
    // Analyze error history to pick best strategy
    const ErrorStats& stats = wifiManager.getErrorStats();
    
    if (errorRecoveryAttempts == 0) {
        return RETRY_CONNECTION;  // Simple retry first
    } else if (lastWiFiError.code == (uint32_t)WiFiError::WEAK_SIGNAL) {
        return REDUCE_WIFI_POWER;  // Try lower power for eero
    } else if (lastWiFiError.code == (uint32_t)WiFiError::CONNECT_TIMEOUT) {
        return FORCE_BG_MODE;  // Force 802.11b/g
    } else if (errorRecoveryAttempts < 4) {
        return RETRY_CONNECTION;  // Keep trying
    } else {
        return FALLBACK_TO_AP;  // Give up, let user fix it
    }
}

bool RunLoop::attemptRecovery(RecoveryStrategy strategy) {
    switch (strategy) {
        case RETRY_CONNECTION:
            ESP_LOGI(TAG, "Recovery: Retry connection");
            return wifiManager.connectToStoredNetworks(storage);
            
        case REDUCE_WIFI_POWER:
            ESP_LOGI(TAG, "Recovery: Reduce WiFi power");
            WiFi.setTxPower(WIFI_POWER_8_5dBm);
            return wifiManager.connectToStoredNetworks(storage);
            
        case FORCE_BG_MODE:
            ESP_LOGI(TAG, "Recovery: Force 802.11b/g mode");
            esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
            return wifiManager.connectToStoredNetworks(storage);
            
        case FALLBACK_TO_AP:
            ESP_LOGI(TAG, "Recovery: Fallback to AP mode");
            transitionToState(AP_MODE);
            return true;
            
        default:
            return false;
    }
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
