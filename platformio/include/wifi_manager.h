#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

class WiFiManager {
private:
    WiFiClientSecure* client;
    unsigned long lastConnectionAttempt;
    const unsigned long connectionTimeout = 20000; // 20 seconds

public:
    WiFiManager();
    ~WiFiManager();
    bool connect();
    bool isConnected();
    void disconnect();
    WiFiClientSecure* getClient();
    String getIPAddress();
    int getRSSI();
    void printStatus();
};

#endif // WIFI_MANAGER_H