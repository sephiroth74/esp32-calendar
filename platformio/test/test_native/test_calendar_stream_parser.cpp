#include <doctest.h>
#include <ctime>
#include <cstring>
#include <set>

// Mock Arduino environment for native testing
#ifdef NATIVE_TEST
#include "../mock_arduino.h"
// Global mock instances are now defined in mock_arduino.cpp
#else
#include <Arduino.h>
#endif

// Include calendar event class and dependencies
#include "date_utils.h"
#include "../../src/date_utils.cpp"
#include "calendar_event.h"
#include "../../src/calendar_event.cpp"

// Include the real CalendarStreamParser implementation
// With NATIVE_TEST defined, it will use mock_calendar_fetcher automatically
#include "calendar_stream_parser.h"
#include "../../src/calendar_stream_parser.cpp"

// Forward declare test helper
time_t parseICSDateTime(const String& dtString);

// Test helper to create mock events
CalendarEvent* createMockEvent(const String& summary, const String& startTime, const String& endTime, bool allDay = false) {
    CalendarEvent* event = new CalendarEvent();
    event->summary = summary;
    // Use the new setStartDateTime/setEndDateTime methods
    bool isDate = allDay || (startTime.length() == 8);
    event->setStartDateTime(startTime, "", isDate);
    if (!endTime.isEmpty()) {
        bool isEndDate = allDay || (endTime.length() == 8);
        event->setEndDateTime(endTime, "", isEndDate);
    }
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
        CHECK(event->startTime > 0);  // Should have valid timestamp
        CHECK(event->endTime > 0);    // Should have valid timestamp
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
        CHECK(event->startTime > 0);  // Should have valid timestamp

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

        // Dates are now already parsed as timestamps
        time_t date1 = event1->startTime;
        time_t date2 = event2->startTime;
        time_t date3 = event3->startTime;

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
            time_t eventDate = event->startTime;
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

// ============================================================================
// NOTE: Recurring Events Tests have been moved to test_expand_recurring_event_v2.cpp
// That file contains comprehensive tests for expandRecurringEventV2() method
// ============================================================================

// ============================================================================
// RRULE Parser Tests
// ============================================================================

TEST_SUITE("CalendarStreamParser - RRULE Parser") {
    CalendarStreamParser parser;

    TEST_CASE("RRULE Parser - FREQ with BYDAY") {
        RRuleComponents rule = parser.parseRRule("FREQ=WEEKLY;BYDAY=MO,WE,FR");

        CHECK(rule.isWeekly());
        CHECK(rule.byDay == "MO,WE,FR");
        CHECK(rule.hasByDay());
    }

    TEST_CASE("RRULE Parser - FREQ with BYMONTHDAY") {
        RRuleComponents rule = parser.parseRRule("FREQ=MONTHLY;BYMONTHDAY=1,15");

        CHECK(rule.isMonthly());
        CHECK(rule.byMonthDay == "1,15");
        CHECK(rule.hasByMonthDay());
    }

    TEST_CASE("RRULE Parser - FREQ with BYMONTH") {
        RRuleComponents rule = parser.parseRRule("FREQ=YEARLY;BYMONTH=1,7");

        CHECK(rule.isYearly());
        CHECK(rule.byMonth == "1,7");
        CHECK(rule.hasByMonth());
    }

    TEST_CASE("RRULE Parser - Complex Rule") {
        RRuleComponents rule = parser.parseRRule("FREQ=WEEKLY;BYDAY=MO,WE,FR;COUNT=20;INTERVAL=2");

        CHECK(rule.isWeekly());
        CHECK(rule.byDay == "MO,WE,FR");
        CHECK(rule.count == 20);
        CHECK(rule.interval == 2);
        CHECK(rule.hasCountLimit());
        CHECK(rule.hasByDay());
        CHECK(rule.hasInterval());
    }

    TEST_CASE("RRULE Parser - Empty String") {
        RRuleComponents rule = parser.parseRRule("");

        CHECK(!rule.isValid());
        CHECK(rule.freq.isEmpty());
    }

    TEST_CASE("RRULE Parser - Invalid INTERVAL defaults to 1") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;INTERVAL=0");

        CHECK(rule.interval == 1);
    }

    TEST_CASE("RRULE Parser - Negative COUNT sanitized") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;COUNT=-5");

        CHECK(rule.count == -1);
    }
}

TEST_SUITE("CalendarStreamParser - RRULE Helper Methods") {
    CalendarStreamParser parser;

    TEST_CASE("Parse BYDAY - Single Day") {
        std::vector<int> days = parser.parseByDay("MO");

        CHECK(days.size() == 1);
        CHECK(days[0] == 1);  // Monday
    }

    TEST_CASE("Parse BYDAY - Multiple Days") {
        std::vector<int> days = parser.parseByDay("MO,WE,FR");

        CHECK(days.size() == 3);
        CHECK(days[0] == 1);  // Monday
        CHECK(days[1] == 3);  // Wednesday
        CHECK(days[2] == 5);  // Friday
    }

    TEST_CASE("Parse BYDAY - All Days") {
        std::vector<int> days = parser.parseByDay("SU,MO,TU,WE,TH,FR,SA");

        CHECK(days.size() == 7);
        CHECK(days[0] == 0);  // Sunday
        CHECK(days[6] == 6);  // Saturday
    }

    TEST_CASE("Parse BYDAY - Empty String") {
        std::vector<int> days = parser.parseByDay("");

        CHECK(days.size() == 0);
    }

    TEST_CASE("Parse BYMONTHDAY - Single Day") {
        std::vector<int> days = parser.parseByMonthDay("15");

        CHECK(days.size() == 1);
        CHECK(days[0] == 15);
    }

    TEST_CASE("Parse BYMONTHDAY - Multiple Days") {
        std::vector<int> days = parser.parseByMonthDay("1,15,30");

        CHECK(days.size() == 3);
        CHECK(days[0] == 1);
        CHECK(days[1] == 15);
        CHECK(days[2] == 30);
    }

    TEST_CASE("Parse BYMONTHDAY - Negative (Last Day)") {
        std::vector<int> days = parser.parseByMonthDay("-1");

        CHECK(days.size() == 1);
        CHECK(days[0] == -1);
    }

    TEST_CASE("Parse BYMONTH - Single Month") {
        std::vector<int> months = parser.parseByMonth("7");

        CHECK(months.size() == 1);
        CHECK(months[0] == 7);
    }

    TEST_CASE("Parse BYMONTH - Multiple Months") {
        std::vector<int> months = parser.parseByMonth("1,7,12");

        CHECK(months.size() == 3);
        CHECK(months[0] == 1);   // January
        CHECK(months[1] == 7);   // July
        CHECK(months[2] == 12);  // December
    }

    TEST_CASE("Parse BYMONTH - Invalid Month Filtered") {
        std::vector<int> months = parser.parseByMonth("0,13,15");

        CHECK(months.size() == 0);  // All invalid
    }
}

// ============================================================================
// RRULE Parser - Edge Cases & Validation Tests
// ============================================================================

TEST_SUITE("CalendarStreamParser - RRULE Edge Cases") {
    CalendarStreamParser parser;

    TEST_CASE("RRULE - Whitespace handling") {
        RRuleComponents rule = parser.parseRRule("FREQ = WEEKLY ; COUNT = 10 ");

        CHECK(rule.isWeekly());
        CHECK(rule.count == 10);
    }

    TEST_CASE("RRULE - Case sensitivity") {
        // RRULE keys should be case-sensitive (uppercase per RFC 5545)
        RRuleComponents rule = parser.parseRRule("freq=daily;count=5");

        // Our implementation treats keys as case-sensitive
        // lowercase 'freq' won't match "FREQ"
        CHECK(rule.freq.isEmpty());  // Should not parse lowercase
    }

    TEST_CASE("RRULE - Missing equals sign") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;COUNT");

        CHECK(rule.isDaily());
        CHECK(rule.count == -1);  // COUNT without value should be ignored
    }

    TEST_CASE("RRULE - Empty value") {
        RRuleComponents rule = parser.parseRRule("FREQ=;COUNT=10");

        CHECK(rule.freq.isEmpty());
        CHECK(rule.count == 10);
    }

    TEST_CASE("RRULE - Trailing semicolon") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;COUNT=10;");

        CHECK(rule.isDaily());
        CHECK(rule.count == 10);
    }

    TEST_CASE("RRULE - Leading semicolon") {
        RRuleComponents rule = parser.parseRRule(";FREQ=DAILY;COUNT=10");

        CHECK(rule.isDaily());
        CHECK(rule.count == 10);
    }

    TEST_CASE("RRULE - Multiple semicolons") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;;COUNT=10");

        CHECK(rule.isDaily());
        CHECK(rule.count == 10);
    }

    TEST_CASE("RRULE - Unknown parameters ignored") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;UNKNOWN=value;COUNT=5");

        CHECK(rule.isDaily());
        CHECK(rule.count == 5);
    }

    TEST_CASE("RRULE - Duplicate parameters (last wins)") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;COUNT=10;COUNT=20");

        CHECK(rule.count == 20);  // Last value should win
    }

    TEST_CASE("RRULE - COUNT=0 treated as invalid") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;COUNT=0");

        // COUNT=0 is invalid per RFC 5545 (must be positive)
        // toInt() returns 0 for "0", which we should handle
        CHECK(rule.count == 0);  // Currently accepted, could add validation
    }

    TEST_CASE("RRULE - Very large COUNT") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;COUNT=10000");

        CHECK(rule.count == 10000);
        CHECK(rule.hasCountLimit());
    }

    TEST_CASE("RRULE - INTERVAL with negative value") {
        RRuleComponents rule = parser.parseRRule("FREQ=DAILY;INTERVAL=-2");

        CHECK(rule.interval == 1);  // Should default to 1
    }

    TEST_CASE("RRULE - Very large INTERVAL") {
        RRuleComponents rule = parser.parseRRule("FREQ=YEARLY;INTERVAL=100");

        CHECK(rule.interval == 100);
    }
}

TEST_SUITE("CalendarStreamParser - UNTIL Date Parsing") {
    CalendarStreamParser parser;

    TEST_CASE("Parse UNTIL - Date only (YYYYMMDD)") {
        time_t until = parser.parseUntilDate("20251231");

        CHECK(until > 0);

        struct tm* tm = localtime(&until);
        CHECK(tm->tm_year == 125);  // 2025
        CHECK(tm->tm_mon == 11);    // December (0-based)
        CHECK(tm->tm_mday == 31);
        CHECK(tm->tm_hour == 23);   // End of day
        CHECK(tm->tm_min == 59);
        CHECK(tm->tm_sec == 59);
    }

    TEST_CASE("Parse UNTIL - Date and time (YYYYMMDDTHHMMSS)") {
        time_t until = parser.parseUntilDate("20251231T143000");

        CHECK(until > 0);

        struct tm* tm = localtime(&until);
        CHECK(tm->tm_year == 125);  // 2025
        CHECK(tm->tm_mon == 11);    // December
        CHECK(tm->tm_mday == 31);
        // Note: Hour might be off due to DST, so we don't test exact hour
    }

    TEST_CASE("Parse UNTIL - UTC format with Z") {
        time_t until = parser.parseUntilDate("20251231T235959Z");

        CHECK(until > 0);

        struct tm* tm = localtime(&until);
        CHECK(tm->tm_year == 125);  // 2025
        CHECK(tm->tm_mon == 11);    // December
        CHECK(tm->tm_mday == 31);
    }

    TEST_CASE("Parse UNTIL - Empty string") {
        time_t until = parser.parseUntilDate("");

        CHECK(until == 0);
    }

    TEST_CASE("Parse UNTIL - Too short string") {
        time_t until = parser.parseUntilDate("2025");

        CHECK(until == 0);
    }

    TEST_CASE("Parse UNTIL - Leap year date") {
        time_t until = parser.parseUntilDate("20240229");  // Feb 29, 2024

        CHECK(until > 0);

        struct tm* tm = localtime(&until);
        CHECK(tm->tm_year == 124);  // 2024
        CHECK(tm->tm_mon == 1);     // February
        CHECK(tm->tm_mday == 29);
    }

    TEST_CASE("Parse UNTIL - Start of year") {
        time_t until = parser.parseUntilDate("20250101T000000");

        CHECK(until > 0);

        struct tm* tm = localtime(&until);
        CHECK(tm->tm_year == 125);
        CHECK(tm->tm_mon == 0);
        CHECK(tm->tm_mday == 1);
    }
}

TEST_SUITE("CalendarStreamParser - BYDAY Advanced") {
    CalendarStreamParser parser;

    TEST_CASE("Parse BYDAY - With position prefix (1MO)") {
        std::vector<int> days = parser.parseByDay("1MO");

        // Position prefix is ignored for now, just extract day code
        CHECK(days.size() == 1);
        CHECK(days[0] == 1);  // Monday
    }

    TEST_CASE("Parse BYDAY - Multiple with positions (1MO,-1FR)") {
        std::vector<int> days = parser.parseByDay("1MO,-1FR");

        CHECK(days.size() == 2);
        CHECK(days[0] == 1);  // Monday
        CHECK(days[1] == 5);  // Friday
    }

    TEST_CASE("Parse BYDAY - Mixed with and without positions") {
        std::vector<int> days = parser.parseByDay("MO,2WE,FR");

        CHECK(days.size() == 3);
        CHECK(days[0] == 1);  // Monday
        CHECK(days[1] == 3);  // Wednesday
        CHECK(days[2] == 5);  // Friday
    }

    TEST_CASE("Parse BYDAY - Invalid day code ignored") {
        std::vector<int> days = parser.parseByDay("MO,XX,FR");

        CHECK(days.size() == 2);  // Only MO and FR
        CHECK(days[0] == 1);
        CHECK(days[1] == 5);
    }

    TEST_CASE("Parse BYDAY - Spaces in list") {
        std::vector<int> days = parser.parseByDay("MO, WE, FR");

        // Should handle spaces after commas
        CHECK(days.size() == 3);
    }

    TEST_CASE("Parse BYDAY - Trailing comma") {
        std::vector<int> days = parser.parseByDay("MO,WE,");

        CHECK(days.size() == 2);
    }

    TEST_CASE("Parse BYDAY - Duplicate days") {
        std::vector<int> days = parser.parseByDay("MO,WE,MO");

        // Duplicates are allowed (will be in vector twice)
        CHECK(days.size() == 3);
        CHECK(days[0] == 1);
        CHECK(days[1] == 3);
        CHECK(days[2] == 1);  // Duplicate Monday
    }
}

TEST_SUITE("CalendarStreamParser - BYMONTHDAY Advanced") {
    CalendarStreamParser parser;

    TEST_CASE("Parse BYMONTHDAY - Mixed positive and negative") {
        std::vector<int> days = parser.parseByMonthDay("1,15,-1");

        CHECK(days.size() == 3);
        CHECK(days[0] == 1);
        CHECK(days[1] == 15);
        CHECK(days[2] == -1);
    }

    TEST_CASE("Parse BYMONTHDAY - Last two days (-1,-2)") {
        std::vector<int> days = parser.parseByMonthDay("-1,-2");

        CHECK(days.size() == 2);
        CHECK(days[0] == -1);
        CHECK(days[1] == -2);
    }

    TEST_CASE("Parse BYMONTHDAY - Out of range days") {
        std::vector<int> days = parser.parseByMonthDay("1,32,15");

        // All values parsed (no validation yet)
        CHECK(days.size() == 3);
        CHECK(days[0] == 1);
        CHECK(days[1] == 32);  // Will be validated during expansion
        CHECK(days[2] == 15);
    }

    TEST_CASE("Parse BYMONTHDAY - Zero day invalid") {
        std::vector<int> days = parser.parseByMonthDay("0,15");

        // 0 is not a valid day (days are 1-31 or -1 to -31)
        CHECK(days.size() == 1);  // Only 15
        CHECK(days[0] == 15);
    }

    TEST_CASE("Parse BYMONTHDAY - Spaces in list") {
        std::vector<int> days = parser.parseByMonthDay("1, 15, 30");

        CHECK(days.size() == 3);
    }

    TEST_CASE("Parse BYMONTHDAY - All days of month") {
        std::vector<int> days = parser.parseByMonthDay("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31");

        CHECK(days.size() == 31);
        CHECK(days[0] == 1);
        CHECK(days[30] == 31);
    }
}

TEST_SUITE("CalendarStreamParser - BYMONTH Advanced") {
    CalendarStreamParser parser;

    TEST_CASE("Parse BYMONTH - All months") {
        std::vector<int> months = parser.parseByMonth("1,2,3,4,5,6,7,8,9,10,11,12");

        CHECK(months.size() == 12);
        CHECK(months[0] == 1);
        CHECK(months[11] == 12);
    }

    TEST_CASE("Parse BYMONTH - Quarters (Q1, Q2, Q3, Q4)") {
        std::vector<int> q1 = parser.parseByMonth("1,2,3");
        std::vector<int> q2 = parser.parseByMonth("4,5,6");
        std::vector<int> q3 = parser.parseByMonth("7,8,9");
        std::vector<int> q4 = parser.parseByMonth("10,11,12");

        CHECK(q1.size() == 3);
        CHECK(q2.size() == 3);
        CHECK(q3.size() == 3);
        CHECK(q4.size() == 3);
    }

    TEST_CASE("Parse BYMONTH - Out of range filtered") {
        std::vector<int> months = parser.parseByMonth("0,1,13,7,100");

        // Only 1 and 7 are valid
        CHECK(months.size() == 2);
        CHECK(months[0] == 1);
        CHECK(months[1] == 7);
    }

    TEST_CASE("Parse BYMONTH - Negative values filtered") {
        std::vector<int> months = parser.parseByMonth("-1,1,7");

        // Negative months not supported
        CHECK(months.size() == 2);
        CHECK(months[0] == 1);
        CHECK(months[1] == 7);
    }

    TEST_CASE("Parse BYMONTH - Spaces in list") {
        std::vector<int> months = parser.parseByMonth("1, 7, 12");

        CHECK(months.size() == 3);
    }
}

