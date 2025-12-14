#ifndef RUNLOOP_H
#define RUNLOOP_H

#if defined(ESP32)
#include "Arduino.h"
#include <esp_log.h>
#include "WiFiManager.h"
#include "ConfigServer.h"
#include "Storage.h"
#include "StatusLED.h"
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

class RunLoop {
public:
    RunLoop();
    void setup();
    void loop();

private:
    Storage storage;
    WiFiManager wifiManager;
    ConfigServer configServer;
    StatusLED statusLED;
    
    enum State {
        INITIALIZING,
        LOADING_CREDENTIALS,
        CONNECTING,
        CONNECTED,
        AP_MODE,
        ERROR
    };
    
    State currentState;
    unsigned long stateStartTime;
    unsigned long lastStatusLog;
    int connectionAttempts;
    bool connectionFailed;
    int consecutiveFailures;
    unsigned long lastConnectionAttemptTime;
    ErrorContext lastWiFiError;
    unsigned long errorStateEnteredTime;
    int errorRecoveryAttempts;
    unsigned long lastErrorHandleTime;  // For rate limiting error handling to prevent tight loops
    
    enum RecoveryStrategy {
        RETRY_CONNECTION,
        REDUCE_WIFI_POWER,
        FORCE_BG_MODE,
        FALLBACK_TO_AP,
        FACTORY_RESET
    };
    
    RecoveryStrategy determineRecoveryStrategy();
    bool attemptRecovery(RecoveryStrategy strategy);
    
    void transitionToState(State newState);
    void handleInitializing();
    void handleLoadingCredentials();
    void handleConnecting();
    void handleConnected();
    void handleAPMode();
    void handleError();
    
    String stateToString(State state);
    void logStatus();
    
    bool connectionRequested;
    
public:
    bool hasConnectionFailed() { return connectionFailed; }
    void requestConnectionAttempt();
};

#endif // RUNLOOP_H
