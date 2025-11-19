#include "calendar_event.h"
#include "date_utils.h"
#include "timezone_map.h"
#include <cstdio>
#include <cstdlib> // for setenv, getenv
#include <cstring>

CalendarEvent::CalendarEvent() { clear(); }

CalendarEvent::~CalendarEvent() {
    // Nothing to clean up as we use String objects
}

void CalendarEvent::clear() {
    // Clear all string properties
    uid           = "";
    summary       = "";
    description   = "";

    allDay        = false;
    startTime     = 0;
    endTime       = 0;

    rrule         = "";
    rdate         = "";
    exdate        = "";

    title         = "";
    date          = "";
    recurrenceId  = "";
    isRecurring   = false;

    status        = "";
    transp        = "";
    eventClass    = "";
    priority      = 0;
    sequence      = 0;

    calendarName  = "";
    calendarColor = "";

    alarm         = "";
    timezone      = "";
    url           = "";
    attach        = "";

    isToday       = false;
    isTomorrow    = false;
    dayOfMonth    = 0;
    isHoliday     = false;
}

bool CalendarEvent::isValid() const {
    // An event must have at least a UID and a valid start time
    return !uid.isEmpty() && startTime > 0;
}

bool CalendarEvent::setStartDateTime(const String& value, const String& tzid, bool isDate) {
    // Check if value ends with 'Z' (UTC indicator)
    bool hasZ = !value.isEmpty() && value.charAt(value.length() - 1) == 'Z';

    // Set allDay flag
    allDay = isDate;

    // Parse the datetime
    startTime = parseICSDateTime(value, tzid, isDate, hasZ);

    // Automatically populate the date field for backward compatibility
    if (startTime > 0) {
        date = getStartDate();
    }

    return startTime > 0;
}

bool CalendarEvent::setEndDateTime(const String& value, const String& tzid, bool isDate) {
    // Check if value ends with 'Z' (UTC indicator)
    bool hasZ = !value.isEmpty() && value.charAt(value.length() - 1) == 'Z';

    // Parse the datetime
    endTime = parseICSDateTime(value, tzid, isDate, hasZ);

    return endTime > 0;
}

time_t CalendarEvent::parseICSDateTime(const String& value, const String& tzid, bool isDate, bool hasZ) const {
    // Parse ICS date/time format: YYYYMMDD[THHMMSS[Z]]
    // Supports 4 formats:
    //   1. DATE-TIME (UTC): "20251119T103000Z" (hasZ=true)
    //   2. DATE-TIME (TZID): "20251119T140000" with tzid (tzid!="")
    //   3. DATE-TIME (Floating): "20251119T080000" (tzid="", hasZ=false)
    //   4. DATE (All-Day): "20251119" (isDate=true)

    if (value.length() < 8) {
        return 0; // Invalid format
    }

    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));

    // Parse date components (YYYYMMDD)
    String yearStr  = value.substring(0, 4);
    String monthStr = value.substring(4, 6);
    String dayStr   = value.substring(6, 8);

    timeinfo.tm_year = yearStr.toInt() - 1900;
    timeinfo.tm_mon  = monthStr.toInt() - 1;
    timeinfo.tm_mday = dayStr.toInt();

    // Parse time components if present (THHMMSS)
    if (!isDate && value.length() >= 15 && value.charAt(8) == 'T') {
        String hourStr = value.substring(9, 11);
        String minStr  = value.substring(11, 13);
        String secStr  = value.substring(13, 15);

        timeinfo.tm_hour = hourStr.toInt();
        timeinfo.tm_min  = minStr.toInt();
        timeinfo.tm_sec  = secStr.toInt();
    } else {
        // All-day event or date-only: set time to midnight
        timeinfo.tm_hour = 0;
        timeinfo.tm_min  = 0;
        timeinfo.tm_sec  = 0;
    }

    timeinfo.tm_isdst = -1; // Let mktime determine DST

    // Save current timezone
    char* oldTZ = getenv("TZ");
    String savedTZ = oldTZ ? String(oldTZ) : "";

    time_t result = 0;

    if (hasZ) {
        // Format 1: DATE-TIME (UTC) - "20251119T103000Z"
        // Set timezone to UTC for parsing
        setenv("TZ", "UTC0", 1);
        tzset();
        result = mktime(&timeinfo);

    } else if (!tzid.isEmpty()) {
        // Format 2: DATE-TIME (TZID) - "20251119T140000" with tzid="America/Los_Angeles"
        // Set timezone to the specified TZID for parsing
        setenv("TZ", tzid.c_str(), 1);
        tzset();
        result = mktime(&timeinfo);

    } else {
        // Format 3: DATE-TIME (Floating) or Format 4: DATE (All-Day)
        // Use local timezone (don't change TZ)
        result = mktime(&timeinfo);
    }

    // Restore original timezone
    if (!savedTZ.isEmpty()) {
        setenv("TZ", savedTZ.c_str(), 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return result;
}

String CalendarEvent::formatDate(time_t timestamp) const {
    return DateUtils::formatDate(timestamp);
}

String CalendarEvent::formatTime(time_t timestamp) const {
    return DateUtils::formatTime(timestamp);
}

// Computed property getters (memory optimization)
String CalendarEvent::getStartDate() const { return formatDate(startTime); }

String CalendarEvent::getEndDate() const { return formatDate(endTime); }

String CalendarEvent::getStartTimeStr() const { return allDay ? "" : formatTime(startTime); }

String CalendarEvent::getEndTimeStr() const { return allDay ? "" : formatTime(endTime); }

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

bool CalendarEvent::operator==(const CalendarEvent& other) const { return uid == other.uid; }

String CalendarEvent::toString() const {
    String result = "CalendarEvent {\n";
    result += "  UID: " + uid + "\n";
    result += "  Summary: " + summary + "\n";

    if (startTime > 0) {
        String startDate = getStartDate();
        result += "  Start: " + startDate;
        if (!allDay) {
            String startTimeStr = getStartTimeStr();
            if (!startTimeStr.isEmpty()) {
                result += " " + startTimeStr;
            }
        }
        result += " (timestamp: " + String((unsigned long)startTime) + ")\n";
    }

    if (endTime > 0) {
        String endDate = getEndDate();
        result += "  End: " + endDate;
        if (!allDay) {
            String endTimeStr = getEndTimeStr();
            if (!endTimeStr.isEmpty()) {
                result += " " + endTimeStr;
            }
        }
        result += " (timestamp: " + String((unsigned long)endTime) + ")\n";
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

    timeinfo.tm_year  = year - 1900;
    timeinfo.tm_mon   = month - 1;
    timeinfo.tm_mday  = day;
    timeinfo.tm_hour  = hour;
    timeinfo.tm_min   = min;
    timeinfo.tm_sec   = sec;
    timeinfo.tm_isdst = -1; // Let mktime determine DST

    return true;
}