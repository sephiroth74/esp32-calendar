#ifndef MOCK_CALENDAR_FETCHER_H
#define MOCK_CALENDAR_FETCHER_H

#include "mock_arduino.h"

/**
 * Mock CalendarFetcher for native testing
 * Provides minimal stub implementation to satisfy CalendarStreamParser
 * Does not actually fetch from network or files
 */
class CalendarFetcher {
private:
    bool debug;

public:
    CalendarFetcher() : debug(false) {}
    ~CalendarFetcher() {}

    // Configuration
    void setTimeout(int timeoutMs) { (void)timeoutMs; }
    void setDebug(bool enable) { debug = enable; }

    // Stream-based fetching - returns nullptr in mock
    Stream* fetchStream(const String& url) {
        (void)url;
        return nullptr;
    }

    void endStream() {
        // No-op in mock
    }
};

#endif // MOCK_CALENDAR_FETCHER_H
