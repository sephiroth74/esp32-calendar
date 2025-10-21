#include "calendar_display_adapter.h"
#include <algorithm>

void CalendarDisplayAdapter::prepareEventsForDisplay(std::vector<CalendarEvent*>& events) {
    for (auto event : events) {
        prepareEventForDisplay(event);
    }

    // Sort events by start time
    std::sort(events.begin(), events.end(), [](CalendarEvent* a, CalendarEvent* b) {
        return a->startTime < b->startTime;
    });
}

void CalendarDisplayAdapter::prepareEventForDisplay(CalendarEvent* event) {
    if (!event) return;

    // Create display-friendly aliases for existing fields
    // Note: We're adding these as temporary compatibility fields
    // In the future, display_manager should be updated to use the standard fields

    // Set the 'title' field from 'summary' for backward compatibility
    // DisplayManager expects 'title' but CalendarEvent uses 'summary'
    event->title = event->summary;

    // Format date and time for display
    if (event->startTime > 0) {
        event->date = formatDate(event->startTime);
        event->startTimeStr = formatTime(event->startTime);
        event->startDate = event->date;  // Alias for compatibility

        // Set day of month for calendar highlighting
        event->dayOfMonth = getDayOfMonth(event->startTime);

        // Check if today or tomorrow
        event->isToday = isToday(event->startTime);
        event->isTomorrow = isTomorrow(event->startTime);
    } else {
        // Parse from dtStart if timestamp not available
        event->date = event->dtStart.substring(0, 10); // Extract YYYY-MM-DD
        if (event->date.indexOf('T') > 0) {
            event->date = event->date.substring(0, event->date.indexOf('T'));
        }
        event->startDate = event->date;
        event->dayOfMonth = 0;
        event->isToday = false;
        event->isTomorrow = false;
    }

    if (event->endTime > 0) {
        event->endTimeStr = formatTime(event->endTime);
        event->endDate = formatDate(event->endTime);
    } else if (!event->dtEnd.isEmpty()) {
        // Parse time from dtEnd if available
        int tPos = event->dtEnd.indexOf('T');
        if (tPos > 0 && tPos + 5 < event->dtEnd.length()) {
            event->endTimeStr = event->dtEnd.substring(tPos + 1, tPos + 6); // HH:MM
        }
    }

    // For all-day events, clear the time strings
    if (event->allDay) {
        event->startTimeStr = "";
        event->endTimeStr = "";
    }

    // Ensure location is set (even if empty)
    if (event->location.isEmpty()) {
        event->location = "";
    }
}

String CalendarDisplayAdapter::formatDate(time_t timestamp) {
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

String CalendarDisplayAdapter::formatTime(time_t timestamp) {
    if (timestamp == 0) return "";

    struct tm* timeinfo = localtime(&timestamp);
    // Buffer size: HH:MM = 5 chars + null terminator
    // Using larger buffer to avoid compiler warnings
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    return String(buffer);
}

bool CalendarDisplayAdapter::isToday(time_t timestamp) {
    time_t now;
    time(&now);

    struct tm* nowInfo = localtime(&now);
    struct tm* eventInfo = localtime(&timestamp);

    return (nowInfo->tm_year == eventInfo->tm_year &&
            nowInfo->tm_mon == eventInfo->tm_mon &&
            nowInfo->tm_mday == eventInfo->tm_mday);
}

bool CalendarDisplayAdapter::isTomorrow(time_t timestamp) {
    time_t now;
    time(&now);

    // Add 24 hours to get tomorrow
    time_t tomorrow = now + 86400;

    struct tm* tomorrowInfo = localtime(&tomorrow);
    struct tm* eventInfo = localtime(&timestamp);

    return (tomorrowInfo->tm_year == eventInfo->tm_year &&
            tomorrowInfo->tm_mon == eventInfo->tm_mon &&
            tomorrowInfo->tm_mday == eventInfo->tm_mday);
}

int CalendarDisplayAdapter::getDayOfMonth(time_t timestamp) {
    if (timestamp == 0) return 0;

    struct tm* timeinfo = localtime(&timestamp);
    return timeinfo->tm_mday;
}