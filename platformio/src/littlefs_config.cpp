#include "littlefs_config.h"
#include "config.h"

LittleFSConfig::LittleFSConfig() {
    // Initialize with default values from config.h
    config.latitude = LOC_LATITUDE;
    config.longitude = LOC_LONGITUDE;
    config.timezone = DEFAULT_TIMEZONE;
    config.update_hour = DEFAULT_UPDATE_HOUR;
    config.valid = false;

    // Initialize with default calendar for backward compatibility
    CalendarConfig defaultCal;
    defaultCal.name = "Default Calendar";
    defaultCal.url = DEFAULT_CALENDAR_URL;
    defaultCal.color = "default";
    defaultCal.enabled = true;
    defaultCal.days_to_fetch = DEFAULT_DAYS_TO_FETCH;
    defaultCal.holiday_calendar = false;
    config.calendars.push_back(defaultCal);
}

bool LittleFSConfig::begin() {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {  // Format if mount fails
        Serial.println("LittleFS mount failed - formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("LittleFS mount failed after format!");
            return false;
        }
    }

    Serial.println("LittleFS mounted successfully");

    // Print filesystem info
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    Serial.println("LittleFS Info:");
    Serial.println("  Total: " + String(totalBytes / 1024.0, 2) + " KB");
    Serial.println("  Used: " + String(usedBytes / 1024.0, 2) + " KB");
    Serial.println("  Free: " + String((totalBytes - usedBytes) / 1024.0, 2) + " KB");

    return true;
}

bool LittleFSConfig::loadConfiguration() {
    // Check if config file exists
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Config file does not exist: " + String(CONFIG_FILE));
        return false;
    }

    // Open config file
    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        Serial.println("Failed to open config file for reading");
        return false;
    }

    // Parse JSON
    size_t size = configFile.size();
    Serial.println("Config file size: " + String(size) + " bytes");

    if (size > 10240) {  // Increased limit to 10KB
        Serial.println("Config file too large!");
        configFile.close();
        return false;
    }

    // Read file content
    String jsonStr = configFile.readString();
    configFile.close();

    // Debug: Show first 200 chars of JSON
    Serial.println("JSON content preview: " + jsonStr.substring(0, 200) + "...");

    // Use DynamicJsonDocument for better flexibility with nested structures
    DynamicJsonDocument doc(8192);  // 8KB should be enough for complex configs
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
        Serial.println("Failed to parse config file: " + String(error.c_str()));
        Serial.println("Error code: " + String(error.code()));

        // Detailed error reporting
        if (error == DeserializationError::NoMemory) {
            Serial.println("JSON document buffer too small! Increase DynamicJsonDocument size.");
        } else if (error == DeserializationError::InvalidInput) {
            Serial.println("Invalid JSON syntax in config file.");
        }
        return false;
    }

    Serial.println("JSON parsed successfully!");
    Serial.println("JSON document memory usage: " + String(doc.memoryUsage()) + " bytes");

    // Extract WiFi settings
    if (doc.containsKey("wifi")) {
        Serial.println("Found 'wifi' section");
        JsonObject wifi = doc["wifi"];
        config.wifi_ssid = wifi["ssid"] | String("");
        config.wifi_password = wifi["password"] | String("");
        Serial.println("  SSID extracted: '" + config.wifi_ssid + "'");
        Serial.println("  Password extracted: " + String(config.wifi_password.isEmpty() ? "(empty)" : "(set)"));
    } else {
        Serial.println("WARNING: 'wifi' section not found in JSON!");
    }

    // Extract location settings
    if (doc.containsKey("location")) {
        Serial.println("Found 'location' section");
        JsonObject location = doc["location"];
        config.latitude = location["latitude"] | LOC_LATITUDE;
        config.longitude = location["longitude"] | LOC_LONGITUDE;
        config.location_name = location["name"] | String("Unknown");
        Serial.println("  Latitude: " + String(config.latitude, 6));
        Serial.println("  Longitude: " + String(config.longitude, 6));
        Serial.println("  Name: " + config.location_name);
    } else {
        Serial.println("WARNING: 'location' section not found in JSON!");
    }

    // Extract calendar settings
    config.calendars.clear(); // Clear default calendars

    // Check for new format (calendars array) - limit to MAX_CALENDARS
    if (doc.containsKey("calendars")) {
        Serial.println("Found 'calendars' section");

        if (doc["calendars"].is<JsonArray>()) {
            JsonArray calendars = doc["calendars"];
            Serial.println("  Calendars array size: " + String(calendars.size()));

            // Validate calendar count
            if (calendars.size() > MAX_CALENDARS) {
                Serial.println("  ERROR: Configuration contains " + String(calendars.size()) +
                             " calendars, but maximum allowed is " + String(MAX_CALENDARS));
                Serial.println("  WARNING: Only the first " + String(MAX_CALENDARS) +
                             " calendars will be loaded. Please update your config.json!");
            }

            int calendarCount = 0;
            for (JsonVariant cal : calendars) {
                if (calendarCount >= MAX_CALENDARS) {
                    Serial.println("  Skipping calendar '" + String(cal["name"] | "unnamed") +
                                 "' (exceeds MAX_CALENDARS limit)");
                    break;
                }

                CalendarConfig calConfig;
                calConfig.name = cal["name"] | String("Unnamed Calendar");
                calConfig.url = cal["url"] | String("");
                calConfig.color = cal["color"] | String("default");
                calConfig.enabled = cal["enabled"] | true;
                calConfig.days_to_fetch = cal["days_to_fetch"] | DEFAULT_DAYS_TO_FETCH;
                calConfig.holiday_calendar = cal["holiday_calendar"] | false;

                Serial.println("  Calendar " + String(calendarCount + 1) + ":");
                Serial.println("    Name: " + calConfig.name);
                Serial.println("    URL: " + (calConfig.url.isEmpty() ? "(empty)" : calConfig.url.substring(0, 50) + "..."));
                Serial.println("    Color: " + calConfig.color);
                Serial.println("    Enabled: " + String(calConfig.enabled));
                Serial.println("    Days to fetch: " + String(calConfig.days_to_fetch));
                Serial.println("    Holiday calendar: " + String(calConfig.holiday_calendar ? "yes" : "no"));

                if (!calConfig.url.isEmpty()) {
                    config.calendars.push_back(calConfig);
                    calendarCount++;
                } else {
                    Serial.println("    WARNING: Calendar has empty URL, skipping");
                }
            }
            Serial.println("  Total calendars loaded: " + String(calendarCount));
        } else {
            Serial.println("  ERROR: 'calendars' is not an array!");
        }
    } else {
        Serial.println("WARNING: 'calendars' section not found in JSON!");
        // Backward compatibility: check for old single calendar format
        if (doc.containsKey("calendar")) {
            Serial.println("Found legacy 'calendar' section (single calendar)");
            JsonObject calendar = doc["calendar"];
            CalendarConfig calConfig;
            calConfig.name = "Primary Calendar";
            calConfig.url = calendar["url"] | String(DEFAULT_CALENDAR_URL);
            calConfig.color = "default";
            calConfig.enabled = true;
            calConfig.days_to_fetch = calendar["days_to_fetch"] | DEFAULT_DAYS_TO_FETCH;

            if (!calConfig.url.isEmpty()) {
                config.calendars.push_back(calConfig);
                Serial.println("  Legacy calendar loaded: " + calConfig.name);
            }
        }
    }

    // Ensure we have at least one calendar
    if (config.calendars.empty()) {
        CalendarConfig defaultCal;
        defaultCal.name = "Default Calendar";
        defaultCal.url = DEFAULT_CALENDAR_URL;
        defaultCal.color = "default";
        defaultCal.enabled = true;
        defaultCal.days_to_fetch = DEFAULT_DAYS_TO_FETCH;
        config.calendars.push_back(defaultCal);
    }

    // Extract display settings
    if (doc.containsKey("display")) {
        Serial.println("Found 'display' section");
        JsonObject display = doc["display"];
        config.timezone = display["timezone"] | String(DEFAULT_TIMEZONE);
        config.update_hour = display["update_hour"] | DEFAULT_UPDATE_HOUR;
        Serial.println("  Timezone: " + config.timezone);
        Serial.println("  Update hour: " + String(config.update_hour));
    } else {
        Serial.println("WARNING: 'display' section not found in JSON!");
        config.timezone = DEFAULT_TIMEZONE;
        config.update_hour = DEFAULT_UPDATE_HOUR;
    }

    // Mark configuration as valid if we have at least WiFi credentials
    config.valid = !config.wifi_ssid.isEmpty() && !config.wifi_password.isEmpty();

    if (config.valid) {
        Serial.println("Configuration loaded successfully from LittleFS");
        printConfiguration();
    } else {
        Serial.println("Configuration loaded but missing required fields");
    }

    return config.valid;
}

bool LittleFSConfig::saveConfiguration() {
    // Create JSON document - use DynamicJsonDocument for consistency
    DynamicJsonDocument doc(2048);  // 2KB should be sufficient for saving

    // Add WiFi settings
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = config.wifi_ssid;
    wifi["password"] = config.wifi_password;

    // Add location settings
    JsonObject location = doc.createNestedObject("location");
    location["latitude"] = config.latitude;
    location["longitude"] = config.longitude;
    location["name"] = config.location_name;

    // Add calendars array
    JsonArray calendars = doc.createNestedArray("calendars");
    for (const auto& cal : config.calendars) {
        JsonObject calObj = calendars.createNestedObject();
        calObj["name"] = cal.name;
        calObj["url"] = cal.url;
        calObj["color"] = cal.color;
        calObj["enabled"] = cal.enabled;
        calObj["days_to_fetch"] = cal.days_to_fetch;
    }

    // Add display settings
    JsonObject display = doc.createNestedObject("display");
    display["timezone"] = config.timezone;
    display["update_hour"] = config.update_hour;

    // Open file for writing
    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    // Serialize JSON to file
    size_t bytesWritten = serializeJsonPretty(doc, configFile);
    configFile.close();

    if (bytesWritten == 0) {
        Serial.println("Failed to write configuration");
        return false;
    }

    Serial.println("Configuration saved (" + String(bytesWritten) + " bytes)");
    return true;
}

void LittleFSConfig::resetConfiguration() {
    // Delete config file
    if (LittleFS.exists(CONFIG_FILE)) {
        LittleFS.remove(CONFIG_FILE);
        Serial.println("Configuration file deleted");
    }

    // Reset to defaults
    config = RuntimeConfig();
    config.latitude = LOC_LATITUDE;
    config.longitude = LOC_LONGITUDE;
    config.timezone = DEFAULT_TIMEZONE;
    config.update_hour = DEFAULT_UPDATE_HOUR;
    config.valid = false;

    // Add default calendar
    CalendarConfig defaultCal;
    defaultCal.name = "Default Calendar";
    defaultCal.url = DEFAULT_CALENDAR_URL;
    defaultCal.color = "default";
    defaultCal.enabled = true;
    defaultCal.days_to_fetch = DEFAULT_DAYS_TO_FETCH;
    config.calendars.push_back(defaultCal);

    Serial.println("Configuration reset to defaults");
}

void LittleFSConfig::setWiFiCredentials(const String& ssid, const String& password) {
    config.wifi_ssid = ssid;
    config.wifi_password = password;
    config.valid = !ssid.isEmpty() && !password.isEmpty();
}

void LittleFSConfig::setLocation(float lat, float lon, const String& name) {
    config.latitude = lat;
    config.longitude = lon;
    if (!name.isEmpty()) {
        config.location_name = name;
    }
}

void LittleFSConfig::setCalendarUrl(const String& url) {
    // Backward compatibility - sets first calendar URL
    if (config.calendars.empty()) {
        CalendarConfig cal;
        cal.name = "Primary Calendar";
        cal.url = url;
        cal.color = "default";
        cal.enabled = true;
        config.calendars.push_back(cal);
    } else {
        config.calendars[0].url = url;
    }
}

void LittleFSConfig::addCalendar(const CalendarConfig& calendar) {
    if (config.calendars.size() >= MAX_CALENDARS) {
        Serial.println("Error: Maximum " + String(MAX_CALENDARS) + " calendars allowed");
        return;
    }
    config.calendars.push_back(calendar);
}

void LittleFSConfig::removeCalendar(int index) {
    if (index >= 0 && index < config.calendars.size()) {
        config.calendars.erase(config.calendars.begin() + index);
    }
}

void LittleFSConfig::clearCalendars() {
    config.calendars.clear();
}

void LittleFSConfig::printConfiguration() {
    Serial.println("\n=== Current Configuration ===");
    Serial.println("WiFi:");
    Serial.println("  SSID: " + config.wifi_ssid);
    Serial.println("  Password: " + String(config.wifi_password.isEmpty() ? "[not set]" : "********"));

    Serial.println("Location:");
    Serial.println("  Name: " + config.location_name);
    Serial.println("  Latitude: " + String(config.latitude, 6));
    Serial.println("  Longitude: " + String(config.longitude, 6));

    Serial.println("Calendars: " + String(config.calendars.size()));
    for (size_t i = 0; i < config.calendars.size(); i++) {
        Serial.println("  [" + String(i) + "] " + config.calendars[i].name);
        Serial.println("      URL: " + config.calendars[i].url);
        Serial.println("      Color: " + config.calendars[i].color);
        Serial.println("      Enabled: " + String(config.calendars[i].enabled ? "Yes" : "No"));
        Serial.println("      Days to fetch: " + String(config.calendars[i].days_to_fetch));
    }

    Serial.println("Display:");
    Serial.println("  Timezone: " + config.timezone);
    Serial.println("  Update hour: " + String(config.update_hour));

    Serial.println("Status: " + String(config.valid ? "VALID" : "INVALID"));
    Serial.println("============================\n");
}