#ifndef VERSION_H
#define VERSION_H

// Semantic Versioning 2.0.0
// MAJOR.MINOR.PATCH
// MAJOR: incompatible API changes
// MINOR: backwards-compatible functionality additions
// PATCH: backwards-compatible bug fixes

#define VERSION_MAJOR 1
#define VERSION_MINOR 10
#define VERSION_PATCH 1

// Build metadata (optional)
#define VERSION_BUILD __DATE__ " " __TIME__

// Version string macros
#define VERSION_STRING_HELPER(major, minor, patch) #major "." #minor "." #patch
#define VERSION_STRING(major, minor, patch)        VERSION_STRING_HELPER(major, minor, patch)

// Full version string
#define VERSION      VERSION_STRING(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)
#define VERSION_FULL VERSION " (" VERSION_BUILD ")"

// Project information
#define PROJECT_NAME   "ESP32 E-Paper Calendar"
#define PROJECT_AUTHOR "Alessandro Crugnola"
#define PROJECT_URL    "https://github.com/sephiroth74/esp32-calendar"

// OTA Update configuration
#define OTA_UPDATE_ENABLED false // Set to true when OTA is implemented
#define OTA_UPDATE_URL     ""    // URL for checking updates
#define OTA_CHECK_INTERVAL 86400 // Check for updates every 24 hours (in seconds)

// Version check for OTA updates
#define VERSION_NUMERIC ((VERSION_MAJOR * 10000) + (VERSION_MINOR * 100) + VERSION_PATCH)

// Feature flags for this version
#define FEATURE_SPLIT_SCREEN        1
#define FEATURE_LOCALIZATION        1
#define FEATURE_DEEP_SLEEP          1
#define FEATURE_BATTERY_MONITOR     1
#define FEATURE_ICS_CALENDAR        1
#define FEATURE_GOOGLE_CALENDAR     1
#define FEATURE_CALDAV              0 // Not fully implemented yet
#define FEATURE_WEATHER             1 // Weather display support
#define FEATURE_BUTTON_WAKEUP       1 // Manual refresh button
#define FEATURE_6COLOR_DISPLAY      1 // Support for 6-color e-paper displays
#define FEATURE_COLOR_CUSTOMIZATION 1 // Configurable color scheme
#define FEATURE_ONCE_DAILY_UPDATE   1 // Calendar updates once per day
#define FEATURE_SMART_ERROR_RETRY   1 // Intelligent retry intervals based on error type
#define FEATURE_LITTLEFS_CONFIG     1 // Configuration stored in LittleFS filesystem
#define FEATURE_BATTERY_SLEEP       1 // No wake on critical battery
#define FEATURE_MULTIPLE_CALENDARS  1 // Support for fetching from multiple calendar sources
#define FEATURE_LOCAL_ICS_FILES     1 // Support for local ICS files in LittleFS
#define FEATURE_RECURRING_EVENTS    1 // Support for RRULE recurring events (birthdays, anniversaries)
#define FEATURE_DUAL_ORIENTATION                                                                   \
    1 // Landscape and portrait display orientations with compile-time optimization

// Display version info function
inline String getVersionString() { return String(VERSION); }

inline String getFullVersionString() { return String(VERSION_FULL); }

inline String getProjectInfo() { return String(PROJECT_NAME) + " v" + String(VERSION); }

inline uint32_t getVersionNumeric() { return VERSION_NUMERIC; }

#endif // VERSION_H