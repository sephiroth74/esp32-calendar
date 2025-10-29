/**
 * Memory-efficient calendar parser that streams and filters events on-demand
 * Does not store events - only processes them for the requested date range
 */

#ifndef CALENDAR_STREAM_PARSER_H
#define CALENDAR_STREAM_PARSER_H

#ifdef NATIVE_TEST
#include "mock_arduino.h"
#else
#include <Arduino.h>
#endif
#include <vector>
#include "calendar_event.h"

// Forward declarations
class Stream;
class CalendarFetcher;

// Callback function for processing events as they're parsed
typedef std::function<void(CalendarEvent*)> EventCallback;

// Result structure for filtered events
struct FilteredEvents {
    std::vector<CalendarEvent*> events;
    size_t totalParsed;       // Total events parsed from stream
    size_t totalFiltered;     // Events that matched the filter
    bool success;
    String error;

    FilteredEvents() : totalParsed(0), totalFiltered(0), success(true) {}

    ~FilteredEvents() {
        // Clean up allocated events
        for (auto event : events) {
            if (event) {
                delete event;
            }
        }
        events.clear();
    }
};

class CalendarStreamParser {
public:
    CalendarStreamParser();
    ~CalendarStreamParser();

    /**
     * Parse and filter events from a URL or file for a specific date range
     * Only events within the range are returned, nothing is stored permanently
     *
     * @param url The calendar URL or file path
     * @param startDate Start of date range (Unix timestamp)
     * @param endDate End of date range (Unix timestamp)
     * @param maxEvents Maximum number of events to return (0 = no limit)
     * @return FilteredEvents containing only events in the specified range
     */
    FilteredEvents* fetchEventsInRange(const String& url,
                                       time_t startDate,
                                       time_t endDate,
                                       size_t maxEvents = 100,
                                       const String& cachePath = "");

    /**
     * Stream parse with callback - for custom processing without storing events
     * Useful for counting, statistics, or custom filtering
     *
     * @param url The calendar URL or file path
     * @param callback Function called for each parsed event
     * @param startDate Optional start date filter (0 = no filter)
     * @param endDate Optional end date filter (0 = no filter)
     * @return true if parsing succeeded
     */
    bool streamParse(const String& url,
                     EventCallback callback,
                     time_t startDate = 0,
                     time_t endDate = 0,
                     const String& cachePath = "");

    /**
     * Parse calendar metadata without loading events
     * Useful for getting calendar name, timezone, etc.
     *
     * @param url The calendar URL or file path
     * @param calendarName Output: calendar name
     * @param timezone Output: calendar timezone
     * @return true if metadata was successfully parsed
     */
    bool parseMetadata(const String& url,
                      String& calendarName,
                      String& timezone);

    // Configuration
    void setDebug(bool enable) { debug = enable; }
    void setCalendarColor(uint16_t color) { calendarColor = color; }
    void setCalendarName(const String& name) { calendarName = name; }

private:
    bool debug;
    uint16_t calendarColor;
    String calendarName;
    CalendarFetcher* fetcher;

    // Parsing state
    enum ParseState {
        LOOKING_FOR_CALENDAR,
        IN_HEADER,
        IN_EVENT,
        DONE
    };

    // Parse a single event from buffer
    CalendarEvent* parseEventFromBuffer(const String& eventData);

    // Check if event is within date range
    bool isEventInRange(CalendarEvent* event, time_t startDate, time_t endDate);

    // Expand recurring events for the date range
    std::vector<CalendarEvent*> expandRecurringEvent(CalendarEvent* event,
                                                     time_t startDate,
                                                     time_t endDate);

    // Parse a line from stream
    String readLineFromStream(Stream* stream, bool& eof);

    // Extract value from property line
    String extractValue(const String& line, const String& property);

    // Extract value from event buffer
    String extractValueFromBuffer(const String& buffer, const String& property);

    // Parse date/time string to Unix timestamp
    time_t parseDateTime(const String& dtString);

    // Clean up a single event
    void deleteEvent(CalendarEvent* event);
};

/**
 * Optimized Calendar Manager that uses streaming parsers
 * Coordinates multiple calendars without storing all events
 */
class OptimizedCalendarManager {
public:
    struct CalendarSource {
        String name;
        String url;
        uint16_t color;
        bool enabled;
        int days_to_fetch;
    };

    OptimizedCalendarManager();
    ~OptimizedCalendarManager();

    /**
     * Add a calendar source
     */
    void addCalendar(const CalendarSource& source);

    /**
     * Get merged events from all calendars for a specific date range
     * Events are fetched on-demand and not stored
     *
     * @param startDate Start of date range
     * @param endDate End of date range
     * @param maxEventsPerCalendar Maximum events per calendar
     * @return Merged and sorted events (caller must delete)
     */
    std::vector<CalendarEvent*> getEventsForRange(time_t startDate,
                                                  time_t endDate,
                                                  size_t maxEventsPerCalendar = 50);

    /**
     * Get events for a specific day
     * Convenience method for single day queries
     *
     * @param date The date to query (will use full day)
     * @return Events for that day (caller must delete)
     */
    std::vector<CalendarEvent*> getEventsForDay(time_t date);

    /**
     * Clear all calendar sources
     */
    void clearCalendars();

    /**
     * Get memory usage estimate
     */
    size_t getMemoryUsage() const;

    // Configuration
    void setDebug(bool enable) { debug = enable; }
    void setCacheEnabled(bool enable) { cacheEnabled = enable; }
    void setCacheDuration(uint32_t seconds) { cacheDuration = seconds; }

private:
    std::vector<CalendarSource> calendars;
    std::vector<CalendarStreamParser*> parsers;
    bool debug;

    // Optional caching for current display period
    bool cacheEnabled;
    uint32_t cacheDuration;
    time_t cacheStartDate;
    time_t cacheEndDate;
    time_t cacheTimestamp;
    std::vector<CalendarEvent*> cachedEvents;

    // Check if cache is valid for requested range
    bool isCacheValid(time_t startDate, time_t endDate) const;

    // Clear cache
    void clearCache();

    // Merge and sort events from multiple calendars
    std::vector<CalendarEvent*> mergeAndSortEvents(
        const std::vector<std::vector<CalendarEvent*>>& eventLists);
};

#endif // CALENDAR_STREAM_PARSER_H