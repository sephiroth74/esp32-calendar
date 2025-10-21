#include "calendar_event.h"
#include <cstring>
#include <cstdio>

CalendarEvent::CalendarEvent() {
    clear();
}

CalendarEvent::~CalendarEvent() {
    // Nothing to clean up as we use String objects
}

void CalendarEvent::clear() {
    // Clear all string properties
    uid = "";
    summary = "";
    description = "";
    location = "";

    dtStart = "";
    dtEnd = "";
    duration = "";
    allDay = false;

    startTime = 0;
    endTime = 0;
    startDate = "";
    endDate = "";
    startTimeStr = "";
    endTimeStr = "";

    rrule = "";
    rdate = "";
    exdate = "";
    recurrenceId = "";
    isRecurring = false;

    status = "";
    transp = "";
    eventClass = "";
    priority = 0;
    sequence = 0;

    organizer = "";
    attendees = "";

    created = "";
    lastModified = "";
    dtStamp = "";

    calendarName = "";
    calendarColor = "";
    categories = "";

    alarm = "";
    timezone = "";
    url = "";
    attach = "";

    isToday = false;
    isTomorrow = false;
    dayOfMonth = 0;
}

bool CalendarEvent::isValid() const {
    // An event must have at least a UID and a start time
    return !uid.isEmpty() && !dtStart.isEmpty();
}

bool CalendarEvent::setStartDateTime(const String& dt, const String& params) {
    dtStart = dt;

    // Check if it's an all-day event
    if (params.indexOf("VALUE=DATE") != -1 || dt.length() == 8) {
        allDay = true;
        startTime = parseICSDateTime(dt, false);
    } else {
        allDay = false;
        bool isUTC = dt.charAt(dt.length() - 1) == 'Z';
        startTime = parseICSDateTime(dt, isUTC);
    }

    // Format the date and time strings
    if (startTime > 0) {
        startDate = formatDate(startTime);
        if (!allDay) {
            startTimeStr = formatTime(startTime);
        }
    }

    return startTime > 0;
}

bool CalendarEvent::setEndDateTime(const String& dt, const String& params) {
    dtEnd = dt;

    // Check if it's an all-day event
    bool isAllDay = params.indexOf("VALUE=DATE") != -1 || dt.length() == 8;

    if (isAllDay) {
        endTime = parseICSDateTime(dt, false);
    } else {
        bool isUTC = dt.charAt(dt.length() - 1) == 'Z';
        endTime = parseICSDateTime(dt, isUTC);
    }

    // Format the date and time strings
    if (endTime > 0) {
        endDate = formatDate(endTime);
        if (!allDay) {
            endTimeStr = formatTime(endTime);
        }
    }

    return endTime > 0;
}

time_t CalendarEvent::parseICSDateTime(const String& dateTime, bool isUTC) const {
    // Parse ICS date/time format: YYYYMMDD[THHMMSS[Z]]

    if (dateTime.length() < 8) {
        return 0; // Invalid format
    }

    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));

    // Parse date components
    String yearStr = dateTime.substring(0, 4);
    String monthStr = dateTime.substring(4, 6);
    String dayStr = dateTime.substring(6, 8);

    timeinfo.tm_year = yearStr.toInt() - 1900;
    timeinfo.tm_mon = monthStr.toInt() - 1;
    timeinfo.tm_mday = dayStr.toInt();

    // Parse time components if present
    if (dateTime.length() >= 15 && dateTime.charAt(8) == 'T') {
        String hourStr = dateTime.substring(9, 11);
        String minStr = dateTime.substring(11, 13);
        String secStr = dateTime.substring(13, 15);

        timeinfo.tm_hour = hourStr.toInt();
        timeinfo.tm_min = minStr.toInt();
        timeinfo.tm_sec = secStr.toInt();
    }

    // Convert to time_t
    // Note: mktime interprets the time as local time
    // For UTC times, we'd need to adjust for timezone offset
    time_t result = mktime(&timeinfo);

    // If UTC, adjust for local timezone
    // This is a simplified approach - proper timezone handling would be more complex
    if (isUTC) {
        // Get local timezone offset
        // This is platform-specific and simplified
        // In a real implementation, you'd use proper timezone libraries
    }

    return result;
}

String CalendarEvent::formatDate(time_t timestamp) const {
    if (timestamp == 0) return "";

    struct tm* timeinfo = localtime(&timestamp);
    // Buffer size: YYYY-MM-DD = 10 chars + null terminator
    // But compiler wants space for theoretical max int values
    char buffer[64];  // Large enough for any possible output
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday);
    return String(buffer);
}

String CalendarEvent::formatTime(time_t timestamp) const {
    if (timestamp == 0) return "";

    struct tm* timeinfo = localtime(&timestamp);
    // Buffer size: HH:MM = 5 chars + null terminator
    // Using larger buffer to avoid compiler warnings
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d:%02d",
             timeinfo->tm_hour,
             timeinfo->tm_min);
    return String(buffer);
}

bool CalendarEvent::operator<(const CalendarEvent& other) const {
    // Compare by start time first
    if (startTime != other.startTime) {
        return startTime < other.startTime;
    }

    // If same start time, all-day events come first
    if (allDay != other.allDay) {
        return allDay;
    }

    // Finally, compare by UID for consistent ordering
    return uid < other.uid;
}

bool CalendarEvent::operator==(const CalendarEvent& other) const {
    return uid == other.uid;
}

String CalendarEvent::toString() const {
    String result = "CalendarEvent {\n";
    result += "  UID: " + uid + "\n";
    result += "  Summary: " + summary + "\n";
    result += "  Start: " + dtStart;
    if (!startDate.isEmpty()) {
        result += " (" + startDate;
        if (!startTimeStr.isEmpty()) {
            result += " " + startTimeStr;
        }
        result += ")";
    }
    result += "\n";

    if (!dtEnd.isEmpty()) {
        result += "  End: " + dtEnd;
        if (!endDate.isEmpty()) {
            result += " (" + endDate;
            if (!endTimeStr.isEmpty()) {
                result += " " + endTimeStr;
            }
            result += ")";
        }
        result += "\n";
    }

    if (!location.isEmpty()) {
        result += "  Location: " + location + "\n";
    }

    if (!description.isEmpty()) {
        result += "  Description: " + description + "\n";
    }

    if (allDay) {
        result += "  All-day: true\n";
    }

    if (isRecurring) {
        result += "  Recurring: true\n";
        if (!rrule.isEmpty()) {
            result += "  RRULE: " + rrule + "\n";
        }
    }

    result += "}";
    return result;
}

void CalendarEvent::print() const {
#ifndef NATIVE_TEST
    Serial.println(toString());
#endif
}

