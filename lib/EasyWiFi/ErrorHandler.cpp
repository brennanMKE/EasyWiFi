#include "ErrorHandler.h"

ErrorHandler::ErrorHandler(const char* name) 
    : moduleName(name), maxConsecutiveErrors(5), lastLogTime(0) {
}

bool ErrorHandler::shouldRetry(uint32_t errorCode, int currentAttempt) {
    if (currentAttempt >= retryPolicy.maxRetries) {
        return false;
    }
    
    // Don't retry certain fatal errors
    if (errorCode == (uint32_t)StorageError::INVALID_SSID ||
        errorCode == (uint32_t)WiFiError::AUTH_FAILED) {
        return false;
    }
    
    return true;
}

unsigned long ErrorHandler::getRetryDelay(int attemptNumber) {
    unsigned long delay = retryPolicy.initialDelayMs;
    for (int i = 0; i < attemptNumber; i++) {
        delay *= retryPolicy.backoffMultiplier;
        if (delay > retryPolicy.maxDelayMs) {
            delay = retryPolicy.maxDelayMs;
            break;
        }
    }
    return delay;
}

void ErrorHandler::recordError(uint32_t errorCode, const char* message) {
    ErrorContext ctx;
    ctx.code = errorCode;
    ctx.message = message;
    ctx.module = moduleName;
    ctx.timestamp = millis();
    
    stats.totalErrors++;
    stats.consecutiveErrors++;
    stats.lastErrorTime = ctx.timestamp;
    stats.lastError = ctx;
    
    // Log error if not rate-limited
    if (shouldLogError()) {
        logError(ctx);
        lastLogTime = millis();
    }
}

void ErrorHandler::recordRecovery() {
    stats.recoveredErrors++;
    stats.consecutiveErrors = 0;
    ESP_LOGI(moduleName, "Error recovery successful (total recovered: %d)", stats.recoveredErrors);
}

void ErrorHandler::resetErrorCount() {
    stats.consecutiveErrors = 0;
}

bool ErrorHandler::isHealthy() const {
    return stats.consecutiveErrors < maxConsecutiveErrors;
}

void ErrorHandler::setRetryPolicy(const RetryPolicy& policy) {
    retryPolicy = policy;
}

const char* ErrorHandler::getErrorMessage(WiFiError error) {
    switch (error) {
        case WiFiError::SUCCESS: return "Success";
        case WiFiError::CONNECT_FAILED: return "Connection failed";
        case WiFiError::CONNECT_TIMEOUT: return "Connection timeout";
        case WiFiError::AUTH_FAILED: return "Authentication failed";
        case WiFiError::SSID_NOT_FOUND: return "SSID not found";
        case WiFiError::WEAK_SIGNAL: return "Signal too weak";
        case WiFiError::AP_START_FAILED: return "Failed to start AP";
        case WiFiError::AP_ALREADY_ACTIVE: return "AP already active";
        case WiFiError::DNS_START_FAILED: return "Failed to start DNS server";
        case WiFiError::MDNS_START_FAILED: return "Failed to start mDNS";
        case WiFiError::NETWORK_UNREACHABLE: return "Network unreachable";
        default: return "Unknown error";
    }
}

const char* ErrorHandler::getErrorMessage(StorageError error) {
    switch (error) {
        case StorageError::SUCCESS: return "Success";
        case StorageError::NVS_INIT_FAILED: return "NVS initialization failed";
        case StorageError::NVS_WRITE_FAILED: return "NVS write failed";
        case StorageError::NVS_READ_FAILED: return "NVS read failed";
        case StorageError::INVALID_SSID: return "Invalid SSID";
        case StorageError::INVALID_PASSWORD: return "Invalid password";
        case StorageError::STORAGE_FULL: return "Storage full";
        case StorageError::CREDENTIALS_NOT_FOUND: return "Credentials not found";
        default: return "Unknown error";
    }
}

const char* ErrorHandler::getErrorMessage(ServerError error) {
    switch (error) {
        case ServerError::SUCCESS: return "Success";
        case ServerError::SERVER_START_FAILED: return "Server start failed";
        case ServerError::INVALID_REQUEST: return "Invalid request";
        case ServerError::MISSING_PARAMETER: return "Missing parameter";
        case ServerError::VALIDATION_FAILED: return "Validation failed";
        default: return "Unknown error";
    }
}

void ErrorHandler::logError(const ErrorContext& ctx) {
    ESP_LOGE(moduleName, "[ERROR #%d] %s (consecutive: %d, total: %d)",
             ctx.code, ctx.message, stats.consecutiveErrors, stats.totalErrors);
}

bool ErrorHandler::shouldLogError() {
    // Rate limit error logs to prevent spam
    unsigned long now = millis();
    if (lastLogTime == 0) {
        return true;  // First error, always log
    }
    
    // Log if enough time has passed (defined in Macros.h or default 60s)
    return (now - lastLogTime) > 60000;
}

