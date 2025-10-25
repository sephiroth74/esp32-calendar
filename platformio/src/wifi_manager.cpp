#include "wifi_manager.h"
#include "littlefs_config.h"
#include "debug_config.h"

WiFiManager::WiFiManager() : client(nullptr), lastConnectionAttempt(0), timeConfigured(false) {
    client = new WiFiClientSecure();
}

WiFiManager::~WiFiManager() {
    if (client) {
        delete client;
    }
}

bool WiFiManager::connect(const RuntimeConfig& config) {
    if (isConnected()) {
        return true;
    }

    // Get WiFi credentials from the passed config
    String ssid = config.wifi_ssid;
    String password = config.wifi_password;

    if (ssid.isEmpty()) {
        Serial.println("ERROR: WiFi SSID not configured in config.json");
        return false;
    }

    Serial.println("Connecting to WiFi: " + ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection with timeout
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.println("IP address: " + WiFi.localIP().toString());
        Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");

        // Configure secure client
        if (client) {
            client->setInsecure(); // Skip certificate validation for simplicity
        }

        return true;
    } else {
        Serial.println("\nWiFi connection failed!");
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disconnected");
}

WiFiClientSecure* WiFiManager::getClient()
{
    return client;
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return -100;
}

void WiFiManager::printStatus() {
    Serial.println("WiFi Status:");
    Serial.print("  IP Address: ");
    Serial.println(getIPAddress());
    Serial.print("  RSSI: ");
    Serial.print(getRSSI());
    Serial.println(" dBm");
    Serial.print("  MAC Address: ");
    Serial.println(WiFi.macAddress());
}

bool WiFiManager::syncTimeFromNTP(const String& timezone, const char* ntpServer1, const char* ntpServer2) {
    DEBUG_INFO_PRINTLN("Configuring time with NTP...");

    // Check WiFi connection first
    if (!isConnected()) {
        DEBUG_ERROR_PRINTLN("ERROR: WiFi not connected, cannot sync time");
        return false;
    }

    // First, configure NTP with UTC to get basic time sync
    DEBUG_INFO_PRINTLN("Initiating NTP sync with servers: " + String(ntpServer1) + ", " + String(ntpServer2));
    configTime(0, 0, ntpServer1, ntpServer2);

    // Wait for time to be set
    int retry = 0;
    const int retry_count = 40;  // Increased timeout to 20 seconds
    time_t now = time(nullptr);

    DEBUG_INFO_PRINT("Waiting for NTP sync");
    while (now < 8 * 3600 * 365 && retry < retry_count) {  // Check for a reasonable year (> 1970)
        delay(500);
        now = time(nullptr);
        retry++;
        if (retry % 4 == 0) {  // Print every 2 seconds
            DEBUG_INFO_PRINT(" (" + String(retry/2) + "s)");
        }
    }
    DEBUG_INFO_PRINTLN("");

    if (now < 8 * 3600 * 365) {
        DEBUG_ERROR_PRINTLN("Failed to sync time from NTP after " + String(retry_count/2) + " seconds");
        DEBUG_ERROR_PRINTLN("Current time value: " + String(now));
        timeConfigured = false;
        return false;
    }

    // Now set the timezone after time is synced
    DEBUG_INFO_PRINTLN("NTP sync successful, setting timezone to: " + timezone);
    setenv("TZ", timezone.c_str(), 1);
    tzset();

    // Re-read time after timezone is set
    now = time(nullptr);

    // Get both UTC and local time for debugging
    struct tm timeinfo_buf;
    struct tm utcinfo_buf;
    struct tm* timeinfo = localtime_r(&now, &timeinfo_buf);
    struct tm* utcinfo = gmtime_r(&now, &utcinfo_buf);

    char local_time_str[64];
    char utc_time_str[64];
    strftime(local_time_str, sizeof(local_time_str), "%Y-%m-%d %H:%M:%S %Z", timeinfo);
    strftime(utc_time_str, sizeof(utc_time_str), "%Y-%m-%d %H:%M:%S UTC", utcinfo);

    DEBUG_INFO_PRINTLN("Time synchronized!");
    DEBUG_INFO_PRINTLN("  Raw timestamp: " + String(now));
    DEBUG_INFO_PRINTLN("  UTC time: " + String(utc_time_str));
    DEBUG_INFO_PRINTLN("  Local time: " + String(local_time_str));
    DEBUG_INFO_PRINTLN("  Timezone: " + timezone);
    DEBUG_INFO_PRINTLN("  Year: " + String(timeinfo->tm_year + 1900));
    DEBUG_INFO_PRINTLN("  Month: " + String(timeinfo->tm_mon + 1));
    DEBUG_INFO_PRINTLN("  Day: " + String(timeinfo->tm_mday));

    // Calculate and display the offset
    int hour_diff = timeinfo->tm_hour - utcinfo->tm_hour;
    int day_diff = timeinfo->tm_mday - utcinfo->tm_mday;
    if (day_diff != 0) {
        hour_diff += day_diff * 24;
    }
    DEBUG_INFO_PRINTLN("  UTC offset: " + String(hour_diff) + " hours");

    timeConfigured = true;
    return true;
}