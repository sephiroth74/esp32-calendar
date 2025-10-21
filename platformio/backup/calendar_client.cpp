#include "calendar_client.h"
#include <algorithm>
#include <LittleFS.h>

CalendarClient::CalendarClient(WiFiClientSecure* client)
    : wifiClient(client)
{
    client->setInsecure();
    httpClient.setReuse(false);
}

CalendarClient::~CalendarClient() {
    httpClient.end();
}

std::vector<CalendarEvent*> CalendarClient::fetchEvents(int daysAhead) {
    // This method is deprecated - use fetchICSEvents directly with URL from config
    // Kept for backward compatibility with debug.cpp
    Serial.println("WARNING: CalendarClient::fetchEvents() is deprecated");
    Serial.println("Use fetchICSEvents() directly with URL from config.json");

    // Return empty vector as configuration now comes from LittleFS
    std::vector<CalendarEvent*> emptyEvents;
    return emptyEvents;
}

std::vector<CalendarEvent*> CalendarClient::fetchICSEvents(const String& url, int daysAhead) {
    std::vector<CalendarEvent*> events;

    Serial.println("Fetching ICS calendar from: " + url);

    httpClient.begin(*wifiClient, url);
    httpClient.setTimeout(10000); // 10 second timeout
    httpClient.addHeader("User-Agent", "ESP32-Calendar/1.0");

    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = httpClient.getString();
        Serial.println("ICS data received, size: " + String(payload.length()));

        // Check PSRAM availability
        if (PSRAM_AVAILABLE) {
            Serial.println("Using PSRAM for event storage");
            Serial.println("Free PSRAM: " + String(ESP.getFreePsram()) + " bytes");
        }

        // Parse events (filtering is now done in parseICSCalendar for date range)
        events = parseICSCalendar(payload, daysAhead);

        // NOTE: Don't filter or limit here when called from fetchMultipleCalendars
        // For backward compatibility with single calendar, we still filter/limit

        Serial.println("Returning " + String(events.size()) + " events (from remote ICS)");
    } else {
        Serial.println("HTTP error: " + String(httpCode));
        if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
            String location = httpClient.getLocation();
            Serial.println("Redirect to: " + location);
        }
    }

    httpClient.end();
    return events;
}

std::vector<CalendarEvent*> CalendarClient::parseICSCalendar(const String& icsData, int daysAhead) {
    std::vector<CalendarEvent*> events;

    // Get current time for filtering
    time_t now;
    time(&now);
    time_t endRange = now + (daysAhead * 24 * 3600);

    // Get today's date for comparison
    struct tm* nowTm = localtime(&now);
    int currentYear = nowTm->tm_year + 1900;
    int currentMonth = nowTm->tm_mon + 1;
    int currentDay = nowTm->tm_mday;

    Serial.println("Parsing ICS, filtering events from today to " + String(daysAhead) + " days ahead");
    Serial.println("Current date: " + String(currentYear) + "-" +
                   (currentMonth < 10 ? "0" : "") + String(currentMonth) + "-" +
                   (currentDay < 10 ? "0" : "") + String(currentDay));
    Serial.println("Current year is: " + String(currentYear) + " (should be 2025 for testing)");


    int pos = 0;
    int totalEvents = 0;
    int eventsInRange = 0;

    while (pos < icsData.length()) {
        int beginPos = icsData.indexOf("BEGIN:VEVENT", pos);
        if (beginPos == -1) break;

        int endPos = icsData.indexOf("END:VEVENT", beginPos);
        if (endPos == -1) break;

        String eventData = icsData.substring(beginPos, endPos + 10);
        CalendarEvent* event = parseICSEvent(eventData);
        if (!event) {
            pos = endPos + 1;
            continue;
        }
        totalEvents++;

        // Check if this is a recurring event
        String rrule = extractICSValue(eventData, "RRULE:");
        bool isYearlyRecurring = rrule.indexOf("FREQ=YEARLY") >= 0;

        if (!event->title.isEmpty() && !event->date.isEmpty()) {
            // Parse event date for filtering
            if (event->date.length() >= 10) {
                int year = event->date.substring(0, 4).toInt();
                int month = event->date.substring(5, 7).toInt();
                int day = event->date.substring(8, 10).toInt();

                // Create time struct for event at start of day
                struct tm eventTm;
                memset(&eventTm, 0, sizeof(struct tm));
                eventTm.tm_year = year - 1900;
                eventTm.tm_mon = month - 1;
                eventTm.tm_mday = day;
                eventTm.tm_hour = 0;
                eventTm.tm_min = 0;
                eventTm.tm_sec = 0;

                time_t eventTime = mktime(&eventTm);

                // Create time struct for start of today
                struct tm todayTm;
                memset(&todayTm, 0, sizeof(struct tm));
                todayTm.tm_year = currentYear - 1900;
                todayTm.tm_mon = currentMonth - 1;
                todayTm.tm_mday = currentDay;
                todayTm.tm_hour = 0;
                todayTm.tm_min = 0;
                todayTm.tm_sec = 0;

                time_t todayStart = mktime(&todayTm);

                Serial.println("Found event: " + event->title + " on " + event->date + (event->allDay ? " (All Day)" : ""));
                Serial.println("  Event date: " + String(year) + "-" + String(month) + "-" + String(day));
                Serial.println("  Today date: " + String(currentYear) + "-" + String(currentMonth) + "-" + String(currentDay));
                Serial.println("  Event time: " + event->startTime);
                Serial.println("  Event parsed from: " + eventData.substring(0, 200)); // Show first 200 chars


                // Debug time calculations
                Serial.println("  Event timestamp: " + String((long)eventTime));
                Serial.println("  Today start timestamp: " + String((long)todayStart));
                Serial.println("  End range timestamp: " + String((long)endRange));
                Serial.println("  Days ahead: " + String(daysAhead));


                // Include events from today onwards (even if they already happened today)
                if (eventTime >= todayStart && eventTime <= endRange) {

                    // Mark if today or tomorrow
                    Serial.println("  Checking if today: year=" + String(year) + "==" + String(currentYear) +
                                  ", month=" + String(month) + "==" + String(currentMonth) +
                                  ", day=" + String(day) + "==" + String(currentDay));

                    if (year == currentYear && month == currentMonth && day == currentDay) {
                        event->isToday = true;
                        Serial.println("  -> Marked as TODAY - will display in events list");
                        // Extra debug for birthdays marked as today
                        String titleLower2 = event->title;
                        titleLower2.toLowerCase();
                        if (titleLower2.indexOf("birthday") >= 0 || titleLower2.indexOf("compleanno") >= 0) {
                            Serial.println("  *** BIRTHDAY MARKED AS TODAY: " + event->title + " ***");
                        }
                    } else if (year == currentYear && month == currentMonth && day == currentDay + 1) {
                        event->isTomorrow = true;
                        Serial.println("  -> Marked as TOMORROW");
                    } else {
                        Serial.println("  -> Not today or tomorrow");
                    }

                    // Set dayOfMonth for current month events
                    if (year == currentYear && month == currentMonth) {
                        event->dayOfMonth = day;
                    }

                    events.push_back(event);
                    eventsInRange++;

                    String eventType = isYearlyRecurring ? " [RECURRING]" : "";
                    Serial.println("Event in range: " + event->title + " on " + event->date + eventType +
                                   (event->isToday ? " [TODAY]" : "") + (event->isTomorrow ? " [TOMORROW]" : ""));

                }
                else {
                    // Event not in range, free memory
                    event->~CalendarEvent();
                    PSRAM_FREE(event);
                }
            } else {
                // Invalid date, free memory
                event->~CalendarEvent();
                PSRAM_FREE(event);
            }
        } else {
            // Empty title or date, free memory
            event->~CalendarEvent();
            PSRAM_FREE(event);
        }

        pos = endPos + 1;
    }

    Serial.println("Total events found: " + String(totalEvents));
    Serial.println("Events in date range: " + String(eventsInRange));

    // Sort events chronologically
    std::sort(events.begin(), events.end(), [](const CalendarEvent* a, const CalendarEvent* b) {
        // Compare dates first
        if (a->date != b->date) {
            return a->date < b->date;
        }
        // If same date, compare times (for non-all-day events)
        if (!a->allDay && !b->allDay) {
            return a->startTime < b->startTime;
        }
        // All-day events come first
        return a->allDay && !b->allDay;
    });

    // Debug: Show all events being returned, especially today's events
    Serial.println("=== FINAL EVENT LIST ===");
    int todayCount = 0;
    int oct26Count = 0;
    for (auto e : events) {
        if (e->isToday) {
            todayCount++;
            Serial.println("TODAY EVENT #" + String(todayCount) + ": " + e->title + " on " + e->date);
            String titleLower = e->title;
            titleLower.toLowerCase();
            if (titleLower.indexOf("birthday") >= 0 || titleLower.indexOf("compleanno") >= 0) {
                Serial.println("  *** THIS IS A BIRTHDAY EVENT MARKED AS TODAY ***");
            }
        }
        // Check for October 26 event
        if (e->date.length() >= 10) {
            int month = e->date.substring(5, 7).toInt();
            int day = e->date.substring(8, 10).toInt();
            if (month == 10 && day == 26) {
                oct26Count++;
                Serial.println("OCTOBER 26 EVENT IN FINAL LIST: " + e->title + " on " + e->date);
            }
        }
    }
    Serial.println("Total TODAY events: " + String(todayCount));
    Serial.println("Total October 26 events in final list: " + String(oct26Count));
    Serial.println("Total events returning: " + String(events.size()));

    return events;
}

CalendarEvent* CalendarClient::parseICSEvent(const String& eventData) {
    // Allocate event in PSRAM if available
    CalendarEvent* event = (CalendarEvent*)PSRAM_ALLOC(sizeof(CalendarEvent));
    if (!event) {
        Serial.println("Failed to allocate memory for CalendarEvent");
        return nullptr;
    }

    // Initialize the event using placement new
    new (event) CalendarEvent();

    // Extract SUMMARY (title)
    event->title = extractICSValue(eventData, "SUMMARY:");

    // Extract LOCATION
    event->location = extractICSValue(eventData, "LOCATION:");

    // Check for RRULE (recurring event)
    String rrule = extractICSValue(eventData, "RRULE:");
    bool isRecurring = !rrule.isEmpty();
    bool isYearlyRecurring = false;

    if (isRecurring) {
        // Check if it's a yearly recurring event (like birthdays)
        if (rrule.indexOf("FREQ=YEARLY") >= 0) {
            isYearlyRecurring = true;
            Serial.println("Found yearly recurring event: " + event->title);
        }
    }

    // Extract DTSTART (start time) - try different formats
    String dtStart = extractICSValue(eventData, "DTSTART:");
    if (dtStart.isEmpty()) {
        // Try alternative format for all-day events
        dtStart = extractICSValue(eventData, "DTSTART;VALUE=DATE:");
    }
    if (dtStart.isEmpty()) {
        // Try with timezone format - extract the part after the colon
        int tzidPos = eventData.indexOf("DTSTART;TZID=");
        if (tzidPos != -1) {
            int colonPos = eventData.indexOf(":", tzidPos);
            if (colonPos != -1) {
                int endPos = eventData.indexOf("\n", colonPos);
                if (endPos == -1) endPos = eventData.indexOf("\r", colonPos);
                if (endPos == -1) endPos = eventData.length();
                dtStart = eventData.substring(colonPos + 1, endPos);
                dtStart.trim();
            }
        }
    }

    // Extract DTEND (end time) - try different formats
    String dtEnd = extractICSValue(eventData, "DTEND:");
    if (dtEnd.isEmpty()) {
        dtEnd = extractICSValue(eventData, "DTEND;VALUE=DATE:");
    }
    if (dtEnd.isEmpty()) {
        // Try with timezone format - extract the part after the colon
        int tzidPos = eventData.indexOf("DTEND;TZID=");
        if (tzidPos != -1) {
            int colonPos = eventData.indexOf(":", tzidPos);
            if (colonPos != -1) {
                int endPos = eventData.indexOf("\n", colonPos);
                if (endPos == -1) endPos = eventData.indexOf("\r", colonPos);
                if (endPos == -1) endPos = eventData.length();
                dtEnd = eventData.substring(colonPos + 1, endPos);
                dtEnd.trim();
            }
        }
    }

    // Parse dates and times
    if (!dtStart.isEmpty()) {
        // Check if it's an all-day event (date only, no time)
        event->allDay = (dtStart.indexOf("T") == -1) || dtStart.endsWith("T000000");

        // Check if this is UTC time (ends with Z)
        bool isUTC = dtStart.endsWith("Z");

        // Convert UTC to local time if needed
        String processedDtStart = dtStart;
        if (isUTC) {
            processedDtStart = convertUTCToLocalTime(dtStart);
            Serial.println("Converted UTC to local: " + dtStart + " -> " + processedDtStart);
        }

        // Parse the date - NO YEAR MODIFICATION for non-recurring events
        if (processedDtStart.length() >= 8) {
            String year = processedDtStart.substring(0, 4);
            String month = processedDtStart.substring(4, 6);
            String day = processedDtStart.substring(6, 8);

            // Only adjust year for YEARLY recurring events (like birthdays)
            // NOT for regular events or DST events
            if (isYearlyRecurring) {
                // Get current year
                time_t now;
                time(&now);
                struct tm* timeinfo = localtime(&now);
                int currentYear = timeinfo->tm_year + 1900;

                // Update year to current year for recurring events
                year = String(currentYear);
                Serial.println("Adjusted recurring event to current year: " + year);
            }

            event->date = year + "-" + month + "-" + day;
            event->dayOfMonth = day.toInt();

            Serial.println("Parsed date: " + event->date + " (from DTSTART: " + processedDtStart + ")");

            // Debug: Show if this is a recurring event being processed
            if (isYearlyRecurring) {
                Serial.println("  Recurring event date: " + event->date);
                Serial.println("  Month: " + month + ", Day: " + day);
            }

            if (!event->allDay && processedDtStart.indexOf("T") > 0) {
                String timeStr = processedDtStart.substring(processedDtStart.indexOf("T") + 1);
                if (timeStr.length() >= 6) {
                    String hour = timeStr.substring(0, 2);
                    String minute = timeStr.substring(2, 4);
                    event->startTime = event->date + "T" + hour + ":" + minute + ":00";
                }
            }
        }
    }

    if (!dtEnd.isEmpty() && !event->allDay) {
        // Check if this is UTC time (ends with Z)
        bool isUTC = dtEnd.endsWith("Z");

        // Convert UTC to local time if needed
        String processedDtEnd = dtEnd;
        if (isUTC) {
            processedDtEnd = convertUTCToLocalTime(dtEnd);
        }

        if (processedDtEnd.indexOf("T") > 0) {
            String timeStr = processedDtEnd.substring(processedDtEnd.indexOf("T") + 1);
            if (timeStr.length() >= 6) {
                String hour = timeStr.substring(0, 2);
                String minute = timeStr.substring(2, 4);

                // For end time, we need to use the same date as start (or calculate if spans days)
                String endDate = event->date;

                // Check if end time is on a different day
                if (processedDtEnd.length() >= 8) {
                    String endYear = processedDtEnd.substring(0, 4);
                    String endMonth = processedDtEnd.substring(4, 6);
                    String endDay = processedDtEnd.substring(6, 8);
                    endDate = endYear + "-" + endMonth + "-" + endDay;
                }

                event->endTime = endDate + "T" + hour + ":" + minute + ":00";
            }
        }
    }

    // Initialize flags
    event->isToday = false;
    event->isTomorrow = false;

    // Extract DESCRIPTION if needed
    // String description = extractICSValue(eventData, "DESCRIPTION:");

    return event;
}

time_t CalendarClient::parseICSDateTime(const String& dtString, bool& isUTC) {
    // Parse various ICS datetime formats
    // Formats: YYYYMMDDTHHMMSSZ (UTC), YYYYMMDDTHHMMSS (local), YYYYMMDD (all-day)

    isUTC = dtString.endsWith("Z");
    String cleanDt = dtString;
    if (isUTC) {
        cleanDt = cleanDt.substring(0, cleanDt.length() - 1);
    }

    // Parse date components
    if (cleanDt.length() < 8) {
        return 0;  // Invalid format
    }

    int year = cleanDt.substring(0, 4).toInt();
    int month = cleanDt.substring(4, 6).toInt();
    int day = cleanDt.substring(6, 8).toInt();
    int hour = 0, minute = 0, second = 0;

    // Parse time components if present
    if (cleanDt.length() >= 15 && cleanDt.indexOf("T") > 0) {
        hour = cleanDt.substring(9, 11).toInt();
        minute = cleanDt.substring(11, 13).toInt();
        second = cleanDt.substring(13, 15).toInt();
    }

    // Create time structure
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = -1;  // Let mktime determine DST

    return mktime(&timeinfo);
}

String CalendarClient::convertUTCToLocalTime(const String& dtString) {
    // dtString format: YYYYMMDDTHHMMSSZ or YYYYMMDDTHHMMSS
    // Serial.println("convertUTCToLocalTime input: " + dtString);

    // Remove Z if present
    String cleanDt = dtString;
    if (cleanDt.endsWith("Z")) {
        cleanDt = cleanDt.substring(0, cleanDt.length() - 1);
    }

    // Parse the datetime components
    if (cleanDt.length() < 15 || cleanDt.indexOf("T") == -1) {
        Serial.println("Invalid datetime format: " + dtString);
        return dtString;  // Return original if can't parse
    }

    int year = cleanDt.substring(0, 4).toInt();
    int month = cleanDt.substring(4, 6).toInt();
    int day = cleanDt.substring(6, 8).toInt();
    int hour = cleanDt.substring(9, 11).toInt();
    int minute = cleanDt.substring(11, 13).toInt();
    int second = cleanDt.substring(13, 15).toInt();

    // Create time structure for UTC time
    struct tm utcTime;
    memset(&utcTime, 0, sizeof(struct tm));
    utcTime.tm_year = year - 1900;
    utcTime.tm_mon = month - 1;
    utcTime.tm_mday = day;
    utcTime.tm_hour = hour;
    utcTime.tm_min = minute;
    utcTime.tm_sec = second;
    utcTime.tm_isdst = 0;  // UTC doesn't have DST

    // Convert to time_t (UTC seconds since epoch)
    // Since we know this is UTC, we need to treat it as such
    time_t utcSeconds = mktime(&utcTime);

    // The system is configured with TIMEZONE via setenv/tzset,
    // so localtime_r will automatically handle timezone conversion including DST
    struct tm localTime;
    localtime_r(&utcSeconds, &localTime);

    // Format back to string (without Z since it's now local time)
    char localDtString[32];  // Increased buffer size to avoid truncation
    snprintf(localDtString, sizeof(localDtString), "%04d%02d%02dT%02d%02d%02d",
             localTime.tm_year + 1900,
             localTime.tm_mon + 1,
             localTime.tm_mday,
             localTime.tm_hour,
             localTime.tm_min,
             localTime.tm_sec);

    // Serial.println("convertUTCToLocalTime output: " + String(localDtString));
    return String(localDtString);
}

String CalendarClient::extractICSValue(const String& line, const String& key) {
    int keyPos = line.indexOf(key);
    if (keyPos == -1) return "";

    int startPos = keyPos + key.length();
    int endPos = line.indexOf("\r", startPos);
    if (endPos == -1) {
        endPos = line.indexOf("\n", startPos);
    }
    if (endPos == -1) {
        endPos = line.length();
    }

    String value = line.substring(startPos, endPos);

    // Handle line folding (lines that start with space are continuations)
    while (endPos < line.length() - 1) {
        char nextChar = line.charAt(endPos + 1);
        if (nextChar == ' ' || nextChar == '\t') {
            int nextEndPos = line.indexOf("\r", endPos + 1);
            if (nextEndPos == -1) {
                nextEndPos = line.indexOf("\n", endPos + 1);
            }
            if (nextEndPos == -1) {
                nextEndPos = line.length();
            }
            value += line.substring(endPos + 2, nextEndPos);
            endPos = nextEndPos;
        } else {
            break;
        }
    }

    // Remove any remaining line breaks
    value.replace("\r", "");
    value.replace("\n", "");

    // Handle escaped characters
    value.replace("\\n", "\n");
    value.replace("\\,", ",");
    value.replace("\\;", ";");
    value.replace("\\\\", "\\");

    // Remove quotes if present
    if (value.startsWith("\"") && value.endsWith("\"")) {
        value = value.substring(1, value.length() - 1);
    }

    return value;
}

std::vector<CalendarEvent*> CalendarClient::fetchLocalICSEvents(const String& filepath, int daysAhead) {
    std::vector<CalendarEvent*> events;

    Serial.println("Fetching local ICS file from: " + filepath);

    // Check if LittleFS is mounted
    if (!LittleFS.begin(false)) {
        Serial.println("LittleFS not mounted, trying to mount...");
        if (!LittleFS.begin(true)) {
            Serial.println("Failed to mount LittleFS");
            return events;
        }
    }

    // Check if file exists
    if (!LittleFS.exists(filepath)) {
        Serial.println("Local ICS file not found: " + filepath);
        return events;
    }

    // Open the file
    File file = LittleFS.open(filepath, "r");
    if (!file) {
        Serial.println("Failed to open local ICS file");
        return events;
    }

    // Read file content
    String icsData = "";
    size_t fileSize = file.size();
    Serial.println("Local ICS file size: " + String(fileSize) + " bytes");

    // Check if we have enough memory
    if (fileSize > 500000) {  // Limit to 500KB for safety
        Serial.println("Local ICS file too large (max 500KB)");
        file.close();
        return events;
    }

    // Read the entire file
    while (file.available()) {
        icsData += file.readString();
    }
    file.close();

    Serial.println("Local ICS data loaded, size: " + String(icsData.length()));

    // Parse the ICS data
    events = parseICSCalendar(icsData, daysAhead);

    // NOTE: Don't filter or limit here - let fetchMultipleCalendars handle it
    // This ensures consistent handling of merged events

    Serial.println("Returning " + String(events.size()) + " events from local file (unfiltered)");
    return events;
}

std::vector<CalendarEvent*> CalendarClient::fetchGoogleCalendarEvents(int daysAhead) {
    std::vector<CalendarEvent*> events;

    // Get current time and end time
    time_t now;
    time(&now);
    struct tm* timeinfo = gmtime(&now);
    char timeMin[30];
    strftime(timeMin, sizeof(timeMin), "%Y-%m-%dT%H:%M:%S.000Z", timeinfo);

    time_t endTime = now + (daysAhead * 24 * 3600);
    timeinfo = gmtime(&endTime);
    char timeMax[30];
    strftime(timeMax, sizeof(timeMax), "%Y-%m-%dT%H:%M:%S.000Z", timeinfo);

    // Google Calendar API integration is not implemented
    // Configuration should be done through config.json with ICS URL
    Serial.println("ERROR: Google Calendar API is not implemented");
    Serial.println("Please use ICS calendar URL in config.json instead");
    return events;  // Return empty vector

    /* Commented out - would need API credentials from config.json
    String url = "https://www.googleapis.com/calendar/v3/calendars/";
    url += urlEncode(calendarId);  // Would need to come from config
    url += "/events?";
    url += "timeMin=" + String(timeMin);
    url += "&timeMax=" + String(timeMax);
    url += "&singleEvents=true";
    url += "&orderBy=startTime";
    url += "&maxResults=" + String(MAX_EVENTS_TO_SHOW);
    url += "&key=" + String(apiKey);  // Would need to come from config

    Serial.println("Fetching Google Calendar events...");

    httpClient.addHeader("Accept", "application/json");

    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = httpClient.getString();
        events = parseGoogleCalendarResponse(payload);
        Serial.println("Found " + String(events.size()) + " events");
    } else {
        Serial.println("HTTP error: " + String(httpCode));
    }

    httpClient.end();
    return events;
    */
}

std::vector<CalendarEvent*> CalendarClient::parseGoogleCalendarResponse(const String& jsonResponse) {
    std::vector<CalendarEvent*> events;

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error) {
        Serial.println("JSON parsing error: " + String(error.c_str()));
        return events;
    }

    JsonArray items = doc["items"];
    for (JsonObject item : items) {
        // Allocate event in PSRAM if available
        CalendarEvent* event = (CalendarEvent*)PSRAM_ALLOC(sizeof(CalendarEvent));
        if (!event) {
            Serial.println("Failed to allocate memory for CalendarEvent");
            continue;
        }
        new (event) CalendarEvent();

        event->title = item["summary"] | "No Title";
        event->location = item["location"] | "";

        JsonObject start = item["start"];
        JsonObject end = item["end"];

        if (start.containsKey("date")) {
            // All-day event
            event->allDay = true;
            event->date = start["date"].as<String>();
            // Extract day of month
            if (event->date.length() >= 10) {
                event->dayOfMonth = event->date.substring(8, 10).toInt();
            }
        } else if (start.containsKey("dateTime")) {
            // Timed event
            event->allDay = false;
            String startDateTime = start["dateTime"];
            event->date = startDateTime.substring(0, 10);
            event->startTime = startDateTime.substring(0, 19);
            // Extract day of month
            if (event->date.length() >= 10) {
                event->dayOfMonth = event->date.substring(8, 10).toInt();
            }

            if (end.containsKey("dateTime")) {
                event->endTime = end["dateTime"].as<String>().substring(0, 19);
            }
        }

        // Check if today or tomorrow
        time_t now;
        time(&now);
        struct tm* nowTm = localtime(&now);

        int eventYear = event->date.substring(0, 4).toInt();
        int eventMonth = event->date.substring(5, 7).toInt();
        int eventDay = event->date.substring(8, 10).toInt();

        if (eventYear == nowTm->tm_year + 1900 &&
            eventMonth == nowTm->tm_mon + 1 &&
            eventDay == nowTm->tm_mday) {
            event->isToday = true;
        } else if (eventYear == nowTm->tm_year + 1900 &&
                 eventMonth == nowTm->tm_mon + 1 &&
                 eventDay == nowTm->tm_mday + 1) {
            event->isTomorrow = true;
        }

        events.push_back(event);
    }

    return events;
}

std::vector<CalendarEvent*> CalendarClient::fetchCalDAVEvents(int daysAhead) {
    // CalDAV implementation would go here
    // This is more complex and requires XML parsing and PROPFIND requests
    std::vector<CalendarEvent*> events;
    Serial.println("CalDAV support not yet implemented");
    return events;
}

void CalendarClient::sortEventsByTime(std::vector<CalendarEvent*>& events) {
    std::sort(events.begin(), events.end(), [](const CalendarEvent* a, const CalendarEvent* b) {
        // Compare dates first
        if (a->date != b->date) {
            return a->date < b->date;
        }
        // Then compare start times
        return a->startTime < b->startTime;
    });
}

void CalendarClient::filterPastEvents(std::vector<CalendarEvent*>& events) {
    // Remove events from past days, but keep ALL of today's events (even if they've already occurred)
    time_t now;
    time(&now);
    struct tm* nowTm = localtime(&now);

    Serial.println("Filtering past events - keeping all of today's events");
    Serial.println("Current date for filtering: " + String(nowTm->tm_year + 1900) + "-" +
                   String(nowTm->tm_mon + 1) + "-" + String(nowTm->tm_mday));

    // Count events before filtering
    int beforeCount = events.size();
    int todayEventsBefore = 0;
    for (auto e : events) {
        if (e->isToday) {
            todayEventsBefore++;
            String titleLower = e->title;
            titleLower.toLowerCase();
            if (titleLower.indexOf("birthday") >= 0 || titleLower.indexOf("compleanno") >= 0) {
                Serial.println("BEFORE FILTER - Birthday marked as today: " + e->title);
            }
        }
    }
    Serial.println("Before filtering: " + String(beforeCount) + " total events, " +
                   String(todayEventsBefore) + " today events");

    // Create a new vector for kept events and free removed ones
    std::vector<CalendarEvent*> filteredEvents;
    for (auto event : events) {
        if (event->date.length() < 10) {
            Serial.println("Removing event with invalid date: " + event->title);
            event->~CalendarEvent();
            PSRAM_FREE(event);
            continue;
        }

        int eventYear = event->date.substring(0, 4).toInt();
        int eventMonth = event->date.substring(5, 7).toInt();
        int eventDay = event->date.substring(8, 10).toInt();

        bool keepEvent = false;

        // Check if event is marked as today
        if (event->isToday) {
            Serial.println("Event marked as TODAY: " + event->title + " on " + event->date + " - KEEPING");
            keepEvent = true;
        }
        // Check if it's a past event
        else if (eventYear < nowTm->tm_year + 1900) {
            Serial.println("Removing past year event: " + event->title);
        }
        else if (eventYear == nowTm->tm_year + 1900 && eventMonth < nowTm->tm_mon + 1) {
            Serial.println("Removing past month event: " + event->title);
        }
        else if (eventYear == nowTm->tm_year + 1900 && eventMonth == nowTm->tm_mon + 1 && eventDay < nowTm->tm_mday) {
            Serial.println("Removing past day event: " + event->title);
        }
        // For today's events, keep them all (even if they've already happened)
        else if (eventYear == nowTm->tm_year + 1900 && eventMonth == nowTm->tm_mon + 1 && eventDay == nowTm->tm_mday) {
            Serial.println("Keeping today's event (by date check): " + event->title);
            keepEvent = true;
        }
        else {
            keepEvent = true;  // Future event
        }

        if (keepEvent) {
            filteredEvents.push_back(event);
        } else {
            // Free memory for removed events
            event->~CalendarEvent();
            PSRAM_FREE(event);
        }
    }

    // Replace the original vector with the filtered one
    events = filteredEvents;

    // Count events after filtering
    int afterCount = events.size();
    int todayEventsAfter = 0;
    for (auto e : events) {
        if (e->isToday) {
            todayEventsAfter++;
            String titleLower = e->title;
            titleLower.toLowerCase();
            if (titleLower.indexOf("birthday") >= 0 || titleLower.indexOf("compleanno") >= 0) {
                Serial.println("AFTER FILTER - Birthday still marked as today: " + e->title);
            }
        }
    }
    Serial.println("After filtering: " + String(afterCount) + " total events, " +
                   String(todayEventsAfter) + " today events");
    Serial.println("Removed " + String(beforeCount - afterCount) + " events");
}

void CalendarClient::limitEvents(std::vector<CalendarEvent*>& events, int maxEvents) {
    if (events.size() > maxEvents) {
        // Free memory for events that will be removed
        for (size_t i = maxEvents; i < events.size(); i++) {
            if (events[i]) {
                events[i]->~CalendarEvent();
                PSRAM_FREE(events[i]);
            }
        }
        events.resize(maxEvents);
    }
}

String CalendarClient::urlEncode(const String& str) {
    String encoded = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += "%";
            if (c < 16) encoded += "0";
            encoded += String(c, HEX);
        }
    }
    return encoded;
}

// Memory management - properly free allocated events
void CalendarClient::freeEvents(std::vector<CalendarEvent*>& events) {
    for (auto event : events) {
        if (event) {
            // Call destructor explicitly for String members
            event->~CalendarEvent();
            // Free the memory
            PSRAM_FREE(event);
        }
    }
    events.clear();
}

// Fetch events from a single ICS calendar with metadata
std::vector<CalendarEvent*> CalendarClient::fetchICSEvents(const String& url, int daysAhead,
                                                           const String& calendarName,
                                                           const String& calendarColor) {
    // Fetch events using the existing method
    std::vector<CalendarEvent*> events = fetchICSEvents(url, daysAhead);

    // Add calendar metadata to each event
    for (auto event : events) {
        event->calendarName = calendarName;
        event->calendarColor = calendarColor;
    }

    return events;
}

// Fetch events from multiple calendars and merge them (max 3 calendars)
std::vector<CalendarEvent*> CalendarClient::fetchMultipleCalendars(const std::vector<CalendarConfig>& calendars) {
    std::vector<CalendarEvent*> allEvents;
    int successCount = 0;
    int failCount = 0;
    int processedCount = 0;

    int calendarLimit = calendars.size() > 3 ? 3 : calendars.size();
    Serial.println("\n--- Fetching from " + String(calendarLimit) + " calendars (max 3) ---");

    for (const auto& calendar : calendars) {
        if (processedCount >= 3) {
            Serial.println("Maximum 3 calendars limit reached");
            break;
        }
        if (!calendar.enabled) {
            Serial.println("Skipping disabled calendar: " + calendar.name);
            continue;
        }

        Serial.println("\nFetching calendar: " + calendar.name);
        Serial.println("  URL: " + calendar.url);
        Serial.println("  Color: " + calendar.color);
        Serial.println("  Days to fetch: " + String(calendar.days_to_fetch));
        Serial.println("  Enabled: " + String(calendar.enabled));
        processedCount++;

        // Check if this is a local file or remote URL
        std::vector<CalendarEvent*> calendarEvents;
        if (calendar.url.startsWith("/") || calendar.url.startsWith("local://")) {
            // Local ICS file in LittleFS
            String localPath = calendar.url;
            if (localPath.startsWith("local://")) {
                localPath = localPath.substring(8); // Remove "local://" prefix
            }
            calendarEvents = fetchLocalICSEvents(localPath, calendar.days_to_fetch);

            // Add calendar metadata to local events
            for (auto event : calendarEvents) {
                event->calendarName = calendar.name;
                event->calendarColor = calendar.color;
            }
        } else {
            // Remote ICS URL
            calendarEvents = fetchICSEvents(
                calendar.url, calendar.days_to_fetch, calendar.name, calendar.color
            );
        }

        if (!calendarEvents.empty()) {
            Serial.println("  Retrieved " + String(calendarEvents.size()) + " events");

            // Debug: show some events from this calendar
            int debugCount = 0;
            for (auto e : calendarEvents) {
                if (debugCount < 3 || e->date.indexOf("2025-10-26") >= 0) {
                    Serial.println("    Event: " + e->title + " on " + e->date);
                    if (e->date.indexOf("2025-10-26") >= 0) {
                        Serial.println("    *** FOUND OCTOBER 26 EVENT IN CALENDAR: " + calendar.name + " ***");
                    }
                }
                debugCount++;
            }

            // Merge events into the main list
            allEvents.insert(allEvents.end(), calendarEvents.begin(), calendarEvents.end());
            successCount++;
        } else {
            Serial.println("  No events retrieved or fetch failed");
            failCount++;
        }
    }

    Serial.println("\n--- Calendar fetch summary ---");
    Serial.println("Successful: " + String(successCount) + " calendars");
    Serial.println("Failed: " + String(failCount) + " calendars");
    Serial.println("Total events: " + String(allEvents.size()));

    // Debug: Show today's events in the merged list
    Serial.println("=== MERGED EVENTS - TODAY'S EVENTS ===");
    int todayEventCount = 0;
    for (auto e : allEvents) {
        if (e->isToday) {
            todayEventCount++;
            Serial.println("MERGED TODAY EVENT #" + String(todayEventCount) + ": " + e->title + " from calendar: " + e->calendarName);
            String titleLower = e->title;
            titleLower.toLowerCase();
            if (titleLower.indexOf("birthday") >= 0 || titleLower.indexOf("compleanno") >= 0) {
                Serial.println("  *** BIRTHDAY EVENT IN MERGED LIST ***");
            }
        }
    }
    Serial.println("Total TODAY events in merged list: " + String(todayEventCount));

    // Sort all events by date/time
    if (!allEvents.empty()) {
        sortEventsByTime(allEvents);
        Serial.println("Events sorted chronologically");

        // Filter past events after merging
        if (!SHOW_PAST_EVENTS) {
            Serial.println("Filtering past events from merged list...");
            filterPastEvents(allEvents);
        }

        // Limit to maximum number of events
        if (allEvents.size() > MAX_EVENTS_TO_SHOW) {
            Serial.println("Limiting merged events from " + String(allEvents.size()) + " to " + String(MAX_EVENTS_TO_SHOW));
            limitEvents(allEvents, MAX_EVENTS_TO_SHOW);
        }
    }

    Serial.println("Final event count after filtering/limiting: " + String(allEvents.size()));

    // Debug: check for October 26 in the final merged/filtered list
    int oct26Final = 0;
    for (auto e : allEvents) {
        if (e->date.length() >= 10) {
            int month = e->date.substring(5, 7).toInt();
            int day = e->date.substring(8, 10).toInt();
            if (month == 10 && day == 26) {
                oct26Final++;
                Serial.println("OCTOBER 26 EVENT IN FINAL MERGED LIST: " + e->title + " from " + e->calendarName);
            }
        }
    }
    Serial.println("October 26 events in final merged list: " + String(oct26Final));

    return allEvents;
}