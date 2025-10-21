#ifndef CALENDAR_WRAPPER_H
#define CALENDAR_WRAPPER_H

#include <Arduino.h>
#include "ics_parser.h"
#include "calendar_fetcher.h"
#include "littlefs_config.h"
#include <vector>

// CalendarWrapper combines ICS parser with calendar configuration
// This class manages a single calendar with its configuration properties
class CalendarWrapper {
private:
    ICSParser parser;
    CalendarConfig config;
    CalendarFetcher fetcher;

    String cachedFilename;
    time_t lastFetchTime;
    bool loaded;
    bool debug;

    // Calculate cache filename for this calendar
    String getCacheFilename() const;

    // Check if cache is still valid
    bool isCacheValid() const;

public:
    CalendarWrapper();
    ~CalendarWrapper();

    // Configuration
    void setConfig(const CalendarConfig& calConfig) { config = calConfig; }
    void setDebug(bool enable) { debug = enable; fetcher.setDebug(enable); parser.setDebug(enable); }

    // Get configuration properties
    const String& getName() const { return config.name; }
    const String& getUrl() const { return config.url; }
    const String& getColor() const { return config.color; }
    bool isEnabled() const { return config.enabled; }
    int getDaysToFetch() const { return config.days_to_fetch; }

    // Load calendar data (from network or cache)
    bool load(bool forceRefresh = false);

    // Get events within date range (respects days_to_fetch)
    std::vector<CalendarEvent*> getEvents(time_t startDate, time_t endDate);

    // Get all events (for the configured days_to_fetch period)
    std::vector<CalendarEvent*> getAllEvents();

    // Status
    bool isLoaded() const { return loaded; }
    size_t getEventCount() const { return parser.getEventCount(); }
    String getLastError() const { return parser.getLastError(); }

    // Memory info
    void printMemoryInfo() const { parser.printMemoryInfo(); }
};

// CalendarManager manages multiple calendars
class CalendarManager {
private:
    std::vector<CalendarWrapper*> calendars;
    bool debug;

public:
    CalendarManager();
    ~CalendarManager();

    // Configuration
    void setDebug(bool enable) { debug = enable; }

    // Add calendars from runtime config
    bool loadFromConfig(const RuntimeConfig& config);

    // Clear all calendars
    void clear();

    // Load all enabled calendars
    bool loadAll(bool forceRefresh = false);

    // Get merged events from all enabled calendars
    std::vector<CalendarEvent*> getAllEvents(time_t startDate, time_t endDate);

    // Get specific calendar
    CalendarWrapper* getCalendar(size_t index);
    size_t getCalendarCount() const { return calendars.size(); }

    // Status
    size_t getTotalEventCount() const;
    void printStatus() const;
};

#endif // CALENDAR_WRAPPER_H