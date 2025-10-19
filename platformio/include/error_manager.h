#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

#include <Arduino.h>
#include "localization.h"

// Error severity levels
enum class ErrorLevel {
    INFO = 0,      // Informational message
    WARNING = 1,   // Warning - operation continues
    ERROR = 2,     // Error - operation may fail
    CRITICAL = 3   // Critical - system cannot continue
};

// Icon types for visual representation
enum class ErrorIcon {
    NONE = 0,
    WIFI = 1,
    CALENDAR = 2,
    BATTERY = 3,
    CLOCK = 4,
    NETWORK = 5,
    MEMORY = 6,
    SETTINGS = 7,
    UPDATE = 8,
    GENERAL = 9
};

// Error codes - grouped by category
enum class ErrorCode {
    // Success
    SUCCESS = 0,

    // WiFi errors (100-199)
    WIFI_CONNECTION_FAILED = 100,
    WIFI_DISCONNECTED = 101,
    WIFI_WEAK_SIGNAL = 102,
    WIFI_SSID_NOT_FOUND = 103,
    WIFI_WRONG_PASSWORD = 104,
    WIFI_DHCP_FAILED = 105,
    WIFI_RECONNECTION_FAILED = 106,

    // Calendar errors (200-299)
    CALENDAR_FETCH_FAILED = 200,
    CALENDAR_PARSE_ERROR = 201,
    CALENDAR_AUTH_FAILED = 202,
    CALENDAR_URL_INVALID = 203,
    CALENDAR_TIMEOUT = 204,
    CALENDAR_NO_EVENTS = 205,
    CALENDAR_TOO_MANY_EVENTS = 206,

    // Time sync errors (300-399)
    NTP_SYNC_FAILED = 300,
    NTP_SERVER_UNREACHABLE = 301,
    TIME_NOT_SET = 302,
    TIMEZONE_ERROR = 303,

    // Display errors (400-499)
    DISPLAY_INIT_FAILED = 400,
    DISPLAY_UPDATE_FAILED = 401,
    DISPLAY_BUSY_TIMEOUT = 402,

    // Battery errors (500-599)
    BATTERY_LOW = 500,
    BATTERY_CRITICAL = 501,
    BATTERY_MONITOR_FAILED = 502,

    // Memory errors (600-699)
    MEMORY_LOW = 600,
    MEMORY_ALLOCATION_FAILED = 601,

    // Network errors (700-799)
    NETWORK_TIMEOUT = 700,
    NETWORK_DNS_FAILED = 701,
    NETWORK_SSL_FAILED = 702,
    HTTP_ERROR = 703,

    // Configuration errors (800-899)
    CONFIG_MISSING = 800,
    CONFIG_INVALID = 801,
    CONFIG_WIFI_NOT_SET = 802,
    CONFIG_CALENDAR_NOT_SET = 803,

    // OTA errors (900-999)
    OTA_UPDATE_AVAILABLE = 900,
    OTA_UPDATE_FAILED = 901,
    OTA_DOWNLOAD_FAILED = 902,
    OTA_VERIFICATION_FAILED = 903,

    // System errors (1000+)
    SYSTEM_RESTART_REQUIRED = 1000,
    SYSTEM_UNKNOWN_ERROR = 1001
};

// Error information structure
struct ErrorInfo {
    ErrorCode code;
    ErrorLevel level;
    ErrorIcon icon;
    String message;         // Localized message
    String details;         // Additional details (optional)
    unsigned long timestamp;
    bool recoverable;       // Can the system recover from this error
    int retryCount;         // Number of retries attempted
    int maxRetries;         // Maximum retries allowed
};

class ErrorManager {
private:
    static ErrorInfo currentError;
    static ErrorInfo lastError;
    static bool hasError;
    static unsigned long errorStartTime;
    static const unsigned long ERROR_DISPLAY_MIN_TIME = 3000; // Minimum time to display error (ms)

    // Get error message from code
    static String getErrorMessage(ErrorCode code);
    static ErrorLevel getErrorLevel(ErrorCode code);
    static ErrorIcon getErrorIcon(ErrorCode code);
    static bool isRecoverable(ErrorCode code);
    static int getMaxRetries(ErrorCode code);

public:
    // Set an error
    static void setError(ErrorCode code, const String& details = "");

    // Clear current error
    static void clearError();

    // Check if there's an active error
    static bool hasActiveError();

    // Get current error info
    static ErrorInfo getCurrentError();

    // Get last error (for logging)
    static ErrorInfo getLastError();

    // Check if error is critical (system should stop)
    static bool isCritical();

    // Check if should retry
    static bool shouldRetry();

    // Increment retry count
    static void incrementRetry();

    // Reset retry count
    static void resetRetry();

    // Check if minimum display time has passed
    static bool canClearError();

    // Get error description for logging
    static String getErrorDescription(ErrorCode code);

    // Log error to serial
    static void logError(const ErrorInfo& error);

    // Check if specific error is active
    static bool isErrorActive(ErrorCode code);
};

// Helper macros
#define SET_ERROR(code) ErrorManager::setError(code)
#define SET_ERROR_WITH_DETAILS(code, details) ErrorManager::setError(code, details)
#define CLEAR_ERROR() ErrorManager::clearError()
#define HAS_ERROR() ErrorManager::hasActiveError()
#define IS_CRITICAL_ERROR() ErrorManager::isCritical()

#endif // ERROR_MANAGER_H