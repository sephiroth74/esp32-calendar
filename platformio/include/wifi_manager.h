#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include "littlefs_config.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

// Forward declaration
class LittleFSConfig;

class WiFiManager {
  private:
    WiFiClientSecure* client;
    unsigned long lastConnectionAttempt;
    const unsigned long connectionTimeout = 20000; // 20 seconds
    bool timeConfigured;

  public:
    WiFiManager();
    ~WiFiManager();
    bool connect(const RuntimeConfig& config);
    bool isConnected();
    void disconnect();
    WiFiClientSecure* getClient();
    String getIPAddress();
    int getRSSI();
    void printStatus();

    /**
     * Sync time from NTP servers
     * @param timezone Timezone string (e.g., "PST8PDT,M3.2.0,M11.1.0")
     * @param ntpServer1 Primary NTP server (default: "pool.ntp.org")
     * @param ntpServer2 Secondary NTP server (default: "time.nist.gov")
     * @return true if time sync successful
     */
    bool syncTimeFromNTP(const String& timezone,
                         const char* ntpServer1 = "pool.ntp.org",
                         const char* ntpServer2 = "time.nist.gov");

    /**
     * Check if time has been synchronized
     * @return true if time has been configured via NTP
     */
    bool isTimeConfigured() const { return timeConfigured; }
};

#endif // WIFI_MANAGER_H