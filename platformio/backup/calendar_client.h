#ifndef CALENDAR_CLIENT_H
#define CALENDAR_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include "config.h"
#include "display_manager.h"
#include "littlefs_config.h"

// PSRAM allocation helpers
#define PSRAM_AVAILABLE (ESP.getPsramSize() > 0)
#define PSRAM_ALLOC(size) (PSRAM_AVAILABLE ? ps_malloc(size) : malloc(size))
#define PSRAM_FREE(ptr) free(ptr)

class CalendarClient {
private:
    WiFiClientSecure* wifiClient;
    HTTPClient httpClient;

    // ICS parsing helpers
    String extractICSValue(const String& line, const String& key);
    CalendarEvent* parseICSEvent(const String& eventData);
    std::vector<CalendarEvent*> parseICSCalendar(const String& icsData, int daysAhead);
    time_t parseICSDateTime(const String& dtString, bool& isUTC);

    // Google Calendar helpers
    std::vector<CalendarEvent*> parseGoogleCalendarResponse(const String& jsonResponse);

    // Common helpers
    String urlEncode(const String& str);

public:
    CalendarClient(WiFiClientSecure* client);
    ~CalendarClient();

    // Public for testing
    String convertUTCToLocalTime(const String& dtString);

    // Main fetch method - deprecated, use fetchICSEvents directly
    std::vector<CalendarEvent*> fetchEvents(int daysAhead = DEFAULT_DAYS_TO_FETCH);

    // Specific calendar source methods
    std::vector<CalendarEvent*> fetchICSEvents(const String& url, int daysAhead);
    std::vector<CalendarEvent*> fetchLocalICSEvents(const String& filepath, int daysAhead);
    std::vector<CalendarEvent*> fetchGoogleCalendarEvents(int daysAhead);
    std::vector<CalendarEvent*> fetchCalDAVEvents(int daysAhead);

    // Multiple calendar support
    std::vector<CalendarEvent*> fetchICSEvents(const String& url, int daysAhead,
                                               const String& calendarName,
                                               const String& calendarColor);
    std::vector<CalendarEvent*> fetchMultipleCalendars(const std::vector<CalendarConfig>& calendars);

    // Utility methods
    void sortEventsByTime(std::vector<CalendarEvent*>& events);
    void filterPastEvents(std::vector<CalendarEvent*>& events);
    void limitEvents(std::vector<CalendarEvent*>& events, int maxEvents);

    // Memory management
    static void freeEvents(std::vector<CalendarEvent*>& events);
};

#endif // CALENDAR_CLIENT_H