#include "error_manager.h"
#include "localization.h"

// Static member definitions
ErrorInfo ErrorManager::currentError = {ErrorCode::SUCCESS, ErrorLevel::INFO, ErrorIcon::NONE, "", "", 0, true, 0, 0};
ErrorInfo ErrorManager::lastError = {ErrorCode::SUCCESS, ErrorLevel::INFO, ErrorIcon::NONE, "", "", 0, true, 0, 0};
bool ErrorManager::hasError = false;
unsigned long ErrorManager::errorStartTime = 0;

void ErrorManager::setError(ErrorCode code, const String& details) {
    // Save last error before overwriting
    if (hasError) {
        lastError = currentError;
    }

    currentError.code = code;
    currentError.level = getErrorLevel(code);
    currentError.icon = getErrorIcon(code);
    currentError.message = getErrorMessage(code);
    currentError.details = details;
    currentError.timestamp = millis();
    currentError.recoverable = isRecoverable(code);
    currentError.retryCount = 0;
    currentError.maxRetries = getMaxRetries(code);

    hasError = (code != ErrorCode::SUCCESS);
    errorStartTime = millis();

    // Log the error
    logError(currentError);
}

void ErrorManager::clearError() {
    if (hasError) {
        lastError = currentError;
    }
    hasError = false;
    currentError.code = ErrorCode::SUCCESS;
    currentError.level = ErrorLevel::INFO;
    currentError.icon = ErrorIcon::NONE;
    currentError.message = "";
    currentError.details = "";
    currentError.retryCount = 0;
}

bool ErrorManager::hasActiveError() {
    return hasError;
}

ErrorInfo ErrorManager::getCurrentError() {
    return currentError;
}

ErrorInfo ErrorManager::getLastError() {
    return lastError;
}

bool ErrorManager::isCritical() {
    return hasError && (currentError.level == ErrorLevel::CRITICAL);
}

bool ErrorManager::shouldRetry() {
    return hasError &&
           currentError.recoverable &&
           (currentError.retryCount < currentError.maxRetries);
}

void ErrorManager::incrementRetry() {
    if (hasError) {
        currentError.retryCount++;
    }
}

void ErrorManager::resetRetry() {
    if (hasError) {
        currentError.retryCount = 0;
    }
}

bool ErrorManager::canClearError() {
    return !hasError || (millis() - errorStartTime >= ERROR_DISPLAY_MIN_TIME);
}

String ErrorManager::getErrorMessage(ErrorCode code) {
    switch (code) {
        // WiFi errors
        case ErrorCode::WIFI_CONNECTION_FAILED: return LOC_ERROR_WIFI_CONNECTION_FAILED;
        case ErrorCode::WIFI_DISCONNECTED: return LOC_ERROR_WIFI_DISCONNECTED;
        case ErrorCode::WIFI_WEAK_SIGNAL: return LOC_ERROR_WIFI_WEAK_SIGNAL;
        case ErrorCode::WIFI_SSID_NOT_FOUND: return LOC_ERROR_WIFI_SSID_NOT_FOUND;
        case ErrorCode::WIFI_WRONG_PASSWORD: return LOC_ERROR_WIFI_WRONG_PASSWORD;
        case ErrorCode::WIFI_DHCP_FAILED: return LOC_ERROR_WIFI_DHCP_FAILED;
        case ErrorCode::WIFI_RECONNECTION_FAILED: return LOC_ERROR_WIFI_RECONNECTION_FAILED;

        // Calendar errors
        case ErrorCode::CALENDAR_FETCH_FAILED: return LOC_ERROR_CALENDAR_FETCH_FAILED;
        case ErrorCode::CALENDAR_PARSE_ERROR: return LOC_ERROR_CALENDAR_PARSE_ERROR;
        case ErrorCode::CALENDAR_AUTH_FAILED: return LOC_ERROR_CALENDAR_AUTH_FAILED;
        case ErrorCode::CALENDAR_URL_INVALID: return LOC_ERROR_CALENDAR_URL_INVALID;
        case ErrorCode::CALENDAR_TIMEOUT: return LOC_ERROR_CALENDAR_TIMEOUT;
        case ErrorCode::CALENDAR_NO_EVENTS: return LOC_ERROR_CALENDAR_NO_EVENTS;
        case ErrorCode::CALENDAR_TOO_MANY_EVENTS: return LOC_ERROR_CALENDAR_TOO_MANY_EVENTS;

        // Time sync errors
        case ErrorCode::NTP_SYNC_FAILED: return LOC_ERROR_NTP_SYNC_FAILED;
        case ErrorCode::NTP_SERVER_UNREACHABLE: return LOC_ERROR_NTP_SERVER_UNREACHABLE;
        case ErrorCode::TIME_NOT_SET: return LOC_ERROR_TIME_NOT_SET;
        case ErrorCode::TIMEZONE_ERROR: return LOC_ERROR_TIMEZONE_ERROR;

        // Display errors
        case ErrorCode::DISPLAY_INIT_FAILED: return LOC_ERROR_DISPLAY_INIT_FAILED;
        case ErrorCode::DISPLAY_UPDATE_FAILED: return LOC_ERROR_DISPLAY_UPDATE_FAILED;
        case ErrorCode::DISPLAY_BUSY_TIMEOUT: return LOC_ERROR_DISPLAY_BUSY_TIMEOUT;

        // Battery errors
        case ErrorCode::BATTERY_LOW: return LOC_ERROR_BATTERY_LOW;
        case ErrorCode::BATTERY_CRITICAL: return LOC_ERROR_BATTERY_CRITICAL;
        case ErrorCode::BATTERY_MONITOR_FAILED: return LOC_ERROR_BATTERY_MONITOR_FAILED;

        // Memory errors
        case ErrorCode::MEMORY_LOW: return LOC_ERROR_MEMORY_LOW;
        case ErrorCode::MEMORY_ALLOCATION_FAILED: return LOC_ERROR_MEMORY_ALLOCATION_FAILED;

        // Network errors
        case ErrorCode::NETWORK_TIMEOUT: return LOC_ERROR_NETWORK_TIMEOUT;
        case ErrorCode::NETWORK_DNS_FAILED: return LOC_ERROR_NETWORK_DNS_FAILED;
        case ErrorCode::NETWORK_SSL_FAILED: return LOC_ERROR_NETWORK_SSL_FAILED;
        case ErrorCode::HTTP_ERROR: return LOC_ERROR_HTTP_ERROR;

        // Configuration errors
        case ErrorCode::CONFIG_MISSING: return LOC_ERROR_CONFIG_MISSING;
        case ErrorCode::CONFIG_INVALID: return LOC_ERROR_CONFIG_INVALID;
        case ErrorCode::CONFIG_WIFI_NOT_SET: return LOC_ERROR_CONFIG_WIFI_NOT_SET;
        case ErrorCode::CONFIG_CALENDAR_NOT_SET: return LOC_ERROR_CONFIG_CALENDAR_NOT_SET;

        // OTA errors
        case ErrorCode::OTA_UPDATE_AVAILABLE: return LOC_ERROR_OTA_UPDATE_AVAILABLE;
        case ErrorCode::OTA_UPDATE_FAILED: return LOC_ERROR_OTA_UPDATE_FAILED;
        case ErrorCode::OTA_DOWNLOAD_FAILED: return LOC_ERROR_OTA_DOWNLOAD_FAILED;
        case ErrorCode::OTA_VERIFICATION_FAILED: return LOC_ERROR_OTA_VERIFICATION_FAILED;

        // System errors
        case ErrorCode::SYSTEM_RESTART_REQUIRED: return LOC_ERROR_SYSTEM_RESTART_REQUIRED;
        case ErrorCode::SYSTEM_UNKNOWN_ERROR: return LOC_ERROR_SYSTEM_UNKNOWN_ERROR;

        default: return LOC_ERROR_SYSTEM_UNKNOWN_ERROR;
    }
}

ErrorLevel ErrorManager::getErrorLevel(ErrorCode code) {
    switch ((int)code) {
        // Info level
        case (int)ErrorCode::CALENDAR_NO_EVENTS:
        case (int)ErrorCode::OTA_UPDATE_AVAILABLE:
            return ErrorLevel::INFO;

        // Warning level
        case (int)ErrorCode::WIFI_WEAK_SIGNAL:
        case (int)ErrorCode::BATTERY_LOW:
        case (int)ErrorCode::MEMORY_LOW:
        case (int)ErrorCode::CALENDAR_TOO_MANY_EVENTS:
            return ErrorLevel::WARNING;

        // Critical level
        case (int)ErrorCode::DISPLAY_INIT_FAILED:
        case (int)ErrorCode::BATTERY_CRITICAL:
        case (int)ErrorCode::MEMORY_ALLOCATION_FAILED:
        case (int)ErrorCode::CONFIG_MISSING:
        case (int)ErrorCode::CONFIG_INVALID:
        case (int)ErrorCode::CONFIG_WIFI_NOT_SET:
        case (int)ErrorCode::CONFIG_CALENDAR_NOT_SET:
            return ErrorLevel::CRITICAL;

        // Error level (default for most errors)
        default:
            return ErrorLevel::ERROR;
    }
}

ErrorIcon ErrorManager::getErrorIcon(ErrorCode code) {
    if ((int)code >= 100 && (int)code < 200) return ErrorIcon::WIFI;
    if ((int)code >= 200 && (int)code < 300) return ErrorIcon::CALENDAR;
    if ((int)code >= 300 && (int)code < 400) return ErrorIcon::CLOCK;
    if ((int)code >= 400 && (int)code < 500) return ErrorIcon::GENERAL;
    if ((int)code >= 500 && (int)code < 600) return ErrorIcon::BATTERY;
    if ((int)code >= 600 && (int)code < 700) return ErrorIcon::MEMORY;
    if ((int)code >= 700 && (int)code < 800) return ErrorIcon::NETWORK;
    if ((int)code >= 800 && (int)code < 900) return ErrorIcon::SETTINGS;
    if ((int)code >= 900 && (int)code < 1000) return ErrorIcon::UPDATE;
    return ErrorIcon::GENERAL;
}

bool ErrorManager::isRecoverable(ErrorCode code) {
    switch (code) {
        case ErrorCode::DISPLAY_INIT_FAILED:
        case ErrorCode::BATTERY_CRITICAL:
        case ErrorCode::MEMORY_ALLOCATION_FAILED:
        case ErrorCode::CONFIG_MISSING:
        case ErrorCode::CONFIG_INVALID:
        case ErrorCode::CONFIG_WIFI_NOT_SET:
        case ErrorCode::CONFIG_CALENDAR_NOT_SET:
            return false;
        default:
            return true;
    }
}

int ErrorManager::getMaxRetries(ErrorCode code) {
    switch (code) {
        case ErrorCode::WIFI_CONNECTION_FAILED:
        case ErrorCode::WIFI_RECONNECTION_FAILED:
        case ErrorCode::NTP_SYNC_FAILED:
            return 5;

        case ErrorCode::CALENDAR_FETCH_FAILED:
        case ErrorCode::CALENDAR_TIMEOUT:
        case ErrorCode::NETWORK_TIMEOUT:
        case ErrorCode::HTTP_ERROR:
            return 3;

        default:
            return 0;
    }
}

String ErrorManager::getErrorDescription(ErrorCode code) {
    String desc = "Error " + String((int)code) + ": ";
    desc += getErrorMessage(code);
    return desc;
}

void ErrorManager::logError(const ErrorInfo& error) {
    if (error.code == ErrorCode::SUCCESS) return;

    Serial.print("[ERROR] ");

    switch (error.level) {
        case ErrorLevel::INFO: Serial.print("INFO: "); break;
        case ErrorLevel::WARNING: Serial.print("WARNING: "); break;
        case ErrorLevel::ERROR: Serial.print("ERROR: "); break;
        case ErrorLevel::CRITICAL: Serial.print("CRITICAL: "); break;
    }

    Serial.print(getErrorDescription(error.code));

    if (error.details.length() > 0) {
        Serial.print(" - ");
        Serial.print(error.details);
    }

    if (error.retryCount > 0) {
        Serial.print(" (Retry ");
        Serial.print(error.retryCount);
        Serial.print("/");
        Serial.print(error.maxRetries);
        Serial.print(")");
    }

    Serial.println();
}

bool ErrorManager::isErrorActive(ErrorCode code) {
    return hasError && (currentError.code == code);
}