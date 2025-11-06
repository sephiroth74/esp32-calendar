#include "calendar_display_adapter.h"
#include "date_utils.h"
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

    // Format date for display
    if (event->startTime > 0) {
        event->date = formatDate(event->startTime);
        // Note: startTimeStr and startDate are now computed on-demand via getters

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
        // Note: startDate is now computed on-demand via getStartDate()
        event->dayOfMonth = 0;
        event->isToday = false;
        event->isTomorrow = false;
    }

    // Note: endTimeStr and endDate are now computed on-demand via getters

    // Ensure location is set (even if empty)
    if (event->location.isEmpty()) {
        event->location = "";
    }
}

String CalendarDisplayAdapter::formatDate(time_t timestamp) {
    return DateUtils::formatDate(timestamp);
}

String CalendarDisplayAdapter::formatTime(time_t timestamp) {
    return DateUtils::formatTime(timestamp);
}

bool CalendarDisplayAdapter::isToday(time_t timestamp) {
    return DateUtils::isToday(timestamp);
}

bool CalendarDisplayAdapter::isTomorrow(time_t timestamp) {
    return DateUtils::isTomorrow(timestamp);
}

int CalendarDisplayAdapter::getDayOfMonth(time_t timestamp) {
    if (timestamp == 0) return 0;

    struct tm* timeinfo = localtime(&timestamp);
    return timeinfo->tm_mday;
}