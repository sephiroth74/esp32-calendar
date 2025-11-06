#ifndef CALENDAR_EVENT_H
#define CALENDAR_EVENT_H

#ifdef NATIVE_TEST
#include "../test/mock_arduino.h"
#else
#include <Arduino.h>
#endif

#include <ctime>

#ifndef POPULATE_TM_DATE_TIME
#define POPULATE_TM_DATE_TIME(tm_info, year, mon, mday, hour, min, sec, isdst)                     \
    do {                                                                                           \
        tm_info.tm_year  = (year) - 1900;                                                          \
        tm_info.tm_mon   = (mon) - 1;                                                              \
        tm_info.tm_mday  = (mday);                                                                 \
        tm_info.tm_hour  = (hour);                                                                 \
        tm_info.tm_min   = (min);                                                                  \
        tm_info.tm_sec   = (sec);                                                                  \
        tm_info.tm_isdst = (isdst);                                                                \
    } while (0)
#endif // POPULATE_TM_DATE_TIME

/**
 * Represents a single calendar event from an ICS file
 * This is a standalone class with no external dependencies
 */
class CalendarEvent {
  public:
    // Constructors
    CalendarEvent();
    ~CalendarEvent();

    // Event identification
    String uid;         // Unique identifier for the event
    String summary;     // Event title/summary (SUMMARY)
    String description; // Event description (DESCRIPTION)
    String location;    // Event location (LOCATION)

    // Date and time properties
    String dtStart;  // Start date/time in ICS format (DTSTART)
    String dtEnd;    // End date/time in ICS format (DTEND)
    String duration; // Duration of the event (DURATION)
    bool allDay;     // True if this is an all-day event

    // Parsed date/time for convenience
    time_t startTime; // Start time as Unix timestamp
    time_t endTime;   // End time as Unix timestamp

    // Computed properties (memory optimization - computed on-demand instead of stored)
    String getStartDate() const;    // Start date in YYYY-MM-DD format
    String getEndDate() const;      // End date in YYYY-MM-DD format
    String getStartTimeStr() const; // Start time in HH:MM format
    String getEndTimeStr() const;   // End time in HH:MM format

    // Recurrence properties
    String rrule;        // Recurrence rule (RRULE)
    String rdate;        // Recurrence dates (RDATE)
    String exdate;       // Exception dates (EXDATE)
    String recurrenceId; // Recurrence ID for modified instances
    bool isRecurring;    // True if event has recurrence rules

    // Status and classification
    String status;     // Event status (TENTATIVE, CONFIRMED, CANCELLED)
    String transp;     // Time transparency (TRANSPARENT, OPAQUE)
    String eventClass; // Event class (PUBLIC, PRIVATE, CONFIDENTIAL)
    int priority;      // Priority level (0-9, 0 = undefined)
    int sequence;      // Sequence number for updates

    // Organizer and attendees
    String organizer; // Event organizer
    String attendees; // Comma-separated list of attendees

    // Timestamps
    String created;      // Creation timestamp (CREATED)
    String lastModified; // Last modification timestamp (LAST-MODIFIED)
    String dtStamp;      // Timestamp of ICS creation (DTSTAMP)

    // Calendar metadata
    String calendarName;  // Name of the source calendar
    String calendarColor; // Color associated with this calendar
    String categories;    // Event categories (CATEGORIES)

    // Alarm/reminder
    String alarm; // Alarm/reminder settings

    // Timezone
    String timezone; // Timezone for the event (TZID)

    // URL and attachments
    String url;    // URL associated with the event
    String attach; // Attachments

    // Helper flags for display
    bool isToday;    // True if event is today
    bool isTomorrow; // True if event is tomorrow
    int dayOfMonth;  // Day of month (1-31) for calendar display
    bool isHoliday;  // True if this is a holiday (from holiday_calendar)

    // Compatibility fields for DisplayManager (set by CalendarDisplayAdapter)
    String title; // Alias for summary (for backward compatibility)
    String date;  // Formatted date string YYYY-MM-DD

    // Methods
    void clear();
    bool isValid() const;

    // Date/time parsing helpers
    bool parseDateTime(const String& icsDateTime, time_t& timestamp, bool& isAllDay);
    String formatDate(time_t timestamp) const;
    String formatTime(time_t timestamp) const;

    // Set date/time from ICS format
    bool setStartDateTime(const String& dtStart, const String& params = "");
    bool setEndDateTime(const String& dtEnd, const String& params = "");

    // Timezone-aware datetime parsing
    static time_t parseICSDateTimeWithTZ(const String& dt, const String& tzid);

    // Comparison operators
    bool operator<(const CalendarEvent& other) const;
    bool operator==(const CalendarEvent& other) const;

    // Debug output
    String toString() const;
    void print() const;

  private:
    // Helper methods
    time_t parseICSDateTime(const String& dateTime, bool isUTC = false) const;
    String timeToICSFormat(time_t timestamp, bool includeTime = true) const;
};

#endif // CALENDAR_EVENT_H