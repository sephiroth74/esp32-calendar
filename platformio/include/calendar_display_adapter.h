#ifndef CALENDAR_DISPLAY_ADAPTER_H
#define CALENDAR_DISPLAY_ADAPTER_H

#include "calendar_event.h"
#include <ctime>
#include <vector>

/**
 * CalendarDisplayAdapter provides compatibility between the new CalendarEvent class
 * and the existing DisplayManager expectations.
 *
 * This adapter adds display-specific fields that DisplayManager needs:
 * - title (from summary)
 * - date (formatted from startTime)
 * - startTime/endTime strings (formatted for display)
 * - location (already in CalendarEvent)
 * - isToday/isTomorrow flags
 * - dayOfMonth for calendar highlighting
 */
class CalendarDisplayAdapter {
  public:
    /**
     * Prepare events for display by adding display-specific fields
     * This modifies the events in place by setting additional fields needed by DisplayManager
     */
    static void prepareEventsForDisplay(std::vector<CalendarEvent*>& events);

    /**
     * Add display fields to a single event
     */
    static void prepareEventForDisplay(CalendarEvent* event);

  private:
    /**
     * Format date from timestamp to YYYY-MM-DD format
     */
    static String formatDate(time_t timestamp);

    /**
     * Format time from timestamp to HH:MM format
     */
    static String formatTime(time_t timestamp);

    /**
     * Check if date is today
     */
    static bool isToday(time_t timestamp);

    /**
     * Check if date is tomorrow
     */
    static bool isTomorrow(time_t timestamp);

    /**
     * Get day of month from timestamp
     */
    static int getDayOfMonth(time_t timestamp);
};

#endif // CALENDAR_DISPLAY_ADAPTER_H