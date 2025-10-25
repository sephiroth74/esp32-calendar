#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <Arduino.h>
#include <time.h>

class DateUtils {
public:
    // Check if a given timestamp is today
    static bool isToday(time_t timestamp);

    // Check if a given timestamp is tomorrow
    static bool isTomorrow(time_t timestamp);

    // Check if a given timestamp is yesterday
    static bool isYesterday(time_t timestamp);

    // Check if two timestamps are on the same day
    static bool isSameDay(time_t timestamp1, time_t timestamp2);

    // Get the start of day (midnight) for a given timestamp
    static time_t getStartOfDay(time_t timestamp);

    // Get the end of day (23:59:59) for a given timestamp
    static time_t getEndOfDay(time_t timestamp);

    // Get days difference between two dates (ignoring time)
    static int getDaysDifference(time_t timestamp1, time_t timestamp2);

    // Format date string from timestamp (YYYY-MM-DD)
    static String formatDate(time_t timestamp);

    // Format time string from timestamp (HH:MM)
    static String formatTime(time_t timestamp);

    // Parse date string (YYYY-MM-DD) to timestamp
    static time_t parseDate(const String& dateStr);

    // Get current time with proper NTP sync check
    static time_t getCurrentTime();

    // Check if time is properly synchronized
    static bool isTimeSynchronized();

private:
    // Helper to normalize a timestamp to midnight
    static time_t normalizeToMidnight(time_t timestamp);
};

#endif // DATE_UTILS_H