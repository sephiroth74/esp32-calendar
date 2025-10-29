#ifndef CALENDAR_FETCHER_H
#define CALENDAR_FETCHER_H

#ifdef NATIVE_TEST
#include "mock_arduino.h"
#else
#include <Arduino.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFiClient.h>
#endif

// Result structure for fetch operations
struct FetchResult {
    bool success;
    String data;
    String error;
    int httpCode;
    size_t dataSize;

    FetchResult() : success(false), httpCode(0), dataSize(0) {}
};

class CalendarFetcher {
private:
    HTTPClient http;
    WiFiClient* streamClient;
    File fileStream;
    int timeout;
    bool debug;

    // Helper functions
    bool isLocalUrl(const String& url) const;
    String getLocalPath(const String& url) const;
    FetchResult fetchFromHttp(const String& url);
    FetchResult fetchFromFile(const String& path);

public:
    CalendarFetcher();
    ~CalendarFetcher();

    // Configuration
    void setTimeout(int timeoutMs) { timeout = timeoutMs; }
    void setDebug(bool enable) { debug = enable; }

    // Main fetch function - automatically determines local vs remote
    FetchResult fetch(const String& url);

    // Stream-based fetching for large files
    Stream* fetchStream(const String& url);
    void endStream();

    // Utility functions
    static String getFilenameFromUrl(const String& url);
    static bool cacheToFile(const String& data, const String& filename);
    static String loadFromCache(const String& filename);
    static bool isCacheValid(const String& filename, unsigned long maxAgeSeconds);
};

#endif // CALENDAR_FETCHER_H