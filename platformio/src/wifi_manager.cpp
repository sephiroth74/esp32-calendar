#include "wifi_manager.h"

WiFiManager::WiFiManager() : client(nullptr), lastConnectionAttempt(0) {
    client = new WiFiClientSecure();
}

WiFiManager::~WiFiManager() {
    if (client) {
        delete client;
    }
}

bool WiFiManager::connect() {
    if (isConnected()) {
        return true;
    }

    Serial.println("Connecting to WiFi...");
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    lastConnectionAttempt = millis();

    while (!isConnected()) {
        if (millis() - lastConnectionAttempt > connectionTimeout) {
            Serial.println("\nWiFi connection timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected!");
    printStatus();

    return true;
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