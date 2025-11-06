#include "date_utils.h"
#include "debug_config.h"

bool DateUtils::isToday(time_t timestamp) {
    time_t now = getCurrentTime();
    return isSameDay(now, timestamp);
}

bool DateUtils::isTomorrow(time_t timestamp) {
    time_t now      = getCurrentTime();
    time_t tomorrow = now + 86400; // Add 24 hours
    return isSameDay(tomorrow, timestamp);
}

bool DateUtils::isYesterday(time_t timestamp) {
    time_t now       = getCurrentTime();
    time_t yesterday = now - 86400; // Subtract 24 hours
    return isSameDay(yesterday, timestamp);
}

bool DateUtils::isSameDay(time_t timestamp1, time_t timestamp2) {
    struct tm* tm1 = localtime(&timestamp1);
    int year1      = tm1->tm_year;
    int month1     = tm1->tm_mon;
    int day1       = tm1->tm_mday;

    struct tm* tm2 = localtime(&timestamp2);
    int year2      = tm2->tm_year;
    int month2     = tm2->tm_mon;
    int day2       = tm2->tm_mday;

    return (year1 == year2 && month1 == month2 && day1 == day2);
}

time_t DateUtils::getStartOfDay(time_t timestamp) {
    struct tm* tm = localtime(&timestamp);
    tm->tm_hour   = 0;
    tm->tm_min    = 0;
    tm->tm_sec    = 0;
    return mktime(tm);
}

time_t DateUtils::getEndOfDay(time_t timestamp) {
    struct tm* tm = localtime(&timestamp);
    tm->tm_hour   = 23;
    tm->tm_min    = 59;
    tm->tm_sec    = 59;
    return mktime(tm);
}

int DateUtils::getDaysDifference(time_t timestamp1, time_t timestamp2) {
    time_t day1 = normalizeToMidnight(timestamp1);
    time_t day2 = normalizeToMidnight(timestamp2);

    // Calculate difference in seconds and convert to days
    int diffSeconds = difftime(day2, day1);
    return diffSeconds / 86400; // 86400 seconds in a day
}

String DateUtils::formatDate(time_t timestamp) {
    if (timestamp == 0)
        return "";

    struct tm* tm = localtime(&timestamp);
    char buffer[32]; // Increased buffer size to avoid truncation warning
    snprintf(
        buffer, sizeof(buffer), "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return String(buffer);
}

String DateUtils::formatTime(time_t timestamp) {
    if (timestamp == 0)
        return "";

    struct tm* tm = localtime(&timestamp);
    char buffer[32]; // Using larger buffer to avoid compiler warnings
    snprintf(buffer, sizeof(buffer), "%02d:%02d", tm->tm_hour, tm->tm_min);
    return String(buffer);
}

time_t DateUtils::parseDate(const String& dateStr) {
    // Parse YYYY-MM-DD format
    if (dateStr.length() < 10) {
        DEBUG_ERROR_PRINTLN("Invalid date string: " + dateStr);
        return 0;
    }

    int year     = dateStr.substring(0, 4).toInt();
    int month    = dateStr.substring(5, 7).toInt();
    int day      = dateStr.substring(8, 10).toInt();

    struct tm tm = {0};
    tm.tm_year   = year - 1900;
    tm.tm_mon    = month - 1;
    tm.tm_mday   = day;
    tm.tm_hour   = 0;
    tm.tm_min    = 0;
    tm.tm_sec    = 0;
    tm.tm_isdst  = -1;

    return mktime(&tm);
}

time_t DateUtils::getCurrentTime() {
    time_t now;
    time(&now);

    // Check if time is properly synchronized
    if (!isTimeSynchronized()) {
        DEBUG_WARN_PRINTLN("Time not synchronized! Current time may be incorrect.");
    }

    return now;
}

bool DateUtils::isTimeSynchronized() {
    time_t now;
    time(&now);

    // If time is before year 2020, it's probably not synchronized
    struct tm* tm = localtime(&now);
    int year      = tm->tm_year + 1900;

    return year >= 2020;
}

time_t DateUtils::normalizeToMidnight(time_t timestamp) {
    struct tm* tm = localtime(&timestamp);
    tm->tm_hour   = 0;
    tm->tm_min    = 0;
    tm->tm_sec    = 0;
    return mktime(tm);
}