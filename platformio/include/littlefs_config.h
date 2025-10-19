#ifndef LITTLEFS_CONFIG_H
#define LITTLEFS_CONFIG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct RuntimeConfig {
    // WiFi settings
    String wifi_ssid;
    String wifi_password;

    // Location settings
    float latitude;
    float longitude;
    String location_name;

    // Calendar settings
    String calendar_url;
    int days_to_fetch;

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
    String getCalendarUrl() const { return config.calendar_url; }
    int getDaysToFetch() const { return config.days_to_fetch; }
    String getTimezone() const { return config.timezone; }
    int getUpdateHour() const { return config.update_hour; }

    // Setters for runtime updates
    void setWiFiCredentials(const String& ssid, const String& password);
    void setLocation(float lat, float lon, const String& name = "");
    void setCalendarUrl(const String& url);

    // Debug helper
    void printConfiguration();
};

#endif // LITTLEFS_CONFIG_H