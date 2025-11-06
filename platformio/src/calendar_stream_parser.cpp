/**
 * Implementation of memory-efficient calendar stream parser
 */

#include "calendar_stream_parser.h"

#ifdef NATIVE_TEST
// Mock dependencies for native testing
#include "../test/test_native/mock_calendar_fetcher.h"
#include <cstdio> // For printf in native tests

// For native tests, use printf so output is visible in test results
// Helper to convert String to const char* for printf
inline const char* str_helper(const String& s) { return s.c_str(); }
inline const char* str_helper(const char* s) { return s; }

#define DEBUG_INFO_PRINTLN(x)                                                                      \
    do {                                                                                           \
        String _s = (x);                                                                           \
        printf("%s\n", _s.c_str());                                                                \
    } while (0)
#define DEBUG_INFO_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_VERBOSE_PRINTLN(x)                                                                   \
    do {                                                                                           \
        String _s = (x);                                                                           \
        printf("%s\n", _s.c_str());                                                                \
    } while (0)
#define DEBUG_VERBOSE_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_WARN_PRINTLN(x)                                                                      \
    do {                                                                                           \
        String _s = (x);                                                                           \
        printf("[WARN] %s\n", _s.c_str());                                                         \
    } while (0)
#define DEBUG_ERROR_PRINTLN(x)                                                                     \
    do {                                                                                           \
        String _s = (x);                                                                           \
        printf("[ERROR] %s\n", _s.c_str());                                                        \
    } while (0)

// Stub TeeStream - not needed for recurring event tests
class TeeStream : public Stream {
  public:
    TeeStream(Stream& s1, Stream& s2) {}
    TeeStream(Stream& s1, File& f2) {} // Also accept File for cache writing
    int available() { return 0; }
    int read() { return -1; }
    int peek() { return -1; }
    size_t write(uint8_t) { return 0; }
    String readString() { return ""; }
};

#else
#include "calendar_fetcher.h"
#include "debug_config.h"
// TeeStream no longer needed - we download first, then parse
#endif

#include <algorithm>

// Helper function for portable timegm (converts tm in UTC to time_t)
static time_t portable_timegm(struct tm* tm) {
#ifndef ARDUINO
    // POSIX systems (including test environment) have timegm
    return timegm(tm);
#else
    // On Arduino/ESP32, temporarily set timezone to UTC
    char* oldTZ    = getenv("TZ");
    String savedTZ = oldTZ ? String(oldTZ) : "";
    setenv("TZ", "UTC0", 1);
    tzset();
    time_t result = mktime(tm);
    if (!savedTZ.isEmpty()) {
        setenv("TZ", savedTZ.c_str(), 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return result;
#endif
}

// Constructor
CalendarStreamParser::CalendarStreamParser() : debug(false), calendarColor(0), fetcher(nullptr) {
    fetcher = new CalendarFetcher();
    fetcher->setDebug(debug);
}

// Destructor
CalendarStreamParser::~CalendarStreamParser() {
    if (fetcher) {
        delete fetcher;
    }
}

FilteredEvents* CalendarStreamParser::fetchEventsInRange(const String& url,
                                                         time_t startDate,
                                                         time_t endDate,
                                                         size_t maxEvents,
                                                         const String& cachePath) {
    FilteredEvents* result = new FilteredEvents();

    DEBUG_INFO_PRINTLN("=== Stream Parsing Calendar ===");
    DEBUG_INFO_PRINTLN("URL: " + url);
    DEBUG_INFO_PRINTLN("Date range: " + String(startDate) + " to " + String(endDate));
    DEBUG_INFO_PRINTLN("Max events: " + String((unsigned long)maxEvents));

    auto eventCallback = [&](CalendarEvent* event) {
        if (result->events.size() < maxEvents || maxEvents == 0) {
            result->events.push_back(event);
        } else {
            delete event; // Free memory if maxEvents is reached
        }
    };

    if (streamParse(url, eventCallback, startDate, endDate, cachePath)) {
        result->success       = true;
        result->totalFiltered = result->events.size();
        // totalParsed is not easily available here, would require another callback
    } else {
        result->success = false;
        result->error   = "Stream parsing failed";
    }

    // Sort events by start time
    std::sort(result->events.begin(), result->events.end(), [](CalendarEvent* a, CalendarEvent* b) {
        return a->startTime < b->startTime;
    });

    DEBUG_INFO_PRINTLN("Parsing complete: " + String((unsigned long)result->totalFiltered) +
                       " events filtered");
    return result;
}

bool CalendarStreamParser::streamParseFromStream(Stream* stream,
                                                 EventCallback callback,
                                                 time_t startDate,
                                                 time_t endDate) {
    if (!callback || !stream) {
        return false;
    }

    ParseState state   = LOOKING_FOR_CALENDAR;
    String currentLine = "";
    String eventBuffer = "";
    bool eof           = false;
    int eventCount     = 0;
    int lineCount      = 0;
    int eventsFiltered = 0;
    int eventsRejected = 0;
    bool parseSuccess  = true;

    // Convert timestamps to readable dates for debugging
    char dateStartStr[32], dateEndStr[32];
    struct tm tmStartCopy, tmEndCopy;

    // localtime() returns pointer to static buffer, so we need to copy before second call
    struct tm* tmStart = localtime(&startDate);
    memcpy(&tmStartCopy, tmStart, sizeof(struct tm));

    struct tm* tmEnd = localtime(&endDate);
    memcpy(&tmEndCopy, tmEnd, sizeof(struct tm));

    strftime(dateStartStr, sizeof(dateStartStr), "%Y-%m-%d", &tmStartCopy);
    strftime(dateEndStr, sizeof(dateEndStr), "%Y-%m-%d", &tmEndCopy);

    DEBUG_INFO_PRINTLN(">>> Starting stream parsing...");
    DEBUG_INFO_PRINTLN(">>> Date range filter: " + String(dateStartStr) + " (" + String(startDate) +
                       ") to " + String(dateEndStr) + " (" + String(endDate) + ")");

    while (!eof && state != DONE) {
        currentLine = readLineFromStream(stream, eof);
        lineCount++;

        if (currentLine.isEmpty() && !eof) {
            continue;
        }

        // Progress update every 100 lines
        if (lineCount % 100 == 0) {
            DEBUG_VERBOSE_PRINTLN(">>> Parse progress: " + String(lineCount) + " lines read, " +
                                  String(eventCount) + " events parsed, " + String(eventsFiltered) +
                                  " events filtered");
        }

        switch (state) {
        case LOOKING_FOR_CALENDAR:
            if (currentLine.indexOf("BEGIN:VCALENDAR") != -1) {
                DEBUG_VERBOSE_PRINTLN(">>> Found BEGIN:VCALENDAR");
                state = IN_HEADER;
            }
            break;

        case IN_HEADER:
            if (currentLine.indexOf("BEGIN:VEVENT") != -1) {
                eventBuffer = currentLine + "\n";
                state       = IN_EVENT;
            } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                DEBUG_VERBOSE_PRINTLN(">>> Found END:VCALENDAR");
                state = DONE;
            }
            break;

        case IN_EVENT:
            eventBuffer += currentLine + "\n";

            if (currentLine.indexOf("END:VEVENT") != -1) {
                // Parse event
                CalendarEvent* event = parseEventFromBuffer(eventBuffer);
                if (event) {
                    eventCount++;
                    if (event->isRecurring) {
                        std::vector<CalendarEvent*> expanded =
                            expandRecurringEventV2(event, startDate, endDate);
                        int expandedCount = 0;
                        for (auto expandedEvent : expanded) {
                            if (isEventInRange(expandedEvent, startDate, endDate)) {
                                callback(expandedEvent);
                                eventsFiltered++;
                                expandedCount++;
                            } else {
                                delete expandedEvent;
                            }
                        }
                        // Debug: show recurring event expansion
                        if (expandedCount > 0) {
                            DEBUG_VERBOSE_PRINTF(
                                ">>> Recurring event expanded: '%s' (RRULE: %s) → %d occurrences\n",
                                event->summary.c_str(),
                                event->rrule.c_str(),
                                expandedCount);
                        }
                        delete event;
                    } else if (isEventInRange(event, startDate, endDate)) {
                        callback(event);
                        eventsFiltered++;
                    } else {
                        // Event rejected - show first few for debugging
                        if (eventsRejected < 3) {
                            struct tm* tmEvent = localtime(&event->startTime);
                            char eventDateStr[32];
                            strftime(eventDateStr, sizeof(eventDateStr), "%Y-%m-%d", tmEvent);
                            DEBUG_WARN_PRINTLN(">>> Event rejected (out of range): '" +
                                               event->summary + "' on " + String(eventDateStr) +
                                               " (" + String(event->startTime) + ")");
                        }
                        eventsRejected++;
                        delete event;
                    }
                }

                eventBuffer = "";
                state       = IN_HEADER;

                if (eventCount % 10 == 0) {
                    delay(1);
                }
            } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                DEBUG_WARN_PRINTLN(">>> Found END:VCALENDAR (unexpected in event)");
                state = DONE;
            }
            break;

        case DONE: break;
        }

        if (eventBuffer.length() > 8192) {
            DEBUG_ERROR_PRINTLN(">>> ERROR: Event buffer exceeded 8192 bytes, skipping event");
            eventBuffer  = "";
            state        = IN_HEADER;
            parseSuccess = false; // Indicate an error
        }
    }

    DEBUG_INFO_PRINTLN(">>> Stream parsing complete: " + String(lineCount) + " lines read, " +
                       String(eventCount) + " events parsed, " + String(eventsFiltered) +
                       " events filtered, " + String(eventsRejected) + " events rejected");

    return parseSuccess;
}

bool CalendarStreamParser::streamParse(const String& url,
                                       EventCallback callback,
                                       time_t startDate,
                                       time_t endDate,
                                       const String& cachePath) {
    if (!callback) {
        return false;
    }

    bool isRemote = !url.startsWith("file://");

    if (isRemote) {
        // Parse directly from HTTP stream (no ICS file caching)
        DEBUG_INFO_PRINTLN(">>> Opening HTTP stream for direct parsing: " + url);

        Stream* httpStream = fetcher->fetchStream(url);
        if (!httpStream) {
            DEBUG_ERROR_PRINTLN(">>> ERROR: Failed to open HTTP stream");
            return false;
        }

        DEBUG_INFO_PRINTLN(">>> Stream opened, starting direct parse...");
        unsigned long parseStart = millis();

        // Parse directly from HTTP stream
        bool parseSuccess = streamParseFromStream(httpStream, callback, startDate, endDate);

        fetcher->endStream();

        unsigned long parseDuration = millis() - parseStart;
        DEBUG_INFO_PRINTLN(">>> Parse complete in " + String(parseDuration) +
                           "ms, success: " + String(parseSuccess ? "true" : "false"));

        return parseSuccess;
    } else {
        // Parse from local file
        String parseFile = url.substring(7); // Remove "file://" prefix
        DEBUG_INFO_PRINTLN(">>> Opening local file for parsing: " + parseFile);

        File file = LittleFS.open(parseFile, "r");
        if (!file) {
            DEBUG_ERROR_PRINTLN(">>> ERROR: Could not open file for parsing: " + parseFile);
            return false;
        }

        DEBUG_INFO_PRINTLN(">>> File opened, size: " + String((unsigned long)file.size()) +
                           " bytes");
        DEBUG_INFO_PRINTLN(">>> Starting parse...");

        // Parse from the file stream
        bool parseSuccess = streamParseFromStream(&file, callback, startDate, endDate);

        file.close();
        DEBUG_INFO_PRINTLN(">>> Parse complete, success: " +
                           String(parseSuccess ? "true" : "false"));

        return parseSuccess;
    }
}

bool CalendarStreamParser::parseMetadata(const String& url,
                                         String& calendarName,
                                         String& timezone) {
    // Get stream from fetcher
    Stream* stream = fetcher->fetchStream(url);
    if (!stream) {
        return false;
    }

    bool foundCalendar = false;
    String line        = "";
    bool eof           = false;

    while (!eof && !foundCalendar) {
        line = readLineFromStream(stream, eof);

        if (line.indexOf("BEGIN:VCALENDAR") != -1) {
            foundCalendar = true;
        }
    }

    if (!foundCalendar) {
        fetcher->endStream();
        return false;
    }

    // Parse header until first event
    while (!eof) {
        line = readLineFromStream(stream, eof);

        if (line.indexOf("X-WR-CALNAME:") != -1) {
            calendarName = extractValue(line, "X-WR-CALNAME:");
        } else if (line.indexOf("X-WR-TIMEZONE:") != -1) {
            timezone = extractValue(line, "X-WR-TIMEZONE:");
        } else if (line.indexOf("BEGIN:VEVENT") != -1) {
            // Stop at first event
            break;
        }
    }

    fetcher->endStream();
    return true;
}

CalendarEvent* CalendarStreamParser::parseEventFromBuffer(const String& eventData) {
    CalendarEvent* event = new CalendarEvent();

    // Extract event fields
    event->summary     = extractValueFromBuffer(eventData, "SUMMARY:");
    event->location    = extractValueFromBuffer(eventData, "LOCATION:");
    event->description = extractValueFromBuffer(eventData, "DESCRIPTION:");
    event->uid         = extractValueFromBuffer(eventData, "UID:");
    event->status      = extractValueFromBuffer(eventData, "STATUS:");
    event->rrule       = extractValueFromBuffer(eventData, "RRULE:");
    event->isRecurring = !event->rrule.isEmpty();

    // Parse start date/time (with timezone support)
    String dtStart;
    String dtStartTZID;

    // Check for TZID parameter first (e.g., DTSTART;TZID=Europe/Amsterdam:20120119T150000)
    int dtStartPos = eventData.indexOf("DTSTART;TZID=");
    if (dtStartPos >= 0) {
        int colonPos   = eventData.indexOf(":", dtStartPos);
        int newlinePos = eventData.indexOf("\n", colonPos);
        if (newlinePos == -1)
            newlinePos = eventData.length();

        // Extract TZID value
        String dtStartLine = eventData.substring(dtStartPos, colonPos);
        int tzidStart      = dtStartLine.indexOf("TZID=") + 5;
        dtStartTZID        = dtStartLine.substring(tzidStart);
        dtStartTZID.trim();

        // Extract datetime value
        dtStart = eventData.substring(colonPos + 1, newlinePos);
        dtStart.trim();
    } else {
        // No TZID - try standard patterns
        dtStart = extractValueFromBuffer(eventData, "DTSTART:");
        if (dtStart.isEmpty()) {
            dtStart = extractValueFromBuffer(eventData, "DTSTART;VALUE=DATE:");
        }
        if (dtStart.isEmpty()) {
            dtStart = extractValueFromBuffer(eventData, "DTSTART;VALUE=DATE-TIME:");
        }
    }

    if (!dtStart.isEmpty()) {
        event->dtStart = dtStart;
        event->allDay  = (dtStart.length() == 8);

        if (!dtStartTZID.isEmpty()) {
            // Parse with timezone conversion
            event->startTime = CalendarEvent::parseICSDateTimeWithTZ(dtStart, dtStartTZID);
            // Note: startDate and startTimeStr are now computed on-demand via getters
            // No need to store them
        } else {
            // Parse without timezone (UTC or all-day)
            event->setStartDateTime(dtStart);
        }
    }

    // Parse end date/time (with timezone support)
    String dtEnd;
    String dtEndTZID;

    // Check for TZID parameter first
    int dtEndPos = eventData.indexOf("DTEND;TZID=");
    if (dtEndPos >= 0) {
        int colonPos   = eventData.indexOf(":", dtEndPos);
        int newlinePos = eventData.indexOf("\n", colonPos);
        if (newlinePos == -1)
            newlinePos = eventData.length();

        // Extract TZID value
        String dtEndLine = eventData.substring(dtEndPos, colonPos);
        int tzidStart    = dtEndLine.indexOf("TZID=") + 5;
        dtEndTZID        = dtEndLine.substring(tzidStart);
        dtEndTZID.trim();

        // Extract datetime value
        dtEnd = eventData.substring(colonPos + 1, newlinePos);
        dtEnd.trim();
    } else {
        // No TZID - try standard patterns
        dtEnd = extractValueFromBuffer(eventData, "DTEND:");
        if (dtEnd.isEmpty()) {
            dtEnd = extractValueFromBuffer(eventData, "DTEND;VALUE=DATE:");
        }
        if (dtEnd.isEmpty()) {
            dtEnd = extractValueFromBuffer(eventData, "DTEND;VALUE=DATE-TIME:");
        }
    }

    if (!dtEnd.isEmpty()) {
        event->dtEnd = dtEnd;

        if (!dtEndTZID.isEmpty()) {
            // Parse with timezone conversion
            event->endTime = CalendarEvent::parseICSDateTimeWithTZ(dtEnd, dtEndTZID);
        } else {
            // Parse without timezone (UTC or all-day)
            event->setEndDateTime(dtEnd);
        }
    }

    // Debug: Track specific event
    bool isTargetEvent =
        (event->uid.indexOf("cor64p3165hjabb364qm2b9k68o3abb26gs3gbb5cco3gc1mcorm8p1o68") >= 0);
    if (isTargetEvent) {
        struct tm* tmStart = localtime(&event->startTime);
        struct tm* tmEnd   = localtime(&event->endTime);
        char startStr[32], endStr[32];
        strftime(startStr, sizeof(startStr), "%Y-%m-%d %H:%M:%S", tmStart);
        strftime(endStr, sizeof(endStr), "%Y-%m-%d %H:%M:%S", tmEnd);

        DEBUG_INFO_PRINTF(">>> PARSING TARGET EVENT (Cambio gomme) <<<\n");
        DEBUG_INFO_PRINTF("  Summary: %s\n", event->summary.c_str());
        DEBUG_INFO_PRINTF("  UID: %s\n", event->uid.c_str());
        DEBUG_INFO_PRINTF("  dtStart string: %s\n", event->dtStart.c_str());
        DEBUG_INFO_PRINTF("  dtEnd string: %s\n", event->dtEnd.c_str());
        DEBUG_INFO_PRINTF("  All-day: %s\n", event->allDay ? "YES" : "NO");
        DEBUG_INFO_PRINTF("  Parsed startTime: %s (unix: %ld)\n", startStr, event->startTime);
        DEBUG_INFO_PRINTF("  Parsed endTime: %s (unix: %ld)\n", endStr, event->endTime);
    }

    return event;
}

String CalendarStreamParser::extractValueFromBuffer(const String& buffer, const String& property) {
    int pos = buffer.indexOf(property);
    if (pos == -1) {
        return "";
    }

    int valueStart = pos + property.length();
    int valueEnd   = buffer.indexOf("\n", valueStart);
    if (valueEnd == -1) {
        valueEnd = buffer.length();
    }

    String value = buffer.substring(valueStart, valueEnd);
    value.trim();
    value.replace("\r", "");

    return value;
}

bool CalendarStreamParser::isEventInRange(CalendarEvent* event, time_t startDate, time_t endDate) {
    if (!event)
        return false;

    time_t eventStart = event->startTime;
    time_t eventEnd   = event->endTime;

    if (eventEnd == 0) {
        eventEnd = eventStart;
    }

    return (eventStart <= endDate) && (eventEnd >= startDate);
}

/**
 * Parse an RRULE string into structured components
 * Example: "FREQ=WEEKLY;BYDAY=MO,WE,FR;COUNT=10"
 */
RRuleComponents CalendarStreamParser::parseRRule(const String& rrule) {
    RRuleComponents result;

    if (rrule.isEmpty()) {
        return result;
    }

    // Split by semicolon to get key=value pairs
    int startPos = 0;
    int semiPos  = 0;

    while (semiPos != -1) {
        semiPos = rrule.indexOf(';', startPos);
        String part =
            (semiPos == -1) ? rrule.substring(startPos) : rrule.substring(startPos, semiPos);
        startPos = semiPos + 1;

        // Split each part by '=' to get key and value
        int eqPos = part.indexOf('=');
        if (eqPos == -1)
            continue;

        String key   = part.substring(0, eqPos);
        String value = part.substring(eqPos + 1);

        key.trim();
        value.trim();

        // Parse each component
        if (key == "FREQ") {
            result.freq = value;
        } else if (key == "COUNT") {
            result.count = value.toInt();
            if (result.count < 0)
                result.count = -1; // Sanitize negative values
        } else if (key == "UNTIL") {
            result.until = parseUntilDate(value);
        } else if (key == "INTERVAL") {
            result.interval = value.toInt();
            if (result.interval <= 0)
                result.interval = 1; // Default to 1 if invalid
        } else if (key == "BYDAY") {
            result.byDay = value;
        } else if (key == "BYMONTHDAY") {
            result.byMonthDay = value;
        } else if (key == "BYMONTH") {
            result.byMonth = value;
        }
        // Note: Other RRULE properties (BYHOUR, BYMINUTE, etc.) can be added later if needed
    }

    return result;
}

/**
 * Parse UNTIL date string (RFC 5545 format)
 * Format: 20251231T235959Z or 20251231
 */
time_t CalendarStreamParser::parseUntilDate(const String& untilStr) {
    if (untilStr.isEmpty())
        return 0;

    // Remove 'Z' suffix if present (UTC indicator)
    String dateStr = untilStr;
    dateStr.replace("Z", "");

    // Parse: YYYYMMDD or YYYYMMDDTHHMMSS
    if (dateStr.length() < 8)
        return 0;

    struct tm tm = {0};

    // Extract date components
    String yearStr = dateStr.substring(0, 4);
    String monStr  = dateStr.substring(4, 6);
    String dayStr  = dateStr.substring(6, 8);

    tm.tm_year     = yearStr.toInt() - 1900;
    tm.tm_mon      = monStr.toInt() - 1;
    tm.tm_mday     = dayStr.toInt();

    // If time component present
    if (dateStr.length() >= 15 && dateStr.charAt(8) == 'T') {
        String hourStr = dateStr.substring(9, 11);
        String minStr  = dateStr.substring(11, 13);
        String secStr  = dateStr.substring(13, 15);

        tm.tm_hour     = hourStr.toInt();
        tm.tm_min      = minStr.toInt();
        tm.tm_sec      = secStr.toInt();
    } else {
        // No time specified, default to end of day
        tm.tm_hour = 23;
        tm.tm_min  = 59;
        tm.tm_sec  = 59;
    }

    tm.tm_isdst = -1; // Let mktime determine DST
    return mktime(&tm);
}

/**
 * Parse BYDAY into weekday numbers
 * Input: "MO,WE,FR" or "1MO,-1FR" (with position prefix)
 * Output: Vector of weekday numbers (0=Sunday, 1=Monday, ..., 6=Saturday)
 * Note: Position prefixes (1MO, -1FR) are stored but not yet fully implemented
 */
std::vector<int> CalendarStreamParser::parseByDay(const String& byDay) {
    std::vector<int> weekdays;
    if (byDay.isEmpty())
        return weekdays;

    int startPos = 0;
    int commaPos = 0;

    while (commaPos != -1) {
        commaPos = byDay.indexOf(',', startPos);
        String day =
            (commaPos == -1) ? byDay.substring(startPos) : byDay.substring(startPos, commaPos);
        startPos = commaPos + 1;

        day.trim();
        if (day.isEmpty())
            continue;

        // Extract day code (last 2 characters, ignoring position prefix)
        String dayCode = day.length() >= 2 ? day.substring(day.length() - 2) : day;

        // Map day codes to numbers (0=SU, 1=MO, 2=TU, 3=WE, 4=TH, 5=FR, 6=SA)
        int weekday = -1;
        if (dayCode == "SU")
            weekday = 0;
        else if (dayCode == "MO")
            weekday = 1;
        else if (dayCode == "TU")
            weekday = 2;
        else if (dayCode == "WE")
            weekday = 3;
        else if (dayCode == "TH")
            weekday = 4;
        else if (dayCode == "FR")
            weekday = 5;
        else if (dayCode == "SA")
            weekday = 6;

        if (weekday != -1) {
            weekdays.push_back(weekday);
        }
    }

    return weekdays;
}

/**
 * Parse BYMONTHDAY into day numbers
 * Input: "1,15,-1" (negative means from end of month)
 * Output: Vector of day numbers
 */
std::vector<int> CalendarStreamParser::parseByMonthDay(const String& byMonthDay) {
    std::vector<int> days;
    if (byMonthDay.isEmpty())
        return days;

    int startPos = 0;
    int commaPos = 0;

    while (commaPos != -1) {
        commaPos      = byMonthDay.indexOf(',', startPos);
        String dayStr = (commaPos == -1) ? byMonthDay.substring(startPos)
                                         : byMonthDay.substring(startPos, commaPos);
        startPos      = commaPos + 1;

        dayStr.trim();
        if (dayStr.isEmpty())
            continue;

        int day = dayStr.toInt();
        if (day !=
            0) { // toInt returns 0 for invalid strings, but we want to allow 0 to be filtered out
            days.push_back(day);
        }
    }

    return days;
}

/**
 * Parse BYMONTH into month numbers
 * Input: "1,7" (1=January, 12=December)
 * Output: Vector of month numbers (1-12)
 */
std::vector<int> CalendarStreamParser::parseByMonth(const String& byMonth) {
    std::vector<int> months;
    if (byMonth.isEmpty())
        return months;

    int startPos = 0;
    int commaPos = 0;

    while (commaPos != -1) {
        commaPos = byMonth.indexOf(',', startPos);
        String monthStr =
            (commaPos == -1) ? byMonth.substring(startPos) : byMonth.substring(startPos, commaPos);
        startPos = commaPos + 1;

        monthStr.trim();
        if (monthStr.isEmpty())
            continue;

        int month = monthStr.toInt();
        if (month >= 1 && month <= 12) {
            months.push_back(month);
        }
    }

    return months;
}

/**
 * Convert string frequency to enum
 */
RecurrenceFrequency CalendarStreamParser::frequencyFromString(const String& freqStr) {
    if (freqStr == "YEARLY")
        return RecurrenceFrequency::YEARLY;
    if (freqStr == "MONTHLY")
        return RecurrenceFrequency::MONTHLY;
    if (freqStr == "WEEKLY")
        return RecurrenceFrequency::WEEKLY;
    if (freqStr == "DAILY")
        return RecurrenceFrequency::DAILY;
    if (freqStr == "HOURLY")
        return RecurrenceFrequency::HOURLY;
    if (freqStr == "MINUTELY")
        return RecurrenceFrequency::MINUTELY;
    if (freqStr == "SECONDLY")
        return RecurrenceFrequency::SECONDLY;
    return RecurrenceFrequency::NONE;
}

/**
 * Find the first occurrence of a recurring event on or after startDate.
 * Uses tm struct arithmetic to efficiently skip forward from eventStart.
 *
 * @param eventStart The DTSTART of the recurring event
 * @param startDate The query range start (find first occurrence on/after this)
 * @param endDate The query range end (first occurrence must be <= this)
 * @param interval The INTERVAL value (1 = every period, 2 = every 2 periods, etc.)
 * @param freq The frequency (YEARLY, MONTHLY, WEEKLY, DAILY)
 * @param count Optional COUNT limit (-1 = no limit). If set, checks if recurrence completed before
 * startDate
 * @return The timestamp of the first occurrence on/after startDate, or -1 if none exists
 */
time_t CalendarStreamParser::findFirstOccurrence(time_t eventStart,
                                                 time_t startDate,
                                                 time_t endDate,
                                                 int interval,
                                                 RecurrenceFrequency freq,
                                                 int count) {
    // Validate input
    if (eventStart < 0 || startDate < 0 || endDate < 0 || startDate > endDate || interval < 1) {
        return -1; // Invalid parameters
    }

    // If event starts after query range, no occurrence in range
    if (eventStart > endDate) {
        return -1;
    }

    // If COUNT is specified, check if the recurrence would complete before startDate
    if (count > 0) {
        // Calculate where the Nth (last) occurrence would be (using GMT)
        struct tm lastOccurrenceTm;
        memcpy(&lastOccurrenceTm, gmtime(&eventStart), sizeof(struct tm));

        switch (freq) {
        case RecurrenceFrequency::YEARLY:  lastOccurrenceTm.tm_year += (count - 1) * interval; break;
        case RecurrenceFrequency::MONTHLY: lastOccurrenceTm.tm_mon += (count - 1) * interval; break;
        case RecurrenceFrequency::WEEKLY:
            lastOccurrenceTm.tm_mday += (count - 1) * interval * 7;
            break;
        case RecurrenceFrequency::DAILY: lastOccurrenceTm.tm_mday += (count - 1) * interval; break;
        default:                         break;
        }

        time_t lastOccurrence = portable_timegm(&lastOccurrenceTm);

        // If the last occurrence is before startDate, the recurrence is complete
        if (lastOccurrence < startDate) {
            return -1;
        }
    }

    // If event starts within query range, return event start
    if (eventStart >= startDate && eventStart <= endDate) {
        return eventStart;
    }

    // Event starts before query range - calculate first occurrence >= startDate
    // Use GMT/UTC consistently to avoid timezone conversion issues
    struct tm eventTm, startTm;
    memcpy(&eventTm, gmtime(&eventStart), sizeof(struct tm));
    memcpy(&startTm, gmtime(&startDate), sizeof(struct tm));

    if (freq == RecurrenceFrequency::YEARLY) {
        // Calculate years between event start and query start
        int eventYear = eventTm.tm_year + 1900;
        int startYear = startTm.tm_year + 1900;
        int yearsDiff = startYear - eventYear;

        if (yearsDiff > 0) {
            // Calculate how many intervals to skip
            int intervalsToSkip = (yearsDiff + interval - 1) / interval; // Ceiling division
            eventTm.tm_year += intervalsToSkip * interval;

            time_t candidate = portable_timegm(&eventTm);
            // If we undershot, advance one interval
            if (candidate < startDate) {
                eventTm.tm_year += interval;
                candidate = portable_timegm(&eventTm);
            }
            // Validate candidate is within range
            return (candidate <= endDate) ? candidate : -1;
        }
    } else if (freq == RecurrenceFrequency::MONTHLY) {
        // Calculate months between event start and query start
        int monthsDiff =
            (startTm.tm_year - eventTm.tm_year) * 12 + (startTm.tm_mon - eventTm.tm_mon);

        if (monthsDiff > 0) {
            // Calculate how many intervals to skip
            int intervalsToSkip = (monthsDiff + interval - 1) / interval; // Ceiling division
            eventTm.tm_mon += intervalsToSkip * interval;

            time_t candidate =
                portable_timegm(&eventTm); // portable_timegm normalizes year overflow
            // If we undershot, advance one interval
            if (candidate < startDate) {
                eventTm.tm_mon += interval;
                candidate = portable_timegm(&eventTm);
            }
            // Validate candidate is within range
            return (candidate <= endDate) ? candidate : -1;
        }
    } else if (freq == RecurrenceFrequency::WEEKLY) {
        // Calculate days between event start and query start
        int daysDiff = (startDate - eventStart) / 86400;

        if (daysDiff > 0) {
            // Calculate how many week intervals to skip
            int weeksDiff       = daysDiff / 7;
            int intervalsToSkip = (weeksDiff + interval - 1) / interval; // Ceiling division
            eventTm.tm_mday += intervalsToSkip * interval * 7;

            time_t candidate = portable_timegm(&eventTm); // portable_timegm normalizes
            // If we undershot, advance one interval
            if (candidate < startDate) {
                eventTm.tm_mday += interval * 7;
                candidate = portable_timegm(&eventTm);
            }
            // Validate candidate is within range
            return (candidate <= endDate) ? candidate : -1;
        }
    } else if (freq == RecurrenceFrequency::DAILY) {
        // Calculate days between event start and query start
        int daysDiff = (startDate - eventStart) / 86400;

        if (daysDiff >= 0) { // Changed from > 0 to >= 0 to handle same-day cases
            // Calculate how many intervals to skip
            int intervalsToSkip = (daysDiff + interval - 1) / interval; // Ceiling division
            eventTm.tm_mday += intervalsToSkip * interval;

            time_t candidate = portable_timegm(&eventTm); // portable_timegm normalizes
            // If we undershot, advance one interval
            if (candidate < startDate) {
                eventTm.tm_mday += interval;
                candidate = portable_timegm(&eventTm);
            }
            // Validate candidate is within range
            return (candidate <= endDate) ? candidate : -1;
        }
    }

    // Fallback: no valid occurrence found
    return -1;
}

String CalendarStreamParser::readLineFromStream(Stream* stream, bool& eof) {
    String line = "";
    eof         = false;

    if (!stream) {
        eof = true;
        return line;
    }

    // Wait for data with timeout (for network streams)
    int retries          = 0;
    const int maxRetries = 100; // 10 seconds total (100 * 100ms)
    while (!stream->available() && retries < maxRetries) {
        delay(100);
        retries++;
    }

    if (!stream->available()) {
        eof = true;
        return line;
    }

    while (stream->available()) {
        char c = stream->read();

        if (c == '\r') {
            if (stream->available() && stream->peek() == '\n') {
                stream->read();
            }
            break;
        } else if (c == '\n') {
            break;
        } else {
            line += c;
        }

        if (line.length() > 1024) {
            break;
        }
    }

    // Don't set EOF just because buffer is empty - check if we got a line
    if (line.isEmpty() && !stream->available()) {
        // Try one more time after a short delay
        delay(100);
        if (!stream->available()) {
            eof = true;
        }
    }

    return line;
}

String CalendarStreamParser::extractValue(const String& line, const String& property) {
    int pos = line.indexOf(property);
    if (pos == -1) {
        return "";
    }

    String value = line.substring(pos + property.length());
    value.trim();
    return value;
}

/**
 * V2: Refactored version of expandRecurringEvent without arbitrary limits.
 *
 * This version:
 * - Validates all inputs clearly
 * - Handles non-recurring events simply
 * - Uses findFirstOccurrence for efficiency
 * - Stops naturally when COUNT/UNTIL/endDate is reached
 * - No hardcoded iteration limits
 *
 * @param event The event to expand (can be recurring or non-recurring)
 * @param startDate Start of query range (inclusive)
 * @param endDate End of query range (inclusive)
 * @return Vector of CalendarEvent pointers (caller must delete)
 */
std::vector<CalendarEvent*> CalendarStreamParser::expandRecurringEventV2(CalendarEvent* event,
                                                                         time_t startDate,
                                                                         time_t endDate) {
    std::vector<CalendarEvent*> occurrences;

    // ========================================================================
    // Step 1: Input Validation
    // ========================================================================

    // Validate event pointer
    if (!event) {
        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: NULL event");
        return occurrences;
    }

    // Validate date range
    if (startDate < 0 || endDate < 0) {
        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: Invalid date range (negative values)");
        return occurrences;
    }

    if (startDate > endDate) {
        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: Invalid date range (start > end)");
        return occurrences;
    }

    // Validate event times
    if (event->startTime < 0 || event->endTime < 0) {
        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: Invalid event times (negative values)");
        return occurrences;
    }

    if (event->endTime < event->startTime) {
        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: Invalid event times (end < start)");
        return occurrences;
    }

    // ========================================================================
    // Step 2: Handle Non-Recurring Events
    // ========================================================================

    if (!event->isRecurring) {
        DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: Non-recurring event");

        // Check if event overlaps with query range
        // Event overlaps if:
        //   - Event starts before/at endDate, AND
        //   - Event ends after/at startDate
        bool overlaps = (event->startTime <= endDate) && (event->endTime >= startDate);

        if (overlaps) {
            // Create a copy of the event
            CalendarEvent* occurrence = new CalendarEvent(*event);
            occurrences.push_back(occurrence);
            DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: Added non-recurring event");
        } else {
            DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: Non-recurring event outside range");
        }

        return occurrences;
    }

    // ========================================================================
    // Step 3: Handle Recurring Events
    // ========================================================================

    DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: Recurring event");

    // Parse RRULE
    RRuleComponents rule = parseRRule(event->rrule);
    if (!rule.isValid()) {
        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: Invalid RRULE");
        return occurrences;
    }

    // Convert frequency string to enum
    RecurrenceFrequency freq = frequencyFromString(rule.freq);

    // Dispatch to frequency-specific handler
    switch (freq) {
    case RecurrenceFrequency::YEARLY:
        DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: YEARLY frequency");
        return expandYearlyV2(event, rule, startDate, endDate);

    case RecurrenceFrequency::MONTHLY:
        DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: MONTHLY frequency");
        return expandMonthlyV2(event, rule, startDate, endDate);

    case RecurrenceFrequency::WEEKLY:
        DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: WEEKLY frequency");
        return expandWeeklyV2(event, rule, startDate, endDate);

    case RecurrenceFrequency::DAILY:
        DEBUG_VERBOSE_PRINTLN("expandRecurringEventV2: DAILY frequency");
        return expandDailyV2(event, rule, startDate, endDate);

    case RecurrenceFrequency::HOURLY:
    case RecurrenceFrequency::MINUTELY:
    case RecurrenceFrequency::SECONDLY:
        DEBUG_WARN_PRINTLN("expandRecurringEventV2: Sub-daily frequencies not yet supported");
        return occurrences;

    case RecurrenceFrequency::NONE:
    default:                        DEBUG_ERROR_PRINTLN("expandRecurringEventV2: Unknown frequency"); return occurrences;
    }
}

/**
 * V2: Expand YEARLY recurring events
 *
 * This method handles expansion of events that recur yearly (e.g., birthdays, anniversaries).
 *
 * Algorithm:
 * 1. Calculate event duration (endTime - startTime) to preserve for all occurrences
 * 2. Determine effective end date (consider UNTIL if present)
 * 3. Use findFirstOccurrence to jump to first valid occurrence >= startDate
 *    - This respects COUNT limit and returns -1 if recurrence already completed
 * 4. Iterate year by year (respecting INTERVAL):
 *    a. Check COUNT limit - stop if reached
 *    b. Check if current year exceeds endDate - stop if yes
 *    c. Check UNTIL limit - stop if exceeded
 *    d. Apply BYMONTH filter (if specified) - only generate on specified months
 *    e. Apply BYMONTHDAY filter (if specified) - only generate on specified days
 *    f. Create occurrence with same time as original event
 *    g. Add to results if within query range [startDate, endDate]
 *    h. Advance to next interval (year += interval)
 * 5. Return list of occurrences
 *
 * No hardcoded limits - stops naturally when:
 * - COUNT limit reached, OR
 * - UNTIL date exceeded, OR
 * - Past endDate (query range end)
 */
std::vector<CalendarEvent*> CalendarStreamParser::expandYearlyV2(CalendarEvent* event,
                                                                 const RRuleComponents& rule,
                                                                 time_t startDate,
                                                                 time_t endDate) {
    std::vector<CalendarEvent*> occurrences;

    DEBUG_VERBOSE_PRINTLN(">>> expandYearlyV2: Starting YEARLY expansion");
    DEBUG_VERBOSE_PRINTF("    Event: %s\n", event->summary.c_str());
    DEBUG_VERBOSE_PRINTF("    RRULE: %s\n", event->rrule.c_str());

    // Print rule components (verbose)
    DEBUG_VERBOSE_PRINTF("    Frequency: %s\n", rule.freq.c_str());
    DEBUG_VERBOSE_PRINTF("    Interval: %d\n", rule.interval);
    DEBUG_VERBOSE_PRINTF("    COUNT: %d\n", rule.count);
    DEBUG_VERBOSE_PRINTF("    UNTIL: %ld\n", rule.until);
    DEBUG_VERBOSE_PRINTF("    BYMONTH: %s\n", rule.byMonth.c_str());
    DEBUG_VERBOSE_PRINTF("    BYMONTHDAY: %s\n", rule.byMonthDay.c_str());

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    // Print query range (verbose)
    struct tm* startTm = localtime(&startDate);
    struct tm* endTm   = localtime(&endDate);
    DEBUG_VERBOSE_PRINTF("    Query range: %04d-%02d-%02d to %04d-%02d-%02d\n",
                         startTm->tm_year + 1900,
                         startTm->tm_mon + 1,
                         startTm->tm_mday,
                         endTm->tm_year + 1900,
                         endTm->tm_mon + 1,
                         endTm->tm_mday);

    // Print event start (verbose)
    struct tm* eventTm = localtime(&event->startTime);
    DEBUG_VERBOSE_PRINTF("    Event starts: %04d-%02d-%02d %02d:%02d:%02d\n",
                         eventTm->tm_year + 1900,
                         eventTm->tm_mon + 1,
                         eventTm->tm_mday,
                         eventTm->tm_hour,
                         eventTm->tm_min,
                         eventTm->tm_sec);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 1: Calculate event duration
    // ========================================================================
    time_t duration = event->endTime - event->startTime;
    DEBUG_VERBOSE_PRINTF("    Duration: %ld seconds\n", duration);

    // ========================================================================
    // Step 2: Determine effective end date (min of query endDate and UNTIL)
    // ========================================================================
    time_t effectiveEndDate = endDate;
    if (rule.until > 0 && rule.until < effectiveEndDate) {
        effectiveEndDate = rule.until;
        DEBUG_VERBOSE_PRINTLN("    Using UNTIL as effective end date");
    }

    // ========================================================================
    // Step 3: Use findFirstOccurrence to jump to first valid year
    // ========================================================================
    int interval = (rule.interval > 0) ? rule.interval : 1;
    int count    = (rule.count > 0) ? rule.count : -1;

    DEBUG_VERBOSE_PRINTF(
        "    Calling findFirstOccurrence with interval=%d, count=%d\n", interval, count);

    time_t firstOccurrence = findFirstOccurrence(event->startTime,
                                                 startDate,
                                                 effectiveEndDate,
                                                 interval,
                                                 RecurrenceFrequency::YEARLY,
                                                 count);

    if (firstOccurrence < 0) {
        DEBUG_VERBOSE_PRINTLN(
            "    findFirstOccurrence returned -1 (recurrence completed or outside range)");
        return occurrences;
    }

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* firstTm = localtime(&firstOccurrence);
    DEBUG_VERBOSE_PRINTF("    First occurrence: %04d-%02d-%02d %02d:%02d:%02d\n",
                         firstTm->tm_year + 1900,
                         firstTm->tm_mon + 1,
                         firstTm->tm_mday,
                         firstTm->tm_hour,
                         firstTm->tm_min,
                         firstTm->tm_sec);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 4: Parse BYMONTH and BYMONTHDAY filters if present
    // ========================================================================
    std::vector<int> byMonthList    = parseByMonth(rule.byMonth);
    std::vector<int> byMonthDayList = parseByMonthDay(rule.byMonthDay);

    if (!byMonthList.empty()) {
        DEBUG_VERBOSE_PRINTF("    BYMONTH filter: %d month(s) specified\n",
                             (int)byMonthList.size());
        for (size_t i = 0; i < byMonthList.size(); i++) {
            DEBUG_VERBOSE_PRINTF("      Month: %d\n", byMonthList[i]);
        }
    } else {
        DEBUG_VERBOSE_PRINTLN("    BYMONTH filter: none (all months)");
    }

    if (!byMonthDayList.empty()) {
        DEBUG_VERBOSE_PRINTF("    BYMONTHDAY filter: %d day(s) specified\n",
                             (int)byMonthDayList.size());
        for (size_t i = 0; i < byMonthDayList.size(); i++) {
            DEBUG_VERBOSE_PRINTF("      Day: %d\n", byMonthDayList[i]);
        }
    } else {
        DEBUG_VERBOSE_PRINTLN("    BYMONTHDAY filter: none (all days)");
    }

    // ========================================================================
    // Step 5: Calculate starting occurrence index and iterate year by year
    // ========================================================================

    // Calculate how many occurrences happened before firstOccurrence
    int occurrencesBeforeRange = 0;
    if (count > 0 && firstOccurrence > event->startTime) {
        struct tm* eventStartTm = localtime(&event->startTime);
        struct tm* firstOccTm   = localtime(&firstOccurrence);
        int yearsDiff           = (firstOccTm->tm_year + 1900) - (eventStartTm->tm_year + 1900);
        occurrencesBeforeRange  = yearsDiff / interval;
        DEBUG_VERBOSE_PRINTF("    Occurrences before query range: %d\n", occurrencesBeforeRange);
    }

    // Start iterating from firstOccurrence
    struct tm currentTm;
    memcpy(&currentTm, localtime(&firstOccurrence), sizeof(struct tm));

    int absoluteOccurrenceIndex = occurrencesBeforeRange; // Track absolute occurrence number
    int maxCount                = (count > 0) ? count : INT_MAX;

    DEBUG_VERBOSE_PRINTLN(">>> expandYearlyV2: Starting year-by-year iteration");

    while (true) {
        // Create occurrence timestamp
        time_t occurrenceTime = mktime(&currentTm);

        // Check if past effective end date
        if (occurrenceTime > effectiveEndDate) {
            DEBUG_VERBOSE_PRINTLN("    Reached effective end date, stopping");
            break;
        }

        // Apply BYMONTH filter (if specified)
        bool monthMatches =
            byMonthList.empty() ||
            (std::find(byMonthList.begin(), byMonthList.end(), currentTm.tm_mon + 1) !=
             byMonthList.end());

        // Apply BYMONTHDAY filter (if specified)
        bool dayMatches =
            byMonthDayList.empty() ||
            (std::find(byMonthDayList.begin(), byMonthDayList.end(), currentTm.tm_mday) !=
             byMonthDayList.end());

        if (monthMatches && dayMatches) {
            absoluteOccurrenceIndex++;

            DEBUG_VERBOSE_PRINTF("    Occurrence #%d: %04d-%02d-%02d\n",
                                 absoluteOccurrenceIndex,
                                 currentTm.tm_year + 1900,
                                 currentTm.tm_mon + 1,
                                 currentTm.tm_mday);

            // Check COUNT limit
            if (absoluteOccurrenceIndex > maxCount) {
                DEBUG_VERBOSE_PRINTLN("    Reached COUNT limit, stopping");
                break;
            }

            // Only add to results if within query range
            if (occurrenceTime >= startDate && occurrenceTime <= endDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime     = occurrenceTime;
                occurrence->endTime       = occurrenceTime + duration;

                // Set date string for display
                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &currentTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
                DEBUG_VERBOSE_PRINTLN("      -> Added to results");
            } else {
                DEBUG_VERBOSE_PRINTLN("      -> Outside query range, not added");
            }
        }

        // Advance to next year (respecting interval)
        currentTm.tm_year += interval;
        mktime(&currentTm); // Normalize
    }

    DEBUG_INFO_PRINTF(">>> expandYearlyV2: Complete. Created %d occurrences\n",
                      (int)occurrences.size());
    return occurrences;
}

/**
 * V2: Expand MONTHLY recurring events
 * TODO: Implementation pending
 */
std::vector<CalendarEvent*> CalendarStreamParser::expandMonthlyV2(CalendarEvent* event,
                                                                  const RRuleComponents& rule,
                                                                  time_t startDate,
                                                                  time_t endDate) {
    std::vector<CalendarEvent*> occurrences;

    DEBUG_VERBOSE_PRINTLN(">>> expandMonthlyV2: Starting MONTHLY expansion");

    // ========================================================================
    // Step 1: Calculate event duration
    // ========================================================================
    time_t duration = event->endTime - event->startTime;
    DEBUG_VERBOSE_PRINTF("    Duration: %ld seconds\n", duration);

    // ========================================================================
    // Step 2: Determine effective end date (respect UNTIL if present)
    // ========================================================================
    time_t effectiveEndDate = endDate;
    if (rule.until > 0 && rule.until < effectiveEndDate) {
        effectiveEndDate = rule.until;
        DEBUG_VERBOSE_PRINTLN("    Using UNTIL as effective end date");
    }

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* startTm = localtime(&startDate);
    struct tm* endTm   = localtime(&effectiveEndDate);
    struct tm* eventTm = localtime(&event->startTime);

    DEBUG_VERBOSE_PRINTF("    Query range: %04d-%02d-%02d to %04d-%02d-%02d\n",
                         startTm->tm_year + 1900,
                         startTm->tm_mon + 1,
                         startTm->tm_mday,
                         endTm->tm_year + 1900,
                         endTm->tm_mon + 1,
                         endTm->tm_mday);

    DEBUG_VERBOSE_PRINTF("    Event starts: %04d-%02d-%02d %02d:%02d:%02d\n",
                         eventTm->tm_year + 1900,
                         eventTm->tm_mon + 1,
                         eventTm->tm_mday,
                         eventTm->tm_hour,
                         eventTm->tm_min,
                         eventTm->tm_sec);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 3: Use findFirstOccurrence to jump to first valid month
    // ========================================================================
    int interval = (rule.interval > 0) ? rule.interval : 1;
    int count    = (rule.count > 0) ? rule.count : -1;

    DEBUG_VERBOSE_PRINTF(
        "    Calling findFirstOccurrence with interval=%d, count=%d\n", interval, count);

    time_t firstOccurrence = findFirstOccurrence(event->startTime,
                                                 startDate,
                                                 effectiveEndDate,
                                                 interval,
                                                 RecurrenceFrequency::MONTHLY,
                                                 count);

    if (firstOccurrence < 0) {
        DEBUG_VERBOSE_PRINTLN(
            "    findFirstOccurrence returned -1 (recurrence completed or outside range)");
        return occurrences;
    }

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* firstTm = localtime(&firstOccurrence);
    DEBUG_VERBOSE_PRINTF("    First occurrence: %04d-%02d-%02d %02d:%02d:%02d\n",
                         firstTm->tm_year + 1900,
                         firstTm->tm_mon + 1,
                         firstTm->tm_mday,
                         firstTm->tm_hour,
                         firstTm->tm_min,
                         firstTm->tm_sec);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 4: Parse BYMONTHDAY filter if present
    // ========================================================================
    std::vector<int> byMonthDayList = parseByMonthDay(rule.byMonthDay);

    if (!byMonthDayList.empty()) {
        DEBUG_VERBOSE_PRINTF("    BYMONTHDAY filter: %d day(s) specified\n",
                             (int)byMonthDayList.size());
        for (size_t i = 0; i < byMonthDayList.size(); i++) {
            DEBUG_VERBOSE_PRINTF("      Day: %d\n", byMonthDayList[i]);
        }
    } else {
        DEBUG_VERBOSE_PRINTLN("    BYMONTHDAY filter: none (use original day of month)");
    }

    // ========================================================================
    // Step 5: Calculate starting occurrence index and iterate month by month
    // ========================================================================

    // Calculate how many occurrences happened before firstOccurrence
    int occurrencesBeforeRange = 0;
    if (count > 0 && firstOccurrence > event->startTime) {
        struct tm* eventStartTm = localtime(&event->startTime);
        struct tm* firstOccTm   = localtime(&firstOccurrence);

        // Calculate months difference
        int yearsDiff          = (firstOccTm->tm_year) - (eventStartTm->tm_year);
        int monthsDiff         = yearsDiff * 12 + (firstOccTm->tm_mon - eventStartTm->tm_mon);

        occurrencesBeforeRange = monthsDiff / interval;
        DEBUG_VERBOSE_PRINTF("    Occurrences before query range: %d\n", occurrencesBeforeRange);
    }

    // Start iterating from firstOccurrence
    struct tm currentTm;
    memcpy(&currentTm, localtime(&firstOccurrence), sizeof(struct tm));

    int absoluteOccurrenceIndex = occurrencesBeforeRange; // Track absolute occurrence number
    int maxCount                = (count > 0) ? count : INT_MAX;

    DEBUG_VERBOSE_PRINTLN(">>> expandMonthlyV2: Starting month-by-month iteration");

    while (true) {
        // Create occurrence timestamp
        time_t occurrenceTime = mktime(&currentTm);

        // Check if past effective end date
        if (occurrenceTime > effectiveEndDate) {
            DEBUG_VERBOSE_PRINTLN("    Reached effective end date, stopping");
            break;
        }

        // Apply BYMONTHDAY filter (if specified)
        // Note: For MONTHLY recurrence, we don't filter by month (already iterating monthly)
        bool dayMatches =
            byMonthDayList.empty() ||
            (std::find(byMonthDayList.begin(), byMonthDayList.end(), currentTm.tm_mday) !=
             byMonthDayList.end());

        if (dayMatches) {
            absoluteOccurrenceIndex++;

            DEBUG_VERBOSE_PRINTF("    Occurrence #%d: %04d-%02d-%02d\n",
                                 absoluteOccurrenceIndex,
                                 currentTm.tm_year + 1900,
                                 currentTm.tm_mon + 1,
                                 currentTm.tm_mday);

            // Check COUNT limit
            if (absoluteOccurrenceIndex > maxCount) {
                DEBUG_VERBOSE_PRINTLN("    Reached COUNT limit, stopping");
                break;
            }

            // Only add to results if within query range
            if (occurrenceTime >= startDate && occurrenceTime <= endDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime     = occurrenceTime;
                occurrence->endTime       = occurrenceTime + duration;

                // Set date string for display
                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &currentTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
                DEBUG_VERBOSE_PRINTLN("      -> Added to results");
            } else {
                DEBUG_VERBOSE_PRINTLN("      -> Outside query range, not added");
            }
        }

        // Advance to next month (respecting interval)
        currentTm.tm_mon += interval;
        mktime(&currentTm); // Normalize (handles month/year overflow)
    }

    DEBUG_INFO_PRINTF(">>> expandMonthlyV2: Complete. Created %d occurrences\n",
                      (int)occurrences.size());

    return occurrences;
}

/**
 * V2: Expand WEEKLY recurring events
 * TODO: Implementation pending
 */
std::vector<CalendarEvent*> CalendarStreamParser::expandWeeklyV2(CalendarEvent* event,
                                                                 const RRuleComponents& rule,
                                                                 time_t startDate,
                                                                 time_t endDate) {
    std::vector<CalendarEvent*> occurrences;

    DEBUG_VERBOSE_PRINTLN(">>> expandWeeklyV2: Starting WEEKLY expansion");

    // ========================================================================
    // Step 1: Calculate event duration
    // ========================================================================
    time_t duration = event->endTime - event->startTime;
    DEBUG_VERBOSE_PRINTF("    Duration: %ld seconds\n", duration);

    // ========================================================================
    // Step 2: Determine effective end date (respect UNTIL if present)
    // ========================================================================
    time_t effectiveEndDate = endDate;
    if (rule.until > 0 && rule.until < effectiveEndDate) {
        effectiveEndDate = rule.until;
        DEBUG_VERBOSE_PRINTLN("    Using UNTIL as effective end date");
    }

    // Use GMT/UTC consistently to avoid timezone conversion issues
    struct tm eventTmCopy;
    memcpy(&eventTmCopy, gmtime(&event->startTime), sizeof(struct tm));
    struct tm* eventTm = &eventTmCopy;
#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* startTm = gmtime(&startDate);
    struct tm* endTm   = gmtime(&effectiveEndDate);

    DEBUG_VERBOSE_PRINTF("    Query range: %04d-%02d-%02d to %04d-%02d-%02d\n",
                         startTm->tm_year + 1900,
                         startTm->tm_mon + 1,
                         startTm->tm_mday,
                         endTm->tm_year + 1900,
                         endTm->tm_mon + 1,
                         endTm->tm_mday);

    DEBUG_VERBOSE_PRINTF("    Event starts: %04d-%02d-%02d %02d:%02d:%02d (day: %d)\n",
                         eventTm->tm_year + 1900,
                         eventTm->tm_mon + 1,
                         eventTm->tm_mday,
                         eventTm->tm_hour,
                         eventTm->tm_min,
                         eventTm->tm_sec,
                         eventTm->tm_wday);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 3: Use findFirstOccurrence to jump to first valid week
    // ========================================================================
    int interval = (rule.interval > 0) ? rule.interval : 1;
    int count    = (rule.count > 0) ? rule.count : -1;

    DEBUG_VERBOSE_PRINTF(
        "    Calling findFirstOccurrence with interval=%d, count=%d\n", interval, count);

    time_t firstOccurrence = findFirstOccurrence(event->startTime,
                                                 startDate,
                                                 effectiveEndDate,
                                                 interval,
                                                 RecurrenceFrequency::WEEKLY,
                                                 count);

    if (firstOccurrence < 0) {
        DEBUG_VERBOSE_PRINTLN(
            "    findFirstOccurrence returned -1 (recurrence completed or outside range)");
        return occurrences;
    }

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* firstTm = gmtime(&firstOccurrence);
    DEBUG_VERBOSE_PRINTF("    First occurrence: %04d-%02d-%02d %02d:%02d:%02d (day: %d)\n",
                         firstTm->tm_year + 1900,
                         firstTm->tm_mon + 1,
                         firstTm->tm_mday,
                         firstTm->tm_hour,
                         firstTm->tm_min,
                         firstTm->tm_sec,
                         firstTm->tm_wday);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 4: Parse BYDAY filter if present
    // ========================================================================
    std::vector<int> byDayList = parseByDay(rule.byDay);

    if (!byDayList.empty()) {
        // Sort byDayList to ensure days are processed in chronological order (Sun=0, Mon=1, ...,
        // Sat=6)
        std::sort(byDayList.begin(), byDayList.end());

        DEBUG_VERBOSE_PRINTF("    BYDAY filter: %d day(s) specified\n", (int)byDayList.size());
        for (size_t i = 0; i < byDayList.size(); i++) {
            DEBUG_VERBOSE_PRINTF("      Day: %d (0=Sun, 1=Mon, ..., 6=Sat)\n", byDayList[i]);
        }
    } else {
        DEBUG_VERBOSE_PRINTLN("    BYDAY filter: none (use event's original day of week)");
        // If no BYDAY, use the event's original day of week
        byDayList.push_back(eventTm->tm_wday);
    }

    // ========================================================================
    // Step 5: Calculate starting occurrence index and iterate week by week
    // ========================================================================

    // Calculate how many occurrences happened before firstOccurrence
    int occurrencesBeforeRange = 0;
    if (count > 0 && firstOccurrence > event->startTime) {
        // Calculate weeks difference
        int daysDiff  = (firstOccurrence - event->startTime) / (24 * 3600);
        int weeksDiff = daysDiff / 7;

        // Each week can have multiple occurrences based on BYDAY
        occurrencesBeforeRange = (weeksDiff / interval) * byDayList.size();
        DEBUG_VERBOSE_PRINTF("    Occurrences before query range: %d\n", occurrencesBeforeRange);
    }

    // Start iterating from the beginning of the week containing firstOccurrence
    struct tm currentTm;
    memcpy(&currentTm, gmtime(&firstOccurrence), sizeof(struct tm));

    // Move back to Sunday (start of week) to check all BYDAY days in this week
    int daysToSunday = currentTm.tm_wday;
    currentTm.tm_mday -= daysToSunday;
    portable_timegm(&currentTm); // Normalize

    int absoluteOccurrenceIndex = occurrencesBeforeRange;
    int maxCount                = (count > 0) ? count : INT_MAX;

    DEBUG_VERBOSE_PRINTLN(">>> expandWeeklyV2: Starting week-by-week iteration");

    while (true) {
        // For each week, check each day in BYDAY list
        for (size_t i = 0; i < byDayList.size(); i++) {
            int targetDay = byDayList[i]; // 0=Sun, 1=Mon, ..., 6=Sat

            // Calculate date for this target day in current week
            struct tm occurrenceTm = currentTm;
            occurrenceTm.tm_mday += targetDay;
            time_t occurrenceTime = portable_timegm(&occurrenceTm);

            // Skip if this occurrence is before event start time
            if (occurrenceTime < event->startTime) {
                continue;
            }

            // Check if past effective end date
            if (occurrenceTime > effectiveEndDate) {
                DEBUG_VERBOSE_PRINTLN("    Reached effective end date, stopping");
                goto done; // Break out of nested loop
            }

            // This is a valid occurrence
            absoluteOccurrenceIndex++;

            DEBUG_VERBOSE_PRINTF("    Occurrence #%d: %04d-%02d-%02d (day %d)\n",
                                 absoluteOccurrenceIndex,
                                 occurrenceTm.tm_year + 1900,
                                 occurrenceTm.tm_mon + 1,
                                 occurrenceTm.tm_mday,
                                 occurrenceTm.tm_wday);

            // Check COUNT limit
            if (absoluteOccurrenceIndex > maxCount) {
                DEBUG_VERBOSE_PRINTLN("    Reached COUNT limit, stopping");
                goto done; // Break out of nested loop
            }

            // Only add to results if within query range
            if (occurrenceTime >= startDate && occurrenceTime <= endDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime     = occurrenceTime;
                occurrence->endTime       = occurrenceTime + duration;

                // Set date string for display
                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &occurrenceTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
                DEBUG_VERBOSE_PRINTLN("      -> Added to results");
            } else {
                DEBUG_VERBOSE_PRINTLN("      -> Outside query range, not added");
            }
        }

        // Advance to next week (respecting interval)
        currentTm.tm_mday += interval * 7;
        portable_timegm(&currentTm); // Normalize
    }

done:
    DEBUG_INFO_PRINTF(">>> expandWeeklyV2: Complete. Created %d occurrences\n",
                      (int)occurrences.size());

    return occurrences;
}

/**
 * V2: Expand DAILY recurring events
 * TODO: Implementation pending
 */
std::vector<CalendarEvent*> CalendarStreamParser::expandDailyV2(CalendarEvent* event,
                                                                const RRuleComponents& rule,
                                                                time_t startDate,
                                                                time_t endDate) {
    std::vector<CalendarEvent*> occurrences;

    DEBUG_VERBOSE_PRINTLN(">>> expandDailyV2: Starting DAILY expansion");

    // ========================================================================
    // Step 1: Calculate event duration
    // ========================================================================
    time_t duration = event->endTime - event->startTime;
    DEBUG_VERBOSE_PRINTF("    Duration: %ld seconds\n", duration);

    // ========================================================================
    // Step 2: Determine effective end date (respect UNTIL if present)
    // ========================================================================
    time_t effectiveEndDate = endDate;
    if (rule.until > 0 && rule.until < effectiveEndDate) {
        effectiveEndDate = rule.until;
        DEBUG_VERBOSE_PRINTLN("    Using UNTIL as effective end date");
    }

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* startTm = localtime(&startDate);
    struct tm* endTm   = localtime(&effectiveEndDate);
    struct tm* eventTm = localtime(&event->startTime);

    DEBUG_VERBOSE_PRINTF("    Query range: %04d-%02d-%02d to %04d-%02d-%02d\n",
                         startTm->tm_year + 1900,
                         startTm->tm_mon + 1,
                         startTm->tm_mday,
                         endTm->tm_year + 1900,
                         endTm->tm_mon + 1,
                         endTm->tm_mday);

    DEBUG_VERBOSE_PRINTF("    Event starts: %04d-%02d-%02d %02d:%02d:%02d\n",
                         eventTm->tm_year + 1900,
                         eventTm->tm_mon + 1,
                         eventTm->tm_mday,
                         eventTm->tm_hour,
                         eventTm->tm_min,
                         eventTm->tm_sec);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 3: Use findFirstOccurrence to jump to first valid day
    // ========================================================================
    int interval = (rule.interval > 0) ? rule.interval : 1;
    int count    = (rule.count > 0) ? rule.count : -1;

    DEBUG_VERBOSE_PRINTF(
        "    Calling findFirstOccurrence with interval=%d, count=%d\n", interval, count);

    time_t firstOccurrence = findFirstOccurrence(
        event->startTime, startDate, effectiveEndDate, interval, RecurrenceFrequency::DAILY, count);

    if (firstOccurrence < 0) {
        DEBUG_VERBOSE_PRINTLN(
            "    findFirstOccurrence returned -1 (recurrence completed or outside range)");
        return occurrences;
    }

#if DEBUG_LEVEL >= DEBUG_VERBOSE
    struct tm* firstTm = localtime(&firstOccurrence);
    DEBUG_VERBOSE_PRINTF("    First occurrence: %04d-%02d-%02d %02d:%02d:%02d\n",
                         firstTm->tm_year + 1900,
                         firstTm->tm_mon + 1,
                         firstTm->tm_mday,
                         firstTm->tm_hour,
                         firstTm->tm_min,
                         firstTm->tm_sec);
#endif // DEBUG_LEVEL >= DEBUG_VERBOSE

    // ========================================================================
    // Step 4: Parse BYDAY filter if present (to limit to specific weekdays)
    // ========================================================================
    std::vector<int> byDayList = parseByDay(rule.byDay);

    if (!byDayList.empty()) {
        // Sort byDayList to ensure days are in order
        std::sort(byDayList.begin(), byDayList.end());

        DEBUG_VERBOSE_PRINTF(
            "    BYDAY filter: %d day(s) specified (limits to specific weekdays)\n",
            (int)byDayList.size());
        for (size_t i = 0; i < byDayList.size(); i++) {
            DEBUG_VERBOSE_PRINTF("      Day: %d (0=Sun, 1=Mon, ..., 6=Sat)\n", byDayList[i]);
        }
    } else {
        DEBUG_VERBOSE_PRINTLN("    BYDAY filter: none (all days)");
    }

    // ========================================================================
    // Step 5: Calculate starting occurrence index and iterate day by day
    // ========================================================================

    // Calculate how many occurrences happened before firstOccurrence
    int occurrencesBeforeRange = 0;
    if (count > 0 && firstOccurrence > event->startTime) {
        // Calculate days difference
        int daysDiff           = (firstOccurrence - event->startTime) / (24 * 3600);
        occurrencesBeforeRange = daysDiff / interval;
        DEBUG_VERBOSE_PRINTF("    Occurrences before query range: %d\n", occurrencesBeforeRange);
    }

    // Start iterating from firstOccurrence
    struct tm currentTm;
    memcpy(&currentTm, localtime(&firstOccurrence), sizeof(struct tm));

    int absoluteOccurrenceIndex = occurrencesBeforeRange;
    int maxCount                = (count > 0) ? count : INT_MAX;

    DEBUG_VERBOSE_PRINTLN(">>> expandDailyV2: Starting day-by-day iteration");

    while (true) {
        // Create occurrence timestamp
        time_t occurrenceTime = mktime(&currentTm);

        // Check if past effective end date
        if (occurrenceTime > effectiveEndDate) {
            DEBUG_VERBOSE_PRINTLN("    Reached effective end date, stopping");
            break;
        }

        // Apply BYDAY filter (if specified) - only include days in the list
        bool dayMatches =
            byDayList.empty() ||
            (std::find(byDayList.begin(), byDayList.end(), currentTm.tm_wday) != byDayList.end());

        if (dayMatches) {
            absoluteOccurrenceIndex++;

            DEBUG_VERBOSE_PRINTF("    Occurrence #%d: %04d-%02d-%02d (day %d)\n",
                                 absoluteOccurrenceIndex,
                                 currentTm.tm_year + 1900,
                                 currentTm.tm_mon + 1,
                                 currentTm.tm_mday,
                                 currentTm.tm_wday);

            // Check COUNT limit
            if (absoluteOccurrenceIndex > maxCount) {
                DEBUG_VERBOSE_PRINTLN("    Reached COUNT limit, stopping");
                break;
            }

            // Only add to results if within query range
            if (occurrenceTime >= startDate && occurrenceTime <= endDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime     = occurrenceTime;
                occurrence->endTime       = occurrenceTime + duration;

                // Set date string for display
                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &currentTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
                DEBUG_VERBOSE_PRINTLN("      -> Added to results");
            } else {
                DEBUG_VERBOSE_PRINTLN("      -> Outside query range, not added");
            }
        }

        // Advance to next day (respecting interval)
        currentTm.tm_mday += interval;
        mktime(&currentTm); // Normalize (handles month/year overflow)
    }

    DEBUG_INFO_PRINTF(">>> expandDailyV2: Complete. Created %d occurrences\n",
                      (int)occurrences.size());

    return occurrences;
}

// ============================================================================
// OptimizedCalendarManager Implementation
// ============================================================================

OptimizedCalendarManager::OptimizedCalendarManager() :
    debug(false),
    cacheEnabled(true),
    cacheDuration(3600),
    cacheStartDate(0),
    cacheEndDate(0),
    cacheTimestamp(0) {}

OptimizedCalendarManager::~OptimizedCalendarManager() {
    clearCache();

    for (auto parser : parsers) {
        delete parser;
    }
    parsers.clear();
}

void OptimizedCalendarManager::addCalendar(const CalendarSource& source) {
    calendars.push_back(source);

    CalendarStreamParser* parser = new CalendarStreamParser();
    parser->setDebug(debug);
    parser->setCalendarColor(source.color);
    parser->setCalendarName(source.name);
    parsers.push_back(parser);
}

std::vector<CalendarEvent*>
OptimizedCalendarManager::getEventsForRange(time_t startDate,
                                            time_t endDate,
                                            size_t maxEventsPerCalendar) {
    // Check cache first
    if (cacheEnabled && isCacheValid(startDate, endDate)) {
        DEBUG_INFO_PRINTLN("Using cached events");
        /// Return copy of cached events
        std::vector<CalendarEvent*> result;
        for (auto event : cachedEvents) {
            result.push_back(new CalendarEvent(*event));
        }
        return result;
    }

    // Clear old cache
    clearCache();

    // Fetch events from all calendars
    std::vector<std::vector<CalendarEvent*>> allEvents;

    for (size_t i = 0; i < calendars.size(); i++) {
        if (!calendars[i].enabled) {
            continue;
        }
        DEBUG_INFO_PRINTLN("Fetching from: " + calendars[i].name);
        // Calculate date range based on calendar's days_to_fetch
        time_t calStartDate = startDate;
        time_t calEndDate   = endDate;

        if (calendars[i].days_to_fetch > 0) {
            // Extend range based on days_to_fetch
            calEndDate = startDate + (calendars[i].days_to_fetch * 86400);
            if (calEndDate < endDate) {
                calEndDate = endDate;
            }
        }

        // Fetch events for this calendar
        FilteredEvents* filtered = parsers[i]->fetchEventsInRange(
            calendars[i].url, calStartDate, calEndDate, maxEventsPerCalendar);

        if (filtered && filtered->success) {
            allEvents.push_back(filtered->events);
            // Clear the events vector without deleting events
            // (ownership transferred to allEvents)
            filtered->events.clear();
        }

        delete filtered;
    }

    // Merge and sort all events
    std::vector<CalendarEvent*> mergedEvents = mergeAndSortEvents(allEvents);

    // Update cache if enabled
    if (cacheEnabled) {
        cacheStartDate = startDate;
        cacheEndDate   = endDate;
        cacheTimestamp = millis();

        // Store copy in cache
        for (auto event : mergedEvents) {
            cachedEvents.push_back(new CalendarEvent(*event));
        }
    }

    return mergedEvents;
}

std::vector<CalendarEvent*> OptimizedCalendarManager::getEventsForDay(time_t date) {
    // Get start and end of day
    struct tm* tm   = localtime(&date);
    tm->tm_hour     = 0;
    tm->tm_min      = 0;
    tm->tm_sec      = 0;
    time_t dayStart = mktime(tm);

    tm->tm_hour     = 23;
    tm->tm_min      = 59;
    tm->tm_sec      = 59;
    time_t dayEnd   = mktime(tm);

    return getEventsForRange(dayStart, dayEnd, 20);
}

void OptimizedCalendarManager::clearCalendars() {
    calendars.clear();

    for (auto parser : parsers) {
        delete parser;
    }
    parsers.clear();

    clearCache();
}

size_t OptimizedCalendarManager::getMemoryUsage() const {
    size_t usage = sizeof(*this);
    usage += calendars.size() * sizeof(CalendarSource);
    usage += parsers.size() * sizeof(CalendarStreamParser*);

    // Add cached events
    usage += cachedEvents.size() * sizeof(CalendarEvent*);
    for (auto event : cachedEvents) {
        usage += sizeof(CalendarEvent);
        usage += event->summary.length();
        usage += event->location.length();
        usage += event->description.length();
    }

    return usage;
}

bool OptimizedCalendarManager::isCacheValid(time_t startDate, time_t endDate) const {
    if (cachedEvents.empty()) {
        return false;
    }

    // Check if requested range is within cached range
    if (startDate < cacheStartDate || endDate > cacheEndDate) {
        return false;
    }

    // Check if cache has expired
    if (millis() - cacheTimestamp > (cacheDuration * 1000)) {
        return false;
    }

    return true;
}

void OptimizedCalendarManager::clearCache() {
    for (auto event : cachedEvents) {
        delete event;
    }
    cachedEvents.clear();
    cacheStartDate = 0;
    cacheEndDate   = 0;
    cacheTimestamp = 0;
}

std::vector<CalendarEvent*> OptimizedCalendarManager::mergeAndSortEvents(
    const std::vector<std::vector<CalendarEvent*>>& eventLists) {

    std::vector<CalendarEvent*> merged;

    for (const auto& list : eventLists) {
        merged.insert(merged.end(), list.begin(), list.end());
    }

    // Sort by start time
    std::sort(merged.begin(), merged.end(), [](CalendarEvent* a, CalendarEvent* b) {
        return a->startTime < b->startTime;
    });

    return merged;
}