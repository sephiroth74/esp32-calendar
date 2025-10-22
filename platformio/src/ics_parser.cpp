#include "ics_parser.h"
#include <algorithm>

#ifndef NATIVE_TEST
    #include <FS.h>
#endif

// PSRAM allocation macros
#ifdef NATIVE_TEST
    // For native testing, always use regular malloc
    #define PSRAM_AVAILABLE 0
    #define PSRAM_ALLOC(size) malloc(size)
    #define PSRAM_FREE(ptr) free(ptr)
#else
    #ifdef BOARD_HAS_PSRAM
        #define PSRAM_AVAILABLE 1
        extern "C" {
            void* ps_malloc(size_t size);
        }
        #define PSRAM_ALLOC(size) ps_malloc(size)
        #define PSRAM_FREE(ptr) free(ptr)
    #else
        #define PSRAM_AVAILABLE 0
        #define PSRAM_ALLOC(size) malloc(size)
        #define PSRAM_FREE(ptr) free(ptr)
    #endif
#endif

ICSParser::ICSParser()
    : errorCode(ICSParseError::NONE)
    , valid(false)
    , debug(false) {
}

ICSParser::~ICSParser() {
    clear();
}

void ICSParser::clear() {
    // Free allocated events
    for (auto event : events) {
        if (event) {
            // Call destructor explicitly since we used placement new
            event->~CalendarEvent();
            // Free the memory (works for both PSRAM and regular heap)
            PSRAM_FREE(event);
        }
    }
    events.clear();

    // Clear properties
    version = "";
    prodId = "";
    calScale = "";
    method = "";
    calendarName = "";
    calendarDesc = "";
    timezone = "";

    // Reset state
    valid = false;
    errorCode = ICSParseError::NONE;
    lastError = "";
}

bool ICSParser::loadFromFile(const String& filepath) {
    clear();

    // Check if LittleFS is mounted
    if (!LittleFS.begin(false)) {
        if (!LittleFS.begin(true)) {
            setError(ICSParseError::FILE_NOT_FOUND, "Failed to mount LittleFS");
            return false;
        }
    }

    // Check if file exists
    if (!LittleFS.exists(filepath)) {
        setError(ICSParseError::FILE_NOT_FOUND, "File not found: " + filepath);
        return false;
    }

    // Open the file
    File file = LittleFS.open(filepath, "r");
    if (!file) {
        setError(ICSParseError::FILE_NOT_FOUND, "Failed to open file: " + filepath);
        return false;
    }

    // Read file content
    String icsData = "";
    while (file.available()) {
        icsData += file.readString();
    }
    file.close();

    if (debug) {
        Serial.println("Loaded " + String(icsData.length()) + " bytes from " + filepath);
    }

    return parse(icsData);
}

bool ICSParser::loadFromString(const String& icsData) {
    clear();

    // Check if string is too large (>30KB) and might cause issues
    if (icsData.length() > 30000) {
        if (debug) {
            Serial.println("Warning: Large ICS file (" + String(icsData.length()) + " bytes), using chunked parsing");
        }
        // For large strings, we need to process differently
        // Create a stream-like interface from the string
        return parseInChunks(icsData);
    }

    return parse(icsData);
}

bool ICSParser::loadFromStream(Stream* stream) {
    clear();

    if (!stream) {
        setError(ICSParseError::STREAM_READ_ERROR, "Invalid stream");
        return false;
    }

    // Use streaming parser directly for large files
    return parseStreamLarge(stream);
}

bool ICSParser::parse(const String& icsData) {
    // First unfold any folded lines
    String unfoldedData = unfoldLines(icsData);

    // Check for basic structure
    if (unfoldedData.indexOf("BEGIN:VCALENDAR") == -1) {
        setError(ICSParseError::MISSING_BEGIN_CALENDAR, "Missing BEGIN:VCALENDAR");
        return false;
    }

    if (unfoldedData.indexOf("END:VCALENDAR") == -1) {
        setError(ICSParseError::MISSING_END_CALENDAR, "Missing END:VCALENDAR");
        return false;
    }

    // Parse header
    if (!parseHeader(unfoldedData)) {
        return false;
    }

    // Validate header
    if (!validateHeader()) {
        return false;
    }

    // Parse events to count them (basic implementation)
    if (!parseEvents(unfoldedData)) {
        return false;
    }

    valid = true;
    return true;
}

bool ICSParser::parseHeader(const String& icsData) {
    // Find the calendar header section (before first BEGIN:VEVENT)
    int calendarStart = icsData.indexOf("BEGIN:VCALENDAR");
    if (calendarStart == -1) {
        setError(ICSParseError::MISSING_BEGIN_CALENDAR, "Missing BEGIN:VCALENDAR");
        return false;
    }

    int firstEvent = icsData.indexOf("BEGIN:VEVENT");
    int headerEnd = (firstEvent != -1) ? firstEvent : icsData.indexOf("END:VCALENDAR");

    String headerSection = icsData.substring(calendarStart, headerEnd);

    // Parse standard properties
    version = extractValue(headerSection, "VERSION:");
    prodId = extractValue(headerSection, "PRODID:");
    calScale = extractValue(headerSection, "CALSCALE:");
    method = extractValue(headerSection, "METHOD:");

    // Parse non-standard but commonly used properties
    calendarName = extractValue(headerSection, "X-WR-CALNAME:");
    calendarDesc = extractValue(headerSection, "X-WR-CALDESC:");
    timezone = extractValue(headerSection, "X-WR-TIMEZONE:");

    // Set default calendar scale if not specified
    if (calScale.isEmpty()) {
        calScale = "GREGORIAN";
    }

    if (debug) {
        Serial.println("=== ICS Header Parsed ===");
        Serial.println("Version: " + version);
        Serial.println("ProdID: " + prodId);
        Serial.println("CalScale: " + calScale);
        Serial.println("Method: " + method);
        Serial.println("Calendar Name: " + calendarName);
        Serial.println("Calendar Desc: " + calendarDesc);
        Serial.println("Timezone: " + timezone);
    }

    return true;
}

bool ICSParser::validateHeader() {
    // Check required properties
    if (version.isEmpty()) {
        setError(ICSParseError::MISSING_VERSION, "Missing VERSION property");
        return false;
    }

    // Check version is 2.0
    if (version != "2.0") {
        setError(ICSParseError::UNSUPPORTED_VERSION, "Unsupported version: " + version + " (only 2.0 is supported)");
        return false;
    }

    if (prodId.isEmpty()) {
        setError(ICSParseError::MISSING_PRODID, "Missing PRODID property");
        return false;
    }

    return true;
}

String ICSParser::unfoldLines(const String& data) {
    String unfolded = "";
    String currentLine = "";

    int pos = 0;
    while (pos < data.length()) {
        char c = data[pos];

        if (c == '\r') {
            pos++;
            if (pos < data.length() && data[pos] == '\n') {
                pos++;
            }

            // Check if next line starts with space or tab (continuation)
            if (pos < data.length() && (data[pos] == ' ' || data[pos] == '\t')) {
                pos++; // Skip the space/tab
                // Continue building current line
            } else {
                // Line is complete
                if (!currentLine.isEmpty()) {
                    unfolded += currentLine + "\n";
                }
                currentLine = "";
            }
        } else if (c == '\n') {
            pos++;

            // Check if next line starts with space or tab (continuation)
            if (pos < data.length() && (data[pos] == ' ' || data[pos] == '\t')) {
                pos++; // Skip the space/tab
                // Continue building current line
            } else {
                // Line is complete
                if (!currentLine.isEmpty()) {
                    unfolded += currentLine + "\n";
                }
                currentLine = "";
            }
        } else {
            currentLine += c;
            pos++;
        }
    }

    // Add any remaining line
    if (!currentLine.isEmpty()) {
        unfolded += currentLine + "\n";
    }

    return unfolded;
}

String ICSParser::extractValue(const String& data, const String& property) {
    int pos = data.indexOf(property);
    if (pos == -1) {
        return "";
    }

    int valueStart = pos + property.length();
    int valueEnd = data.indexOf("\n", valueStart);
    if (valueEnd == -1) {
        valueEnd = data.length();
    }

    String value = data.substring(valueStart, valueEnd);
    value.trim();

    // Remove any remaining CR
    value.replace("\r", "");

    return value;
}

bool ICSParser::parseLine(const String& line, String& name, String& params, String& value) {
    // Format: NAME[;PARAMS]:VALUE

    int colonPos = line.indexOf(':');
    if (colonPos == -1) {
        return false;
    }

    String nameAndParams = line.substring(0, colonPos);
    value = line.substring(colonPos + 1);

    int semicolonPos = nameAndParams.indexOf(';');
    if (semicolonPos != -1) {
        name = nameAndParams.substring(0, semicolonPos);
        params = nameAndParams.substring(semicolonPos + 1);
    } else {
        name = nameAndParams;
        params = "";
    }

    // Trim whitespace
    name.trim();
    params.trim();
    value.trim();

    return true;
}

void ICSParser::setError(ICSParseError error, const String& message) {
    errorCode = error;
    lastError = message;
    valid = false;

    if (debug) {
        Serial.println("ICS Parse Error: " + message);
    }
}

CalendarEvent* ICSParser::getEvent(size_t index) const {
    if (index >= events.size()) {
        return nullptr;
    }
    return events[index];
}

std::vector<CalendarEvent*> ICSParser::getEvents(time_t start, time_t end) const {
    // Delegate to the new method
    return getEventsInRange(start, end);
}

std::vector<CalendarEvent*> ICSParser::getEventsInRange(time_t startDate, time_t endDate) const {
    std::vector<CalendarEvent*> result;

    for (auto event : events) {
        if (!event) continue;

        if (event->isRecurring) {
            // For recurring events, expand occurrences within the date range
            std::vector<CalendarEvent*> occurrences = expandRecurringEvent(event, startDate, endDate);
            result.insert(result.end(), occurrences.begin(), occurrences.end());
        } else {
            // For non-recurring events, check if they fall within the range
            if (isEventInRange(event, startDate, endDate)) {
                result.push_back(event);
            }
        }
    }

    // Sort events by start time
    std::sort(result.begin(), result.end(), [](CalendarEvent* a, CalendarEvent* b) {
        return a->startTime < b->startTime;
    });

    return result;
}

bool ICSParser::isEventInRange(CalendarEvent* event, time_t startDate, time_t endDate) const {
    if (!event) return false;

    // Get event start and end times
    time_t eventStart = event->startTime;
    time_t eventEnd = event->endTime;

    // If end time is not set, use start time
    if (eventEnd == 0) {
        eventEnd = eventStart;
    }

    // Check if event overlaps with the date range
    return (eventStart <= endDate) && (eventEnd >= startDate);
}

std::vector<CalendarEvent*> ICSParser::expandRecurringEvent(CalendarEvent* event, time_t startDate, time_t endDate) const {
    std::vector<CalendarEvent*> occurrences;

    if (!event || event->rrule.isEmpty()) {
        return occurrences;
    }

    // Parse RRULE
    String freq;
    int interval = 1;
    int count = 0;
    String until;

    if (!parseRRule(event->rrule, freq, interval, count, until)) {
        // If we can't parse the RRULE, treat as single event
        if (isEventInRange(event, startDate, endDate)) {
            occurrences.push_back(event);
        }
        return occurrences;
    }

    // Start from the event's original start time
    time_t originalStart = event->startTime;

    // If the original event is after our search range, no occurrences
    if (originalStart > endDate) {
        return occurrences;
    }

    // Calculate the first occurrence that might be in our range
    time_t currentTime = originalStart;

    // For events that started before our search range, advance to the first possible occurrence
    if (freq == "YEARLY") {
        // Calculate years to advance
        struct tm* eventTm = localtime(&originalStart);
        struct tm* searchTm = localtime(&startDate);

        int eventYear = eventTm->tm_year + 1900;
        int searchYear = searchTm->tm_year + 1900;

        if (eventYear < searchYear) {
            // Advance to the search year or later

            // Create a new time for the occurrence in the search year
            struct tm newTm = *eventTm;
            newTm.tm_year = searchTm->tm_year;

            // Keep the same month and day
            currentTime = mktime(&newTm);

            // If this occurrence is before the start date (e.g., event is in January, search starts in March)
            // advance to next year
            if (currentTime < startDate) {
                newTm.tm_year++;
                currentTime = mktime(&newTm);
            }
        }
    }

    // Now check occurrences from currentTime onwards
    int occurrenceCount = 0;
    const int MAX_OCCURRENCES = 365;
    bool foundInRange = false;

    while (currentTime <= endDate && occurrenceCount < MAX_OCCURRENCES) {
        // Check if this occurrence is within our date range
        if (currentTime >= startDate && currentTime <= endDate) {
            // For simplified implementation, just add the original event once
            // TODO: In a full implementation, create new CalendarEvent instances
            // for each occurrence with adjusted start/end times
            if (!foundInRange) {
                occurrences.push_back(event);
                foundInRange = true;
            }
        }

        // Calculate next occurrence based on frequency
        if (freq == "DAILY") {
            currentTime += (86400 * interval);  // 86400 seconds in a day
        } else if (freq == "WEEKLY") {
            currentTime += (86400 * 7 * interval);  // 7 days in a week
        } else if (freq == "MONTHLY") {
            // Advance by month
            struct tm* tm = localtime(&currentTime);
            tm->tm_mon += interval;
            currentTime = mktime(tm);
        } else if (freq == "YEARLY") {
            // Advance by year
            struct tm* tm = localtime(&currentTime);
            tm->tm_year += interval;
            currentTime = mktime(tm);
        } else {
            break;  // Unknown frequency
        }

        occurrenceCount++;

        // Check COUNT limit
        if (count > 0 && occurrenceCount >= count) {
            break;
        }

        // If we've found an occurrence and moved past the range, we can stop
        if (foundInRange && currentTime > endDate) {
            break;
        }
    }

    return occurrences;
}

bool ICSParser::parseRRule(const String& rrule, String& freq, int& interval, int& count, String& until) const {
    // Initialize defaults
    freq = "";
    interval = 1;
    count = 0;
    until = "";

    if (rrule.isEmpty()) {
        return false;
    }

    // Parse RRULE components (simplified)
    // Format: FREQ=YEARLY;INTERVAL=1;COUNT=10;UNTIL=20251231T235959Z

    // Extract FREQ
    int freqPos = rrule.indexOf("FREQ=");
    if (freqPos != -1) {
        int freqEnd = rrule.indexOf(";", freqPos);
        if (freqEnd == -1) {
            freqEnd = rrule.length();
        }
        freq = rrule.substring(freqPos + 5, freqEnd);
    }

    // Extract INTERVAL
    int intervalPos = rrule.indexOf("INTERVAL=");
    if (intervalPos != -1) {
        int intervalEnd = rrule.indexOf(";", intervalPos);
        if (intervalEnd == -1) {
            intervalEnd = rrule.length();
        }
        String intervalStr = rrule.substring(intervalPos + 9, intervalEnd);
        interval = intervalStr.toInt();
        if (interval <= 0) interval = 1;
    }

    // Extract COUNT
    int countPos = rrule.indexOf("COUNT=");
    if (countPos != -1) {
        int countEnd = rrule.indexOf(";", countPos);
        if (countEnd == -1) {
            countEnd = rrule.length();
        }
        String countStr = rrule.substring(countPos + 6, countEnd);
        count = countStr.toInt();
    }

    // Extract UNTIL
    int untilPos = rrule.indexOf("UNTIL=");
    if (untilPos != -1) {
        int untilEnd = rrule.indexOf(";", untilPos);
        if (untilEnd == -1) {
            untilEnd = rrule.length();
        }
        until = rrule.substring(untilPos + 6, untilEnd);
    }

    return !freq.isEmpty();
}

bool ICSParser::parseEvents(const String& icsData) {
    // Clear any existing events
    for (auto event : events) {
        if (event) {
            event->~CalendarEvent();
            PSRAM_FREE(event);
        }
    }
    events.clear();

    // Find all VEVENT blocks
    int pos = 0;
    while (pos < icsData.length()) {
        int beginPos = icsData.indexOf("BEGIN:VEVENT", pos);
        if (beginPos == -1) break;

        int endPos = icsData.indexOf("END:VEVENT", beginPos);
        if (endPos == -1) {
            setError(ICSParseError::INVALID_FORMAT, "Unclosed VEVENT block");
            return false;
        }

        // Extract the event data including END:VEVENT
        String eventData = icsData.substring(beginPos, endPos + 10);

        // Parse the event
        if (!parseEvent(eventData)) {
            // Continue parsing other events even if one fails
            Serial.println("Warning: Failed to parse an event, continuing...");
        }

        pos = endPos + 10;
    }

    if (debug) {
        Serial.println("Parsed " + String(events.size()) + " events");
    }

    return true;
}

bool ICSParser::parseEvent(const String& eventData) {
    // Create a new event - use PSRAM if available
    void* mem = PSRAM_ALLOC(sizeof(CalendarEvent));
    if (!mem) {
        setError(ICSParseError::MEMORY_ALLOCATION_FAILED, "Failed to allocate memory for event");
        return false;
    }

    // Use placement new to construct the object
    CalendarEvent* event = new(mem) CalendarEvent();

    if (debug && PSRAM_AVAILABLE) {
        Serial.println("Event allocated in PSRAM");
    }

    // Extract required fields

    // 1. SUMMARY (title/summary)
    event->summary = extractValue(eventData, "SUMMARY:");

    // 2. DTSTART (start date/time)
    String dtStart = extractValue(eventData, "DTSTART:");
    String dtStartTz = "";

    if (dtStart.isEmpty()) {
        // Try with VALUE=DATE for all-day events
        dtStart = extractValue(eventData, "DTSTART;VALUE=DATE:");
    }

    if (dtStart.isEmpty()) {
        // Try with explicit VALUE=DATE-TIME
        dtStart = extractValue(eventData, "DTSTART;VALUE=DATE-TIME:");
    }

    if (dtStart.isEmpty()) {
        // Try to find DTSTART with TZID
        int dtStartPos = eventData.indexOf("DTSTART;TZID=");
        if (dtStartPos != -1) {
            int colonPos = eventData.indexOf(":", dtStartPos);
            int newlinePos = eventData.indexOf("\n", dtStartPos);
            if (colonPos != -1 && (newlinePos == -1 || colonPos < newlinePos)) {
                // Extract timezone ID
                String params = eventData.substring(dtStartPos + 13, colonPos); // Skip "DTSTART;TZID="
                int semicolonPos = params.indexOf(";");
                if (semicolonPos != -1) {
                    dtStartTz = params.substring(0, semicolonPos);
                } else {
                    dtStartTz = params;
                }
                // Extract the datetime value
                dtStart = eventData.substring(colonPos + 1, newlinePos != -1 ? newlinePos : eventData.length());
                dtStart.trim();
                dtStart.replace("\r", "");
            }
        }
    }

    if (!dtStart.isEmpty()) {
        event->dtStart = dtStart;
        // Check if it's an all-day event
        event->allDay = (dtStart.length() == 8) || (eventData.indexOf("DTSTART;VALUE=DATE:") != -1);

        // Store timezone if found
        if (!dtStartTz.isEmpty()) {
            event->timezone = dtStartTz;
        }

        // Store the raw value
        event->setStartDateTime(dtStart);
    }

    // 3. DTEND (end date/time)
    String dtEnd = extractValue(eventData, "DTEND:");

    if (dtEnd.isEmpty()) {
        // Try with VALUE=DATE for all-day events
        dtEnd = extractValue(eventData, "DTEND;VALUE=DATE:");
    }

    if (dtEnd.isEmpty()) {
        // Try with explicit VALUE=DATE-TIME
        dtEnd = extractValue(eventData, "DTEND;VALUE=DATE-TIME:");
    }

    if (dtEnd.isEmpty()) {
        // Try to find DTEND with TZID (similar to DTSTART)
        int dtEndPos = eventData.indexOf("DTEND;TZID=");
        if (dtEndPos != -1) {
            int colonPos = eventData.indexOf(":", dtEndPos);
            int newlinePos = eventData.indexOf("\n", dtEndPos);
            if (colonPos != -1 && (newlinePos == -1 || colonPos < newlinePos)) {
                // Extract the datetime value
                dtEnd = eventData.substring(colonPos + 1, newlinePos != -1 ? newlinePos : eventData.length());
                dtEnd.trim();
                dtEnd.replace("\r", "");
            }
        }
    }

    if (!dtEnd.isEmpty()) {
        event->dtEnd = dtEnd;
        event->setEndDateTime(dtEnd);
    }

    // 4. CREATED (creation timestamp)
    event->created = extractValue(eventData, "CREATED:");

    // 5. LAST-MODIFIED (last modification timestamp)
    event->lastModified = extractValue(eventData, "LAST-MODIFIED:");

    // 6. DTSTAMP (timestamp of ICS creation)
    event->dtStamp = extractValue(eventData, "DTSTAMP:");

    // 7. Check if recurring (has RRULE)
    event->rrule = extractValue(eventData, "RRULE:");
    event->isRecurring = !event->rrule.isEmpty();

    // Also extract some other useful fields while we're at it
    event->uid = extractValue(eventData, "UID:");
    event->location = extractValue(eventData, "LOCATION:");
    event->description = extractValue(eventData, "DESCRIPTION:");
    event->status = extractValue(eventData, "STATUS:");
    event->transp = extractValue(eventData, "TRANSP:");

    // Add calendar metadata if available
    event->calendarName = calendarName;

    // Add the event to our list
    events.push_back(event);

    // Always log key event details for debugging
    Serial.println("=== Event Parsed ===");
    Serial.println("Summary: " + event->summary);
    Serial.println("Date Start: " + event->dtStart + (event->allDay ? " (All-day)" : ""));
    Serial.println("Recurring: " + String(event->isRecurring ? "Yes" : "No"));
    if (event->isRecurring && !event->rrule.isEmpty()) {
        Serial.println("Recurrence Rule: " + event->rrule);
    }

    if (debug) {
        // Additional debug information
        Serial.println("  End: " + event->dtEnd);
        Serial.println("  UID: " + event->uid);
        Serial.println("  Created: " + event->created);
        Serial.println("  Modified: " + event->lastModified);
        Serial.println("  Status: " + event->status);
        if (!event->location.isEmpty()) {
            Serial.println("  Location: " + event->location);
        }
    }

    return true;
}
void ICSParser::printMemoryInfo() const {
#ifndef NATIVE_TEST
    Serial.println("=== ICS Parser Memory Info ===");
    Serial.println("Events loaded: " + String(events.size()));
    
    if (PSRAM_AVAILABLE) {
        Serial.println("PSRAM enabled: Yes");
        #ifdef ESP32
        size_t psramSize = ESP.getPsramSize();
        size_t freePsram = ESP.getFreePsram();
        if (psramSize > 0) {
            Serial.println("PSRAM total: " + String(psramSize / 1024) + " KB");
            Serial.println("PSRAM free: " + String(freePsram / 1024) + " KB");
            Serial.println("Events stored in PSRAM");
        }
        #endif
    } else {
        Serial.println("PSRAM enabled: No");
        Serial.println("Events stored in regular heap");
    }
    
    #ifdef ESP32
    Serial.println("Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB");
    #endif
#endif
}

bool ICSParser::parseInChunks(const String& icsData) {
    // For large files, we process line by line without unfolding the entire file
    // This prevents memory issues with large ICS files

    if (debug) {
        Serial.println("=== Parsing ICS in chunks (large file mode) ===");
        Serial.println("Data size: " + String(icsData.length()) + " bytes");
    }

    // First, scan for header information without unfolding
    int headerEnd = icsData.indexOf("BEGIN:VEVENT");
    if (headerEnd == -1) {
        headerEnd = icsData.indexOf("END:VCALENDAR");
    }

    if (headerEnd > 0) {
        String headerSection = icsData.substring(0, headerEnd);
        // Unfold just the header section
        String unfoldedHeader = unfoldLines(headerSection);
        if (!parseHeader(unfoldedHeader)) {
            return false;
        }
        if (!validateHeader()) {
            return false;
        }
    }

    // Now parse events one by one
    int pos = 0;
    int eventCount = 0;
    const int MAX_EVENT_SIZE = 8192; // Maximum size of a single event block

    while (pos < icsData.length()) {
        // Find the next event
        int beginPos = icsData.indexOf("BEGIN:VEVENT", pos);
        if (beginPos == -1) break;

        int endPos = icsData.indexOf("END:VEVENT", beginPos);
        if (endPos == -1) {
            setError(ICSParseError::INVALID_FORMAT, "Unclosed VEVENT block");
            return false;
        }

        // Include "END:VEVENT" in the substring
        endPos += 10;

        // Check if event is too large
        int eventSize = endPos - beginPos;
        if (eventSize > MAX_EVENT_SIZE) {
            if (debug) {
                Serial.println("Warning: Skipping oversized event (" + String(eventSize) + " bytes)");
            }
            pos = endPos;
            continue;
        }

        // Extract and unfold just this event
        String eventData = icsData.substring(beginPos, endPos);
        String unfoldedEvent = unfoldLines(eventData);

        // Parse the event
        if (parseEvent(unfoldedEvent)) {
            eventCount++;
            if (debug && (eventCount % 10 == 0)) {
                Serial.println("Parsed " + String(eventCount) + " events...");
            }
        } else {
            if (debug) {
                Serial.println("Warning: Failed to parse event at position " + String(beginPos));
            }
        }

        pos = endPos;

        // Yield periodically to avoid watchdog reset
        if (eventCount % 5 == 0) {
            delay(1);
        }
    }

    if (debug) {
        Serial.println("Finished parsing: " + String(eventCount) + " events found");
    }

    valid = true;
    return true;
}

bool ICSParser::parseStreamLarge(Stream* stream) {
    if (!stream) {
        setError(ICSParseError::STREAM_READ_ERROR, "Invalid stream");
        return false;
    }

    if (debug) {
        Serial.println("=== Parsing ICS from stream ===");
    }

    clear();

    // State machine for parsing
    enum ParseState {
        LOOKING_FOR_CALENDAR,
        IN_HEADER,
        IN_EVENT,
        DONE
    };

    ParseState state = LOOKING_FOR_CALENDAR;
    String currentLine = "";
    String eventBuffer = "";
    String headerBuffer = "";
    bool eof = false;
    int eventCount = 0;

    while (!eof && state != DONE) {
        currentLine = readLineFromStream(stream, eof);

        if (currentLine.isEmpty() && !eof) {
            continue;
        }

        switch (state) {
            case LOOKING_FOR_CALENDAR:
                if (currentLine.indexOf("BEGIN:VCALENDAR") != -1) {
                    headerBuffer = currentLine + "\n";
                    state = IN_HEADER;
                }
                break;

            case IN_HEADER:
                headerBuffer += currentLine + "\n";

                if (currentLine.indexOf("BEGIN:VEVENT") != -1) {
                    // Parse header
                    String unfoldedHeader = unfoldLines(headerBuffer);
                    if (!parseHeader(unfoldedHeader)) {
                        return false;
                    }
                    if (!validateHeader()) {
                        return false;
                    }

                    // Start collecting event
                    eventBuffer = currentLine + "\n";
                    state = IN_EVENT;
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    // No events found
                    state = DONE;
                }
                break;

            case IN_EVENT:
                eventBuffer += currentLine + "\n";

                if (currentLine.indexOf("END:VEVENT") != -1) {
                    // Process the complete event
                    String unfoldedEvent = unfoldLines(eventBuffer);
                    if (parseEvent(unfoldedEvent)) {
                        eventCount++;
                        if (debug && (eventCount % 10 == 0)) {
                            Serial.println("Parsed " + String(eventCount) + " events...");
                        }
                    }

                    // Clear event buffer and look for next event
                    eventBuffer = "";
                    state = IN_HEADER;

                    // Yield periodically
                    if (eventCount % 5 == 0) {
                        delay(1);
                    }
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    state = DONE;
                }
                break;

            case DONE:
                // Already finished, nothing to do
                break;
        }

        // Prevent buffer overflow
        if (eventBuffer.length() > 8192) {
            if (debug) {
                Serial.println("Warning: Event buffer overflow, skipping event");
            }
            eventBuffer = "";
            state = IN_HEADER;
        }
    }

    if (debug) {
        Serial.println("Stream parsing complete: " + String(eventCount) + " events found");
    }

    valid = true;
    return true;
}

String ICSParser::readLineFromStream(Stream* stream, bool& eof) {
    String line = "";
    eof = false;

    if (!stream || !stream->available()) {
        eof = true;
        return line;
    }

    // Read until we hit a newline or run out of data
    while (stream->available()) {
        char c = stream->read();

        if (c == '\r') {
            // Possible line ending, check for \n
            if (stream->available() && stream->peek() == '\n') {
                stream->read(); // Consume the \n
            }
            break;
        } else if (c == '\n') {
            break;
        } else {
            line += c;
        }

        // Prevent line from getting too long
        if (line.length() > 1024) {
            break;
        }
    }

    if (!stream->available() && line.isEmpty()) {
        eof = true;
    }

    return line;
}

bool ICSParser::processEventBlock(const String& eventBlock) {
    // This is just a wrapper around parseEvent for compatibility
    return parseEvent(eventBlock);
}
