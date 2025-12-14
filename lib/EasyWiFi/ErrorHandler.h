#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#if defined(ESP32)
#include "Arduino.h"
#include <esp_log.h>
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// Error codes for each module
enum class WiFiError {
    SUCCESS = 0,
    CONNECT_FAILED,
    CONNECT_TIMEOUT,
    AUTH_FAILED,
    SSID_NOT_FOUND,
    WEAK_SIGNAL,
    AP_START_FAILED,
    AP_ALREADY_ACTIVE,
    DNS_START_FAILED,
    MDNS_START_FAILED,
    NETWORK_UNREACHABLE
};

enum class StorageError {
    SUCCESS = 0,
    NVS_INIT_FAILED,
    NVS_WRITE_FAILED,
    NVS_READ_FAILED,
    INVALID_SSID,
    INVALID_PASSWORD,
    STORAGE_FULL,
    CREDENTIALS_NOT_FOUND
};

enum class ServerError {
    SUCCESS = 0,
    SERVER_START_FAILED,
    INVALID_REQUEST,
    MISSING_PARAMETER,
    VALIDATION_FAILED
};

// Error context with details
struct ErrorContext {
    uint32_t code;
    const char* message;
    const char* module;
    unsigned long timestamp;
    int retryCount;
    
    ErrorContext() : code(0), message(""), module(""), timestamp(0), retryCount(0) {}
};

// Retry policy configuration
struct RetryPolicy {
    int maxRetries;
    unsigned long initialDelayMs;
    float backoffMultiplier;
    unsigned long maxDelayMs;
    
    RetryPolicy() : maxRetries(3), initialDelayMs(1000), 
                    backoffMultiplier(2.0f), maxDelayMs(30000) {}
};

// Error statistics for monitoring
struct ErrorStats {
    uint32_t totalErrors;
    uint32_t connectionErrors;
    uint32_t storageErrors;
    uint32_t serverErrors;
    uint32_t recoveredErrors;
    uint32_t consecutiveErrors;
    unsigned long lastErrorTime;
    ErrorContext lastError;
    
    ErrorStats() : totalErrors(0), connectionErrors(0), storageErrors(0),
                   serverErrors(0), recoveredErrors(0), consecutiveErrors(0),
                   lastErrorTime(0) {}
};

class ErrorHandler {
public:
    ErrorHandler(const char* moduleName);
    
    // Record error and apply retry logic
    bool shouldRetry(uint32_t errorCode, int currentAttempt);
    unsigned long getRetryDelay(int attemptNumber);
    void recordError(uint32_t errorCode, const char* message);
    void recordRecovery();
    void resetErrorCount();
    
    // Error to string conversion
    const char* getErrorMessage(WiFiError error);
    const char* getErrorMessage(StorageError error);
    const char* getErrorMessage(ServerError error);
    
    // Error statistics
    const ErrorStats& getStats() const { return stats; }
    bool isHealthy() const;
    
    // Configuration
    void setRetryPolicy(const RetryPolicy& policy);
    void setMaxConsecutiveErrors(int max) { maxConsecutiveErrors = max; }
    
private:
    const char* moduleName;
    ErrorStats stats;
    RetryPolicy retryPolicy;
    int maxConsecutiveErrors;
    unsigned long lastLogTime;
    
    void logError(const ErrorContext& ctx);
    bool shouldLogError();
};

#endif // ERRORHANDLER_H

