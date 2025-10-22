#ifndef ICS_PARSER_H
#define ICS_PARSER_H

#ifdef NATIVE_TEST
    // For native testing
    #include "../test/mock_arduino.h"
#else
    // For actual Arduino environment
    #include <Arduino.h>
    #include <LittleFS.h>
#endif

#include <vector>

// Include the CalendarEvent class
#include "calendar_event.h"

enum class ICSParseError {
    NONE = 0,
    FILE_NOT_FOUND,
    INVALID_FORMAT,
    UNSUPPORTED_VERSION,
    MISSING_BEGIN_CALENDAR,
    MISSING_END_CALENDAR,
    MISSING_VERSION,
    MISSING_PRODID,
    INVALID_DATE_FORMAT,
    MEMORY_ALLOCATION_FAILED,
    STREAM_READ_ERROR
};

class ICSParser {
public:
    ICSParser();
    ~ICSParser();

    // Loading methods
    bool loadFromFile(const String& filepath);
    bool loadFromString(const String& icsData);
    bool loadFromStream(Stream* stream);

    // Validation
    bool isValid() const { return valid; }
    String getLastError() const { return lastError; }
    ICSParseError getErrorCode() const { return errorCode; }

    // Calendar properties
    String getVersion() const { return version; }
    String getProductId() const { return prodId; }
    String getCalendarScale() const { return calScale; }
    String getMethod() const { return method; }
    String getCalendarName() const { return calendarName; }
    String getCalendarDescription() const { return calendarDesc; }
    String getTimezone() const { return timezone; }

    // Event access
    size_t getEventCount() const { return events.size(); }
    CalendarEvent* getEvent(size_t index) const;
    std::vector<CalendarEvent*> getEvents(time_t start, time_t end) const;
    std::vector<CalendarEvent*> getAllEvents() const { return events; }

    // Date range filtering with recurring event support
    std::vector<CalendarEvent*> getEventsInRange(time_t startDate, time_t endDate) const;

    // Memory management
    void clear();

    // Debug control
    void setDebug(bool enable) { debug = enable; }
    bool getDebug() const { return debug; }

    // Memory info
    void printMemoryInfo() const;

    // RRULE parsing helper (public for testing)
    bool parseRRule(const String& rrule, String& freq, int& interval, int& count, String& until) const;

    // Testing support - make these public for unit tests
    #ifdef NATIVE_TEST
    public:
    #else
    private:
    #endif
    // Parser methods
    bool parse(const String& icsData);
    bool parseHeader(const String& icsData);
    bool parseEvents(const String& icsData);
    bool parseEvent(const String& eventData);
    bool parseLine(const String& line, String& name, String& params, String& value);
    String unfoldLines(const String& data);
    String extractValue(const String& data, const String& property);

    // Validation methods
    bool validateHeader();
    void setError(ICSParseError error, const String& message);

    // Recurring event helpers
    bool isEventInRange(CalendarEvent* event, time_t startDate, time_t endDate) const;
    std::vector<CalendarEvent*> expandRecurringEvent(CalendarEvent* event, time_t startDate, time_t endDate) const;
    time_t calculateNextOccurrence(CalendarEvent* event, time_t after, const String& rrule) const;

    // Streaming parse helpers for large files
    bool parseInChunks(const String& icsData);
    bool parseStreamLarge(Stream* stream);
    String readLineFromStream(Stream* stream, bool& eof);
    bool processEventBlock(const String& eventBlock);

    // Calendar properties
    String version;
    String prodId;
    String calScale;
    String method;
    String calendarName;
    String calendarDesc;
    String timezone;

    // Events storage
    std::vector<CalendarEvent*> events;

    // Parser state
    String lastError;
    ICSParseError errorCode;
    bool valid;

    // Debug flag
    bool debug;
};

#endif // ICS_PARSER_H