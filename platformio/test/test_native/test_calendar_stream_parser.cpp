#include <doctest.h>
#include <ctime>
#include <cstring>

// Mock Arduino environment for native testing
#ifdef NATIVE_TEST
#include "../mock_arduino.h"
MockSerial Serial;
MockLittleFS LittleFS;
#else
#include <Arduino.h>
#endif

// Include calendar event class
#include "calendar_event.h"
#include "../../src/calendar_event.cpp"

// Test helper to create a mock calendar event
CalendarEvent* createMockEvent(const char* summary, const char* dtStart, const char* dtEnd, bool allDay = false) {
    CalendarEvent* event = new CalendarEvent();
    event->summary = summary;
    event->dtStart = dtStart;
    event->dtEnd = dtEnd;
    event->allDay = allDay;
    event->isRecurring = false;
    event->isToday = false;
    event->isTomorrow = false;
    event->calendarName = "Test Calendar";
    event->calendarColor = "blue";
    return event;
}

// Test helper to check time parsing
time_t parseICSDateTime(const String& dtString) {
    // Simple ICS date-time parser for testing
    // Format: YYYYMMDDTHHMMSS or YYYYMMDD
    if (dtString.length() < 8) return 0;

    struct tm timeinfo = {0};

    // Parse YYYYMMDD
    timeinfo.tm_year = dtString.substring(0, 4).toInt() - 1900;
    timeinfo.tm_mon = dtString.substring(4, 6).toInt() - 1;
    timeinfo.tm_mday = dtString.substring(6, 8).toInt();

    // Parse THHMMSS if present
    if (dtString.length() >= 15 && dtString.charAt(8) == 'T') {
        timeinfo.tm_hour = dtString.substring(9, 11).toInt();
        timeinfo.tm_min = dtString.substring(11, 13).toInt();
        timeinfo.tm_sec = dtString.substring(13, 15).toInt();
    } else {
        timeinfo.tm_hour = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
    }

    timeinfo.tm_isdst = -1;
    return mktime(&timeinfo);
}

// Basic ICS parsing tests without full parser integration
TEST_SUITE("CalendarStreamParser - Basic Parsing") {

    TEST_CASE("DateTime Parsing - Basic Format") {
        MESSAGE("Testing ICS date-time parsing");

        // Test date only
        time_t date1 = parseICSDateTime("20251029");
        struct tm* tm1 = localtime(&date1);
        CHECK(tm1->tm_year + 1900 == 2025);
        CHECK(tm1->tm_mon + 1 == 10);
        CHECK(tm1->tm_mday == 29);

        // Test date with time
        time_t date2 = parseICSDateTime("20251029T143000");
        struct tm* tm2 = localtime(&date2);
        CHECK(tm2->tm_year + 1900 == 2025);
        CHECK(tm2->tm_mon + 1 == 10);
        CHECK(tm2->tm_mday == 29);
        CHECK(tm2->tm_hour == 14);
        CHECK(tm2->tm_min == 30);
        CHECK(tm2->tm_sec == 0);
    }

    TEST_CASE("DateTime Parsing - Edge Cases") {
        // Start of year
        time_t date1 = parseICSDateTime("20250101T000000");
        struct tm* tm1 = localtime(&date1);
        CHECK(tm1->tm_year + 1900 == 2025);
        CHECK(tm1->tm_mon + 1 == 1);
        CHECK(tm1->tm_mday == 1);

        // End of year
        time_t date2 = parseICSDateTime("20251231T235959");
        struct tm* tm2 = localtime(&date2);
        CHECK(tm2->tm_year + 1900 == 2025);
        CHECK(tm2->tm_mon + 1 == 12);
        CHECK(tm2->tm_mday == 31);
        CHECK(tm2->tm_hour == 23);
        CHECK(tm2->tm_min == 59);
        CHECK(tm2->tm_sec == 59);
    }

    TEST_CASE("CalendarEvent - Basic Properties") {
        MESSAGE("Testing CalendarEvent structure");

        CalendarEvent* event = createMockEvent(
            "Team Meeting",
            "20251029T140000",
            "20251029T150000",
            false
        );

        CHECK(event->summary == "Team Meeting");
        CHECK(event->dtStart == "20251029T140000");
        CHECK(event->dtEnd == "20251029T150000");
        CHECK(event->allDay == false);
        CHECK(event->isRecurring == false);
        CHECK(event->calendarName == "Test Calendar");

        delete event;
    }

    TEST_CASE("CalendarEvent - All Day Event") {
        CalendarEvent* event = createMockEvent(
            "Birthday",
            "20251029",
            "20251030",
            true
        );

        CHECK(event->summary == "Birthday");
        CHECK(event->allDay == true);
        CHECK(event->dtStart == "20251029");

        delete event;
    }

    TEST_CASE("CalendarEvent - Today/Tomorrow Flags") {
        CalendarEvent* event = createMockEvent(
            "Today's Event",
            "20251029T100000",
            "20251029T110000",
            false
        );

        event->isToday = true;
        event->isTomorrow = false;

        CHECK(event->isToday == true);
        CHECK(event->isTomorrow == false);

        delete event;
    }

    TEST_CASE("CalendarEvent - Multiple Events") {
        std::vector<CalendarEvent*> events;

        events.push_back(createMockEvent("Event 1", "20251029T090000", "20251029T100000"));
        events.push_back(createMockEvent("Event 2", "20251029T110000", "20251029T120000"));
        events.push_back(createMockEvent("Event 3", "20251029T140000", "20251029T150000"));

        CHECK(events.size() == 3);
        CHECK(events[0]->summary == "Event 1");
        CHECK(events[1]->summary == "Event 2");
        CHECK(events[2]->summary == "Event 3");

        // Clean up
        for (auto event : events) {
            delete event;
        }
    }
}

TEST_SUITE("CalendarStreamParser - ICS Content Validation") {

    TEST_CASE("ICS Structure - Basic Validation") {
        MESSAGE("Testing basic ICS file structure validation");

        const char* validICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
END:VCALENDAR)";

        // Check for required keywords
        String icsContent(validICS);
        CHECK(icsContent.indexOf("BEGIN:VCALENDAR") != -1);
        CHECK(icsContent.indexOf("VERSION:2.0") != -1);
        CHECK(icsContent.indexOf("PRODID:") != -1);
        CHECK(icsContent.indexOf("END:VCALENDAR") != -1);
    }

    TEST_CASE("ICS Structure - With Event") {
        const char* icsWithEvent = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
BEGIN:VEVENT
SUMMARY:Test Event
DTSTART:20251029T100000
DTEND:20251029T110000
END:VEVENT
END:VCALENDAR)";

        String icsContent(icsWithEvent);
        CHECK(icsContent.indexOf("BEGIN:VEVENT") != -1);
        CHECK(icsContent.indexOf("SUMMARY:") != -1);
        CHECK(icsContent.indexOf("DTSTART:") != -1);
        CHECK(icsContent.indexOf("DTEND:") != -1);
        CHECK(icsContent.indexOf("END:VEVENT") != -1);
    }

    TEST_CASE("ICS Properties - Extract Calendar Name") {
        const char* icsWithName = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
X-WR-CALNAME:My Test Calendar
END:VCALENDAR)";

        String icsContent(icsWithName);
        int namePos = icsContent.indexOf("X-WR-CALNAME:");
        CHECK(namePos != -1);

        if (namePos != -1) {
            int lineEnd = icsContent.indexOf('\n', namePos);
            String nameLine = icsContent.substring(namePos, lineEnd);
            CHECK(nameLine.indexOf("My Test Calendar") != -1);
        }
    }

    TEST_CASE("ICS Properties - Extract Timezone") {
        const char* icsWithTZ = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
X-WR-TIMEZONE:Europe/Rome
END:VCALENDAR)";

        String icsContent(icsWithTZ);
        int tzPos = icsContent.indexOf("X-WR-TIMEZONE:");
        CHECK(tzPos != -1);

        if (tzPos != -1) {
            int lineEnd = icsContent.indexOf('\n', tzPos);
            String tzLine = icsContent.substring(tzPos, lineEnd);
            CHECK(tzLine.indexOf("Europe/Rome") != -1);
        }
    }

    TEST_CASE("ICS Line Folding - Unfold Long Lines") {
        // RFC 5545: Lines are folded at 75 octets, continuation starts with space
        const char* foldedICS = R"(X-WR-CALDESC:This is a very long description that needs to be folded beca
 use it exceeds the 75 character limit)";

        String content(foldedICS);

        // Unfold by removing CRLF + space or LF + space
        String unfolded = content;
        unfolded.replace("\r\n ", "");
        unfolded.replace("\n ", "");

        CHECK(unfolded.indexOf("because") != -1);
        CHECK(unfolded.indexOf("beca\n use") == -1); // Should be unfolded
    }
}

TEST_SUITE("CalendarStreamParser - Event Properties") {

    TEST_CASE("Event Properties - Recurring Event") {
        CalendarEvent* event = createMockEvent(
            "Weekly Meeting",
            "20251029T100000",
            "20251029T110000"
        );

        event->isRecurring = true;
        event->rrule = "FREQ=WEEKLY;BYDAY=TU";

        CHECK(event->isRecurring == true);
        CHECK(event->rrule == "FREQ=WEEKLY;BYDAY=TU");

        delete event;
    }

    TEST_CASE("Event Properties - With Location") {
        CalendarEvent* event = createMockEvent(
            "Conference",
            "20251029T090000",
            "20251029T170000"
        );

        event->location = "Conference Room A";

        CHECK(event->location == "Conference Room A");
        CHECK(event->summary == "Conference");

        delete event;
    }

    TEST_CASE("Event Properties - With Description") {
        CalendarEvent* event = createMockEvent(
            "Project Review",
            "20251029T140000",
            "20251029T150000"
        );

        event->description = "Quarterly project review with team";

        CHECK(event->description == "Quarterly project review with team");

        delete event;
    }

    TEST_CASE("Event Properties - Calendar Colors") {
        CalendarEvent* event1 = createMockEvent("Work Event", "20251029T100000", "20251029T110000");
        event1->calendarColor = "blue";

        CalendarEvent* event2 = createMockEvent("Personal Event", "20251029T140000", "20251029T150000");
        event2->calendarColor = "red";

        CHECK(event1->calendarColor == "blue");
        CHECK(event2->calendarColor == "red");

        delete event1;
        delete event2;
    }
}

TEST_SUITE("CalendarStreamParser - Date Range Filtering") {

    TEST_CASE("Date Range - Events Within Range") {
        // Create events for different days
        CalendarEvent* event1 = createMockEvent("Today", "20251029T100000", "20251029T110000");
        CalendarEvent* event2 = createMockEvent("Tomorrow", "20251030T100000", "20251030T110000");
        CalendarEvent* event3 = createMockEvent("Next Week", "20251105T100000", "20251105T110000");

        // Parse dates
        time_t date1 = parseICSDateTime(event1->dtStart);
        time_t date2 = parseICSDateTime(event2->dtStart);
        time_t date3 = parseICSDateTime(event3->dtStart);

        // Check that dates are in order
        CHECK(date1 < date2);
        CHECK(date2 < date3);
        CHECK(date1 < date3);

        delete event1;
        delete event2;
        delete event3;
    }

    TEST_CASE("Date Range - Filter By Date") {
        std::vector<CalendarEvent*> allEvents;
        allEvents.push_back(createMockEvent("Event 1", "20251029T100000", "20251029T110000"));
        allEvents.push_back(createMockEvent("Event 2", "20251030T100000", "20251030T110000"));
        allEvents.push_back(createMockEvent("Event 3", "20251105T100000", "20251105T110000"));

        // Filter to get only events on 2025-10-29
        time_t targetDate = parseICSDateTime("20251029");
        time_t nextDay = parseICSDateTime("20251030");

        std::vector<CalendarEvent*> filtered;
        for (auto event : allEvents) {
            time_t eventDate = parseICSDateTime(event->dtStart);
            if (eventDate >= targetDate && eventDate < nextDay) {
                filtered.push_back(event);
            }
        }

        CHECK(filtered.size() == 1);
        if (filtered.size() > 0) {
            CHECK(filtered[0]->summary == "Event 1");
        }

        // Clean up
        for (auto event : allEvents) {
            delete event;
        }
    }
}

TEST_SUITE("CalendarStreamParser - Memory Management") {

    TEST_CASE("Memory - Event Cleanup") {
        MESSAGE("Testing proper memory management of events");

        std::vector<CalendarEvent*> events;

        // Create multiple events
        for (int i = 0; i < 10; i++) {
            char summary[32];
            snprintf(summary, sizeof(summary), "Event %d", i);
            events.push_back(createMockEvent(summary, "20251029T100000", "20251029T110000"));
        }

        CHECK(events.size() == 10);

        // Clean up all events
        for (auto event : events) {
            delete event;
        }
        events.clear();

        CHECK(events.size() == 0);
    }

    TEST_CASE("Memory - Event Vector Management") {
        std::vector<CalendarEvent*> events;

        events.push_back(createMockEvent("Event 1", "20251029T100000", "20251029T110000"));
        events.push_back(createMockEvent("Event 2", "20251029T110000", "20251029T120000"));

        CHECK(events.size() == 2);

        // Remove first event
        delete events[0];
        events.erase(events.begin());

        CHECK(events.size() == 1);
        CHECK(events[0]->summary == "Event 2");

        // Clean up remaining
        delete events[0];
        events.clear();
    }
}
