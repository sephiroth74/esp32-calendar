#include "littlefs_config.h"
#include "config.h"

LittleFSConfig::LittleFSConfig() {
    // Initialize with default values from config.h
    config.latitude = LOC_LATITUDE;
    config.longitude = LOC_LONGITUDE;
    config.calendar_url = DEFAULT_CALENDAR_URL;
    config.days_to_fetch = DEFAULT_DAYS_TO_FETCH;
    config.timezone = DEFAULT_TIMEZONE;
    config.update_hour = DEFAULT_UPDATE_HOUR;
    config.valid = false;
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

    if (size > 2048) {
        Serial.println("Config file too large!");
        configFile.close();
        return false;
    }

    // Read file content
    String jsonStr = configFile.readString();
    configFile.close();

    // Parse JSON document
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
        Serial.println("Failed to parse config file: " + String(error.c_str()));
        return false;
    }

    // Extract WiFi settings
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        config.wifi_ssid = wifi["ssid"] | String("");
        config.wifi_password = wifi["password"] | String("");
    }

    // Extract location settings
    if (doc.containsKey("location")) {
        JsonObject location = doc["location"];
        config.latitude = location["latitude"] | LOC_LATITUDE;
        config.longitude = location["longitude"] | LOC_LONGITUDE;
        config.location_name = location["name"] | String("Unknown");
    }

    // Extract calendar settings
    if (doc.containsKey("calendar")) {
        JsonObject calendar = doc["calendar"];
        config.calendar_url = calendar["url"] | String(DEFAULT_CALENDAR_URL);
        config.days_to_fetch = calendar["days_to_fetch"] | DEFAULT_DAYS_TO_FETCH;
    }

    // Extract display settings
    if (doc.containsKey("display")) {
        JsonObject display = doc["display"];
        config.timezone = display["timezone"] | String(DEFAULT_TIMEZONE);
        config.update_hour = display["update_hour"] | DEFAULT_UPDATE_HOUR;
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
    // Create JSON document
    StaticJsonDocument<1024> doc;

    // Add WiFi settings
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = config.wifi_ssid;
    wifi["password"] = config.wifi_password;

    // Add location settings
    JsonObject location = doc.createNestedObject("location");
    location["latitude"] = config.latitude;
    location["longitude"] = config.longitude;
    location["name"] = config.location_name;

    // Add calendar settings
    JsonObject calendar = doc.createNestedObject("calendar");
    calendar["url"] = config.calendar_url;
    calendar["days_to_fetch"] = config.days_to_fetch;

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
    config.calendar_url = DEFAULT_CALENDAR_URL;
    config.days_to_fetch = DEFAULT_DAYS_TO_FETCH;
    config.timezone = DEFAULT_TIMEZONE;
    config.update_hour = DEFAULT_UPDATE_HOUR;
    config.valid = false;

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
    config.calendar_url = url;
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

    Serial.println("Calendar:");
    Serial.println("  URL: " + config.calendar_url);
    Serial.println("  Days to fetch: " + String(config.days_to_fetch));

    Serial.println("Display:");
    Serial.println("  Timezone: " + config.timezone);
    Serial.println("  Update hour: " + String(config.update_hour));

    Serial.println("Status: " + String(config.valid ? "VALID" : "INVALID"));
    Serial.println("============================\n");
}