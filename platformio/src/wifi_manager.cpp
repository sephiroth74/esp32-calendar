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

    // This WiFiManager is deprecated - configuration now comes from LittleFS
    Serial.println("WARNING: WiFiManager::connect() called but this class is deprecated");
    Serial.println("WiFi configuration should be loaded from LittleFS config.json");

    // Return false as we don't have credentials
    return false;
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