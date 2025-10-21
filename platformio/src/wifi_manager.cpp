#include "wifi_manager.h"
#include "littlefs_config.h"

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
    Serial.println("Configuring time with NTP...");

    // Check WiFi connection first
    if (!isConnected()) {
        Serial.println("ERROR: WiFi not connected, cannot sync time");
        return false;
    }

    // Set timezone
    setenv("TZ", timezone.c_str(), 1);
    tzset();

    // Configure NTP servers
    configTime(0, 0, ntpServer1, ntpServer2);

    // Wait for time to be set
    int retry = 0;
    const int retry_count = 20;
    time_t now = time(nullptr);

    Serial.print("Waiting for NTP sync");
    while (now < 24 * 3600 && retry < retry_count) {
        delay(500);
        now = time(nullptr);
        retry++;
        Serial.print(".");
    }
    Serial.println();

    if (now < 24 * 3600) {
        Serial.println("Failed to sync time from NTP");
        timeConfigured = false;
        return false;
    }

    struct tm* timeinfo = localtime(&now);
    Serial.println("Time synchronized: " + String(asctime(timeinfo)));

    // Print timezone info
    Serial.println("Timezone: " + timezone);

    timeConfigured = true;
    return true;
}