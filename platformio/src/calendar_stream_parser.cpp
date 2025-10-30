/**
 * Implementation of memory-efficient calendar stream parser
 */

#include "calendar_stream_parser.h"
#include "calendar_fetcher.h"
#include "debug_config.h"
#include "tee_stream.h"
#include <algorithm>

// Constructor
CalendarStreamParser::CalendarStreamParser()
    : debug(false), calendarColor(0), fetcher(nullptr) {
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
        result->success = true;
        result->totalFiltered = result->events.size();
        // totalParsed is not easily available here, would require another callback
    } else {
        result->success = false;
        result->error = "Stream parsing failed";
    }

    // Sort events by start time
    std::sort(result->events.begin(), result->events.end(),
              [](CalendarEvent* a, CalendarEvent* b) {
                  return a->startTime < b->startTime;
              });

    DEBUG_INFO_PRINTLN("Parsing complete: " + String((unsigned long)result->totalFiltered) + " events filtered");
    return result;
}

bool CalendarStreamParser::streamParse(const String& url,
                                       EventCallback callback,
                                       time_t startDate,
                                       time_t endDate,
                                       const String& cachePath) {
    if (!callback) {
        return false;
    }

    // Get stream from fetcher
    Stream* httpStream = fetcher->fetchStream(url);
    if (!httpStream) {
        DEBUG_ERROR_PRINTLN("Error: Failed to open stream");
        return false;
    }

    Stream* finalStream = httpStream;
    File cacheFile;
    TeeStream* teeStream = nullptr;

    bool isRemote = !url.startsWith("file://");

    if (isRemote && !cachePath.isEmpty()) {
        if (!LittleFS.exists("/cache")) {
            DEBUG_INFO_PRINTLN(">>> Cache directory doesn't exist, creating /cache");
            if (LittleFS.mkdir("/cache")) {
                DEBUG_VERBOSE_PRINTLN(">>> Cache directory created successfully");
            } else {
                DEBUG_WARN_PRINTLN(">>> WARNING: Failed to create cache directory");
            }
        }

        DEBUG_INFO_PRINTLN(">>> Opening cache file for writing: " + cachePath);
        cacheFile = LittleFS.open(cachePath, "w");
        if (cacheFile) {
            DEBUG_VERBOSE_PRINTLN(">>> Cache file opened successfully for writing");
            DEBUG_VERBOSE_PRINTLN("Caching to: " + cachePath);
            teeStream = new TeeStream(*httpStream, cacheFile);
            finalStream = teeStream;
        } else {
            DEBUG_ERROR_PRINTLN(">>> ERROR: Could not open cache file for writing: " + cachePath);
            DEBUG_WARN_PRINTLN("Warning: Could not open cache file " + cachePath);
        }
    }

    ParseState state = LOOKING_FOR_CALENDAR;
    String currentLine = "";
    String eventBuffer = "";
    bool eof = false;
    int eventCount = 0;
    int lineCount = 0;
    int eventsFiltered = 0;
    int eventsRejected = 0;
    bool parseSuccess = true;

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
    DEBUG_INFO_PRINTLN(">>> Date range filter: " + String(dateStartStr) + " (" + String(startDate) + ") to " + String(dateEndStr) + " (" + String(endDate) + ")");

    while (!eof && state != DONE) {
        currentLine = readLineFromStream(finalStream, eof);
        lineCount++;

        if (currentLine.isEmpty() && !eof) {
            continue;
        }

        // Progress update every 100 lines
        if (lineCount % 100 == 0) {
            DEBUG_VERBOSE_PRINTLN(">>> Parse progress: " + String(lineCount) + " lines read, " + String(eventCount) + " events parsed, " + String(eventsFiltered) + " events filtered");
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
                    state = IN_EVENT;
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
                            std::vector<CalendarEvent*> expanded = expandRecurringEvent(event, startDate, endDate);
                            int expandedCount = 0;
                            for (auto expandedEvent : expanded) {
                                if (isEventInRange(expandedEvent, startDate, endDate)) {
                                    callback(expandedEvent);
                                    eventsFiltered++;
                                    expandedCount++;
                                }
                                else {
                                    delete expandedEvent;
                                }
                            }
                            // Debug: show recurring event expansion
                            if (expandedCount > 0) {
                                DEBUG_VERBOSE_PRINTF(">>> Recurring event expanded: '%s' (RRULE: %s) → %d occurrences\n",
                                             event->summary.c_str(), event->rrule.c_str(), expandedCount);
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
                                DEBUG_WARN_PRINTLN(">>> Event rejected (out of range): '" + event->summary + "' on " + String(eventDateStr) + " (" + String(event->startTime) + ")");
                            }
                            eventsRejected++;
                            delete event;
                        }
                    }

                    eventBuffer = "";
                    state = IN_HEADER;

                    if (eventCount % 10 == 0) {
                        delay(1);
                    }
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    DEBUG_WARN_PRINTLN(">>> Found END:VCALENDAR (unexpected in event)");
                    state = DONE;
                }
                break;

            case DONE:
                break;
        }

        if (eventBuffer.length() > 8192) {
            DEBUG_ERROR_PRINTLN(">>> ERROR: Event buffer exceeded 8192 bytes, skipping event");
            eventBuffer = "";
            state = IN_HEADER;
            parseSuccess = false; // Indicate an error
        }
    }

    DEBUG_INFO_PRINTLN(">>> Stream parsing complete: " + String(lineCount) + " lines read, " + String(eventCount) + " events parsed, " + String(eventsFiltered) + " events filtered, " + String(eventsRejected) + " events rejected");

    if (teeStream) {
        delete teeStream;
    }
    if (cacheFile) {
        cacheFile.flush(); // Ensure all data is written
        size_t cacheSize = cacheFile.size();
        cacheFile.close();
        DEBUG_INFO_PRINTLN(">>> Cache file closed, size: " + String((unsigned long)cacheSize) + " bytes written to disk");
    }

    fetcher->endStream();
    return parseSuccess;
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
    String line = "";
    bool eof = false;

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
    event->summary = extractValueFromBuffer(eventData, "SUMMARY:");
    event->location = extractValueFromBuffer(eventData, "LOCATION:");
    event->description = extractValueFromBuffer(eventData, "DESCRIPTION:");
    event->uid = extractValueFromBuffer(eventData, "UID:");
    event->status = extractValueFromBuffer(eventData, "STATUS:");
    event->rrule = extractValueFromBuffer(eventData, "RRULE:");
    event->isRecurring = !event->rrule.isEmpty();

    // Parse start date/time
    String dtStart = extractValueFromBuffer(eventData, "DTSTART:");
    if (dtStart.isEmpty()) {
        dtStart = extractValueFromBuffer(eventData, "DTSTART;VALUE=DATE:");
    }
    if (dtStart.isEmpty()) {
        dtStart = extractValueFromBuffer(eventData, "DTSTART;VALUE=DATE-TIME:");
    }

    if (!dtStart.isEmpty()) {
        event->dtStart = dtStart;
        event->allDay = (dtStart.length() == 8);
        event->setStartDateTime(dtStart);
    }

    // Parse end date/time
    String dtEnd = extractValueFromBuffer(eventData, "DTEND:");
    if (dtEnd.isEmpty()) {
        dtEnd = extractValueFromBuffer(eventData, "DTEND;VALUE=DATE:");
    }
    if (dtEnd.isEmpty()) {
        dtEnd = extractValueFromBuffer(eventData, "DTEND;VALUE=DATE-TIME:");
    }

    if (!dtEnd.isEmpty()) {
        event->dtEnd = dtEnd;
        event->setEndDateTime(dtEnd);
    }

    return event;
}

String CalendarStreamParser::extractValueFromBuffer(const String& buffer, const String& property) {
    int pos = buffer.indexOf(property);
    if (pos == -1) {
        return "";
    }

    int valueStart = pos + property.length();
    int valueEnd = buffer.indexOf("\n", valueStart);
    if (valueEnd == -1) {
        valueEnd = buffer.length();
    }

    String value = buffer.substring(valueStart, valueEnd);
    value.trim();
    value.replace("\r", "");

    return value;
}

bool CalendarStreamParser::isEventInRange(CalendarEvent* event, time_t startDate, time_t endDate) {
    if (!event) return false;

    time_t eventStart = event->startTime;
    time_t eventEnd = event->endTime;

    if (eventEnd == 0) {
        eventEnd = eventStart;
    }


    return (eventStart <= endDate) && (eventEnd >= startDate);
}

std::vector<CalendarEvent*> CalendarStreamParser::expandRecurringEvent(CalendarEvent* event,
                                                                       time_t startDate,
                                                                       time_t endDate) {
    std::vector<CalendarEvent*> occurrences;

    // Sanity checks
    if (!event || event->rrule.isEmpty()) {
        return occurrences;
    }

    if (startDate >= endDate) {
        DEBUG_WARN_PRINTLN(">>> expandRecurringEvent: Invalid date range (startDate >= endDate)");
        return occurrences;
    }

    if (endDate - startDate > 63072000) {  // More than 2 years (730 days)
        DEBUG_WARN_PRINTLN(">>> expandRecurringEvent: Date range too large (> 2 years), limiting expansion");
        endDate = startDate + 63072000;  // Limit to 2 years
    }

    // Get original event time components
    struct tm originalTm;
    memcpy(&originalTm, localtime(&event->startTime), sizeof(struct tm));
    int originalDay = originalTm.tm_mday;
    int originalMonth = originalTm.tm_mon;
    int originalHour = originalTm.tm_hour;
    int originalMin = originalTm.tm_min;
    int originalSec = originalTm.tm_sec;

    // Get start/end date components
    struct tm startTm, endTm;
    memcpy(&startTm, localtime(&startDate), sizeof(struct tm));
    memcpy(&endTm, localtime(&endDate), sizeof(struct tm));

    time_t duration = event->endTime - event->startTime;

    // Parse RRULE for different frequencies
    if (event->rrule.indexOf("FREQ=YEARLY") != -1) {
        // Yearly: Set event month/day to current year, check if in range
        int startYear = startTm.tm_year + 1900;
        int endYear = endTm.tm_year + 1900;

        for (int year = startYear; year <= endYear; year++) {
            struct tm occurrenceTm = {0};
            occurrenceTm.tm_year = year - 1900;
            occurrenceTm.tm_mon = originalMonth;
            occurrenceTm.tm_mday = originalDay;
            occurrenceTm.tm_hour = originalHour;
            occurrenceTm.tm_min = originalMin;
            occurrenceTm.tm_sec = originalSec;

            time_t occurrenceTime = mktime(&occurrenceTm);

            if (occurrenceTime >= startDate && occurrenceTime <= endDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime = occurrenceTime;
                occurrence->endTime = occurrenceTime + duration;

                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &occurrenceTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
            }
        }
    } else if (event->rrule.indexOf("FREQ=MONTHLY") != -1) {
        // Monthly: Set event year/month to start range, advance month by month
        struct tm occurrenceTm = {0};
        occurrenceTm.tm_year = startTm.tm_year;
        occurrenceTm.tm_mon = startTm.tm_mon;
        occurrenceTm.tm_mday = originalDay;
        occurrenceTm.tm_hour = originalHour;
        occurrenceTm.tm_min = originalMin;
        occurrenceTm.tm_sec = originalSec;

        int maxIterations = 24;  // Max 2 years of monthly events
        for (int i = 0; i < maxIterations; i++) {
            time_t occurrenceTime = mktime(&occurrenceTm);

            if (occurrenceTime > endDate) break;

            if (occurrenceTime >= startDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime = occurrenceTime;
                occurrence->endTime = occurrenceTime + duration;

                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &occurrenceTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
            }

            // Advance to next month
            occurrenceTm.tm_mon++;
            if (occurrenceTm.tm_mon > 11) {
                occurrenceTm.tm_mon = 0;
                occurrenceTm.tm_year++;
            }
        }
    } else if (event->rrule.indexOf("FREQ=WEEKLY") != -1) {
        // Weekly: Set event to start range day of week, advance by weeks
        struct tm occurrenceTm = {0};
        occurrenceTm.tm_year = startTm.tm_year;
        occurrenceTm.tm_mon = startTm.tm_mon;
        occurrenceTm.tm_mday = startTm.tm_mday;
        occurrenceTm.tm_hour = originalHour;
        occurrenceTm.tm_min = originalMin;
        occurrenceTm.tm_sec = originalSec;

        // Normalize to get the correct day of week
        mktime(&occurrenceTm);

        // Find the first occurrence on or after startDate with the same day of week
        int originalDayOfWeek = originalTm.tm_wday;
        int daySearchLimit = 7;  // Safety: max 7 days to find matching weekday
        for (int i = 0; i < daySearchLimit && occurrenceTm.tm_wday != originalDayOfWeek; i++) {
            occurrenceTm.tm_mday++;
            mktime(&occurrenceTm);
        }

        int maxIterations = 104;  // Max 2 years of weekly events
        for (int i = 0; i < maxIterations; i++) {
            time_t occurrenceTime = mktime(&occurrenceTm);

            if (occurrenceTime > endDate) break;

            if (occurrenceTime >= startDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime = occurrenceTime;
                occurrence->endTime = occurrenceTime + duration;

                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &occurrenceTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
            }

            // Advance by 7 days
            occurrenceTm.tm_mday += 7;
            mktime(&occurrenceTm);  // Normalize
        }
    } else if (event->rrule.indexOf("FREQ=DAILY") != -1) {
        // Daily: Every day from start to end range
        struct tm occurrenceTm = {0};
        occurrenceTm.tm_year = startTm.tm_year;
        occurrenceTm.tm_mon = startTm.tm_mon;
        occurrenceTm.tm_mday = startTm.tm_mday;
        occurrenceTm.tm_hour = originalHour;
        occurrenceTm.tm_min = originalMin;
        occurrenceTm.tm_sec = originalSec;

        // Calculate exact iterations needed based on date range (inclusive), coerced to [1, 365]
        int daysDiff = (endDate - startDate) / 86400 + 1;  // +1 for inclusive range
        int maxIterations = std::max(1, std::min(daysDiff, 365));
        for (int i = 0; i < maxIterations; i++) {
            time_t occurrenceTime = mktime(&occurrenceTm);

            if (occurrenceTime > endDate) break;

            if (occurrenceTime >= startDate) {
                CalendarEvent* occurrence = new CalendarEvent(*event);
                occurrence->startTime = occurrenceTime;
                occurrence->endTime = occurrenceTime + duration;

                char dateBuffer[32];
                strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &occurrenceTm);
                occurrence->date = String(dateBuffer);

                occurrences.push_back(occurrence);
            }

            // Advance by 1 day
            occurrenceTm.tm_mday++;
            mktime(&occurrenceTm);  // Normalize
        }
    } else {
        // For events without recognized RRULE or non-recurring, just check if in range
        if (isEventInRange(event, startDate, endDate)) {
            CalendarEvent* occurrence = new CalendarEvent(*event);
            occurrences.push_back(occurrence);
        }
    }

    return occurrences;
}

String CalendarStreamParser::readLineFromStream(Stream* stream, bool& eof) {
    String line = "";
    eof = false;

    if (!stream || !stream->available()) {
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

    if (!stream->available() && line.isEmpty()) {
        eof = true;
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

// ============================================================================
// OptimizedCalendarManager Implementation
// ============================================================================

OptimizedCalendarManager::OptimizedCalendarManager()
    : debug(false), cacheEnabled(true), cacheDuration(3600),
      cacheStartDate(0), cacheEndDate(0), cacheTimestamp(0) {
}

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

std::vector<CalendarEvent*> OptimizedCalendarManager::getEventsForRange(time_t startDate,
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
        time_t calEndDate = endDate;

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
        cacheEndDate = endDate;
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
    struct tm* tm = localtime(&date);
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    time_t dayStart = mktime(tm);

    tm->tm_hour = 23;
    tm->tm_min = 59;
    tm->tm_sec = 59;
    time_t dayEnd = mktime(tm);

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
    cacheEndDate = 0;
    cacheTimestamp = 0;
}

std::vector<CalendarEvent*> OptimizedCalendarManager::mergeAndSortEvents(
    const std::vector<std::vector<CalendarEvent*>>& eventLists) {

    std::vector<CalendarEvent*> merged;

    for (const auto& list : eventLists) {
        merged.insert(merged.end(), list.begin(), list.end());
    }

    // Sort by start time
    std::sort(merged.begin(), merged.end(),
              [](CalendarEvent* a, CalendarEvent* b) {
                  return a->startTime < b->startTime;
              });

    return merged;
}