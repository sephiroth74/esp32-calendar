#include "calendar_event.h"
#include "timezone_map.h"
#include "date_utils.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>  // for setenv, getenv

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
    isHoliday = false;
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
    return DateUtils::formatDate(timestamp);
}

String CalendarEvent::formatTime(time_t timestamp) const {
    return DateUtils::formatTime(timestamp);
}

// Computed property getters (memory optimization)
String CalendarEvent::getStartDate() const {
    return formatDate(startTime);
}

String CalendarEvent::getEndDate() const {
    return formatDate(endTime);
}

String CalendarEvent::getStartTimeStr() const {
    return allDay ? "" : formatTime(startTime);
}

String CalendarEvent::getEndTimeStr() const {
    return allDay ? "" : formatTime(endTime);
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
    String startDate = getStartDate();
    if (!startDate.isEmpty()) {
        result += " (" + startDate;
        String startTimeStr = getStartTimeStr();
        if (!startTimeStr.isEmpty()) {
            result += " " + startTimeStr;
        }
        result += ")";
    }
    result += "\n";

    if (!dtEnd.isEmpty()) {
        result += "  End: " + dtEnd;
        String endDate = getEndDate();
        if (!endDate.isEmpty()) {
            result += " (" + endDate;
            String endTimeStr = getEndTimeStr();
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

/**
 * Helper function to parse ICS datetime string into tm struct
 *
 * @param dt ICS datetime string (e.g., "20120119T150000")
 * @param timeinfo Output tm struct to populate
 * @return true if parsing succeeded, false otherwise
 */
static bool parseICSDateTimeToTm(const String& dt, struct tm& timeinfo) {
    if (dt.isEmpty() || dt.length() < 8) {
        return false;
    }

    memset(&timeinfo, 0, sizeof(struct tm));

    int year, month, day, hour = 0, min = 0, sec = 0;

    // Parse date: YYYYMMDD
    if (sscanf(dt.c_str(), "%4d%2d%2d", &year, &month, &day) != 3) {
        return false;
    }

    // Parse time if present: THHMMSS
    if (dt.length() >= 15 && dt.charAt(8) == 'T') {
        if (sscanf(dt.c_str() + 9, "%2d%2d%2d", &hour, &min, &sec) != 3) {
            return false;
        }
    }

    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;
    timeinfo.tm_isdst = -1;  // Let mktime determine DST

    return true;
}

/**
 * Parse ICS datetime string with timezone conversion
 *
 * Converts a datetime from the specified timezone to UTC using ESP32's built-in
 * timezone support via POSIX TZ strings.
 *
 * @param dt ICS datetime string (e.g., "20120119T150000")
 * @param tzid IANA timezone identifier (e.g., "Europe/Amsterdam")
 * @return UTC timestamp, or -1 if parsing fails
 */
time_t CalendarEvent::parseICSDateTimeWithTZ(const String& dt, const String& tzid) {
    if (dt.isEmpty()) {
        return -1;
    }

    // Get POSIX TZ string for this IANA timezone ID
    String posixTZ = getPosixTZ(tzid);

    if (posixTZ.isEmpty()) {
        // Unknown timezone - fall back to parsing as UTC
        struct tm timeinfo;
        if (!parseICSDateTimeToTm(dt, timeinfo)) {
            return -1;
        }
        timeinfo.tm_isdst = 0;  // UTC has no DST

        // For UTC, use timegm (or mktime after setting TZ=UTC)
#ifndef ARDUINO
        return timegm(&timeinfo);  // Available in POSIX
#else
        // On Arduino, temporarily set UTC
        char* oldTZ = getenv("TZ");
        String savedTZ = oldTZ ? String(oldTZ) : "";
        setenv("TZ", "UTC0", 1);
        tzset();
        time_t result = mktime(&timeinfo);
        if (!savedTZ.isEmpty()) {
            setenv("TZ", savedTZ.c_str(), 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
        return result;
#endif
    }

    // Save current TZ environment variable
    char* oldTZ = getenv("TZ");
    String savedTZ = oldTZ ? String(oldTZ) : "";

    // Temporarily set TZ to the event's timezone
    setenv("TZ", posixTZ.c_str(), 1);
    tzset();  // Apply timezone change

    // Parse datetime string: YYYYMMDDTHHmmSS
    struct tm timeinfo;
    if (!parseICSDateTimeToTm(dt, timeinfo)) {
        // Restore original TZ before returning
        if (!savedTZ.isEmpty()) {
            setenv("TZ", savedTZ.c_str(), 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
        return -1;
    }

    // mktime interprets timeinfo as local time (in our set TZ) and returns UTC timestamp
    time_t result = mktime(&timeinfo);

    // Restore original TZ environment variable
    if (!savedTZ.isEmpty()) {
        setenv("TZ", savedTZ.c_str(), 1);
    } else {
        unsetenv("TZ");
    }
    tzset();  // Apply timezone restoration

    return result;
}

