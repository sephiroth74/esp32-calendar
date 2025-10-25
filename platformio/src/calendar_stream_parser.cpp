/**
 * Implementation of memory-efficient calendar stream parser
 */

#include "calendar_stream_parser.h"
#include "calendar_fetcher.h"
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
                                                         size_t maxEvents) {
    FilteredEvents* result = new FilteredEvents();

    if (debug) {
        Serial.println("=== Stream Parsing Calendar ===");
        Serial.println("URL: " + url);
        Serial.printf("Date range: %ld to %ld\n", startDate, endDate);
        Serial.printf("Max events: %d\n", maxEvents);
    }

    // Get stream from fetcher
    Stream* stream = fetcher->fetchStream(url);
    if (!stream) {
        result->success = false;
        result->error = "Failed to open stream";
        if (debug) Serial.println("Error: " + result->error);
        return result;
    }

    // Parse events from stream
    ParseState state = LOOKING_FOR_CALENDAR;
    String currentLine = "";
    String eventBuffer = "";
    String headerBuffer = "";
    bool eof = false;

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

                // Parse calendar metadata
                if (currentLine.indexOf("X-WR-CALNAME:") != -1) {
                    calendarName = extractValue(currentLine, "X-WR-CALNAME:");
                }

                if (currentLine.indexOf("BEGIN:VEVENT") != -1) {
                    // Start collecting event
                    eventBuffer = currentLine + "\n";
                    state = IN_EVENT;
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    state = DONE;
                }
                break;

            case IN_EVENT:
                eventBuffer += currentLine + "\n";

                if (currentLine.indexOf("END:VEVENT") != -1) {
                    // Parse and filter event
                    result->totalParsed++;

                    // Parse the event
                    CalendarEvent* event = parseEventFromBuffer(eventBuffer);
                    if (event) {
                        // Add calendar metadata
                        event->calendarName = calendarName;
                        event->calendarColor = calendarColor;

                        // Check if event is in range
                        if (event->isRecurring) {
                            // Expand recurring events
                            std::vector<CalendarEvent*> expanded =
                                expandRecurringEvent(event, startDate, endDate);

                            for (auto expandedEvent : expanded) {
                                if (result->events.size() < maxEvents || maxEvents == 0) {
                                    result->events.push_back(expandedEvent);
                                    result->totalFiltered++;
                                } else {
                                    delete expandedEvent;
                                }
                            }

                            // Delete original recurring event
                            delete event;
                        } else if (isEventInRange(event, startDate, endDate)) {
                            if (result->events.size() < maxEvents || maxEvents == 0) {
                                result->events.push_back(event);
                                result->totalFiltered++;
                            } else {
                                delete event;
                            }
                        } else {
                            // Event not in range, delete it
                            delete event;
                        }
                    }

                    // Clear event buffer
                    eventBuffer = "";
                    state = IN_HEADER;

                    // Yield periodically
                    if (result->totalParsed % 10 == 0) {
                        delay(1);
                    }

                    // Stop if we have enough events
                    if (maxEvents > 0 && result->events.size() >= maxEvents) {
                        state = DONE;
                    }
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    state = DONE;
                }
                break;

            case DONE:
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

    // Clean up stream
    fetcher->endStream();

    // Sort events by start time
    std::sort(result->events.begin(), result->events.end(),
              [](CalendarEvent* a, CalendarEvent* b) {
                  return a->startTime < b->startTime;
              });

    if (debug) {
        Serial.printf("Parsing complete: %d events parsed, %d filtered\n",
                     result->totalParsed, result->totalFiltered);
    }

    result->success = true;
    return result;
}

bool CalendarStreamParser::streamParse(const String& url,
                                       EventCallback callback,
                                       time_t startDate,
                                       time_t endDate) {
    if (!callback) {
        return false;
    }

    // Get stream from fetcher
    Stream* stream = fetcher->fetchStream(url);
    if (!stream) {
        if (debug) Serial.println("Error: Failed to open stream");
        return false;
    }

    ParseState state = LOOKING_FOR_CALENDAR;
    String currentLine = "";
    String eventBuffer = "";
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
                    state = IN_HEADER;
                }
                break;

            case IN_HEADER:
                if (currentLine.indexOf("BEGIN:VEVENT") != -1) {
                    eventBuffer = currentLine + "\n";
                    state = IN_EVENT;
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    state = DONE;
                }
                break;

            case IN_EVENT:
                eventBuffer += currentLine + "\n";

                if (currentLine.indexOf("END:VEVENT") != -1) {
                    // Parse event
                    CalendarEvent* event = parseEventFromBuffer(eventBuffer);
                    if (event) {
                        // Check date range if specified
                        bool inRange = true;
                        if (startDate > 0 || endDate > 0) {
                            inRange = isEventInRange(event, startDate, endDate);
                        }

                        if (inRange) {
                            // Call callback with event
                            callback(event);
                        }

                        // Delete event after callback
                        delete event;
                        eventCount++;
                    }

                    eventBuffer = "";
                    state = IN_HEADER;

                    // Yield periodically
                    if (eventCount % 10 == 0) {
                        delay(1);
                    }
                } else if (currentLine.indexOf("END:VCALENDAR") != -1) {
                    state = DONE;
                }
                break;

            case DONE:
                break;
        }

        // Prevent buffer overflow
        if (eventBuffer.length() > 8192) {
            eventBuffer = "";
            state = IN_HEADER;
        }
    }

    fetcher->endStream();
    return true;
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

    if (!event || event->rrule.isEmpty()) {
        return occurrences;
    }

    // Simple implementation - just add the original event if it's in range
    // TODO: Implement full RRULE expansion
    if (isEventInRange(event, startDate, endDate)) {
        CalendarEvent* occurrence = new CalendarEvent(*event);
        occurrences.push_back(occurrence);
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
        if (debug) {
            Serial.println("Using cached events");
        }
        // Return copy of cached events
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

        if (debug) {
            Serial.println("Fetching from: " + calendars[i].name);
        }

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