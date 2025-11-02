#ifndef LITTLEFS_CONFIG_H
#define LITTLEFS_CONFIG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"

// Structure for individual calendar configuration
struct CalendarConfig {
    String name;              // Display name for the calendar
    String url;               // ICS URL
    String color;             // Optional: color code for this calendar's events
    bool enabled;             // Whether this calendar is active
    int days_to_fetch;        // How many days ahead to fetch for this calendar
    bool holiday_calendar;    // True if full-day events from this calendar are holidays
};

struct RuntimeConfig {
    // WiFi settings
    String wifi_ssid;
    String wifi_password;

    // Location settings
    float latitude;
    float longitude;
    String location_name;

    // Calendar settings - now supports multiple calendars
    std::vector<CalendarConfig> calendars;

    // Display settings
    String timezone;
    int update_hour;

    // Configuration status
    bool valid = false;
};

class LittleFSConfig {
private:
    RuntimeConfig config;
    const char* CONFIG_FILE = "/config.json";

public:
    LittleFSConfig();
    bool begin();
    bool loadConfiguration();
    bool saveConfiguration();
    void resetConfiguration();

    // Getters for configuration values
    const RuntimeConfig& getConfig() const { return config; }
    bool isValid() const { return config.valid; }

    // Individual getters for convenience
    String getWiFiSSID() const { return config.wifi_ssid; }
    String getWiFiPassword() const { return config.wifi_password; }
    float getLatitude() const { return config.latitude; }
    float getLongitude() const { return config.longitude; }
    String getLocationName() const { return config.location_name; }

    // Calendar getters - now returns vector of calendars
    const std::vector<CalendarConfig>& getCalendars() const { return config.calendars; }
    String getCalendarUrl() const {
        // Backward compatibility - return first calendar URL if exists
        return config.calendars.empty() ? "" : config.calendars[0].url;
    }

    // getDaysToFetch removed - days_to_fetch is now per-calendar in CalendarConfig

    String getTimezone() const { return config.timezone; }
    int getUpdateHour() const { return config.update_hour; }

    // Setters for runtime updates
    void setWiFiCredentials(const String& ssid, const String& password);
    void setLocation(float lat, float lon, const String& name = "");

    // Calendar management
    void addCalendar(const CalendarConfig& calendar);
    void removeCalendar(int index);
    void clearCalendars();
    void setCalendarUrl(const String& url); // Backward compatibility - sets first calendar

    // Debug helper
    void printConfiguration();
};

#endif // LITTLEFS_CONFIG_H