#include <doctest.h>
#include "mock_arduino.h"
#include "calendar_stream_parser.h"

// Helper function to create a time_t from year/month/day/hour
time_t makeTime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0) {
    struct tm tm = {0};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;  // 0-based
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = 0;
    return mktime(&tm);
}

// Helper to format time_t as string for debugging
String formatTime(time_t t) {
    struct tm* tm = localtime(&t);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
    return String(buffer);
}

TEST_SUITE("findFirstOccurrence - Basic Validation") {
    CalendarStreamParser parser;

    TEST_CASE("Invalid parameters - negative eventStart") {
        time_t eventStart = -1;
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }

    TEST_CASE("Invalid parameters - negative startDate") {
        time_t eventStart = makeTime(2020, 1, 1);
        time_t startDate = -1;
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }

    TEST_CASE("Invalid parameters - negative endDate") {
        time_t eventStart = makeTime(2020, 1, 1);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = -1;
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }

    TEST_CASE("Invalid parameters - startDate > endDate") {
        time_t eventStart = makeTime(2020, 1, 1);
        time_t startDate = makeTime(2025, 12, 31);
        time_t endDate = makeTime(2025, 1, 1);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }

    TEST_CASE("Invalid parameters - interval < 1") {
        time_t eventStart = makeTime(2020, 1, 1);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 0;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }

    TEST_CASE("Invalid parameters - negative interval") {
        time_t eventStart = makeTime(2020, 1, 1);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = -1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }
}

TEST_SUITE("findFirstOccurrence - Event Position Relative to Query Range") {
    CalendarStreamParser parser;

    TEST_CASE("Event starts after query range ends - should return -1") {
        time_t eventStart = makeTime(2026, 1, 1);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == -1);
    }

    TEST_CASE("Event starts exactly at query start - should return eventStart") {
        time_t eventStart = makeTime(2025, 1, 1);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == eventStart);
    }

    TEST_CASE("Event starts within query range - should return eventStart") {
        time_t eventStart = makeTime(2025, 6, 15, 14, 30);  // Mid-year with specific time
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == eventStart);

        // Verify time is preserved
        CHECK(formatTime(result) == formatTime(eventStart));
    }

    TEST_CASE("Event starts exactly at query end - should return eventStart") {
        time_t eventStart = makeTime(2025, 12, 31, 23, 59, 59);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31, 23, 59, 59);
        int interval = 1;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result == eventStart);
    }

    TEST_CASE("Event starts before query range - DAILY recurrence") {
        // Event starts on Dec 31, 2024 at 10:00
        time_t eventStart = makeTime(2024, 12, 31, 10, 0);
        time_t startDate = makeTime(2025, 1, 1);  // Query starts Jan 1
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;

        // For DAILY recurrence, should find first occurrence on/after Jan 1
        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY);
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should be Jan 1 at 10:00 (preserving time from eventStart)
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_year + 1900 == 2025);
        CHECK(tm->tm_mon == 0);  // January
        CHECK(tm->tm_mday == 1);
        CHECK(tm->tm_hour == 10);
    }
}

TEST_SUITE("findFirstOccurrence - No COUNT Limit") {
    CalendarStreamParser parser;

    TEST_CASE("DAILY - no COUNT - event before range") {
        time_t eventStart = makeTime(2020, 1, 1, 10, 0);
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = -1;  // No COUNT limit

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY, count);

        // Should find first occurrence on or after startDate
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should be 2025-01-01 at 10:00 (preserving time from eventStart)
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_year + 1900 == 2025);
        CHECK(tm->tm_mon == 0);  // January
        CHECK(tm->tm_mday == 1);
        CHECK(tm->tm_hour == 10);
    }

    TEST_CASE("WEEKLY - no COUNT - event before range") {
        time_t eventStart = makeTime(2020, 1, 1, 14, 30);  // Wednesday
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = -1;  // No COUNT limit

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::WEEKLY, count);

        // Should find first occurrence on or after startDate
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should maintain same day of week and time
        struct tm* eventTm = localtime(&eventStart);
        struct tm* resultTm = localtime(&result);
        CHECK(resultTm->tm_wday == eventTm->tm_wday);  // Same day of week
        CHECK(resultTm->tm_hour == 14);
        CHECK(resultTm->tm_min == 30);
    }

    TEST_CASE("MONTHLY - no COUNT - event before range") {
        time_t eventStart = makeTime(2020, 1, 15, 9, 0);  // 15th of month
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = -1;  // No COUNT limit

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::MONTHLY, count);

        // Should find first occurrence on or after startDate
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should be 2025-01-15 at 09:00
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_year + 1900 == 2025);
        CHECK(tm->tm_mon == 0);  // January
        CHECK(tm->tm_mday == 15);
        CHECK(tm->tm_hour == 9);
    }

    TEST_CASE("YEARLY - no COUNT - event before range") {
        time_t eventStart = makeTime(2020, 3, 15, 12, 0);  // March 15
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = -1;  // No COUNT limit

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::YEARLY, count);

        // Should find first occurrence on or after startDate
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should be 2025-03-15 at 12:00
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_year + 1900 == 2025);
        CHECK(tm->tm_mon == 2);  // March
        CHECK(tm->tm_mday == 15);
        CHECK(tm->tm_hour == 12);
    }
}

TEST_SUITE("findFirstOccurrence - With COUNT Limit - Recurrence Still Active") {
    CalendarStreamParser parser;

    TEST_CASE("DAILY - COUNT=100 - event before range, still active") {
        time_t eventStart = makeTime(2025, 1, 1, 10, 0);
        time_t startDate = makeTime(2025, 2, 1);  // Query starts 31 days later
        time_t endDate = makeTime(2025, 3, 1);
        int interval = 1;
        int count = 100;  // 100 daily occurrences = will last 100 days

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY, count);

        // Should find first occurrence on or after startDate
        // Event lasts 100 days (through 2025-04-10), so query range (Feb 1 - Mar 1) is valid
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Result should be Feb 1 at 10:00 (preserving event time)
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_year + 1900 == 2025);
        CHECK(tm->tm_mon == 1);  // February
        CHECK(tm->tm_mday == 1);
        CHECK(tm->tm_hour == 10);
    }

    TEST_CASE("WEEKLY - COUNT=10 - event before range, still active") {
        time_t eventStart = makeTime(2025, 1, 1, 14, 0);  // Week 1
        time_t startDate = makeTime(2025, 1, 29);  // Week 5 (within 10 weeks)
        time_t endDate = makeTime(2025, 3, 31);
        int interval = 1;
        int count = 10;  // 10 weekly occurrences = 10 weeks

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::WEEKLY, count);

        // Should find occurrence (recurrence lasts 10 weeks from Jan 1)
        CHECK(result >= startDate);
        CHECK(result <= endDate);
    }

    TEST_CASE("MONTHLY - COUNT=12 - event before range, still active") {
        time_t eventStart = makeTime(2025, 1, 15, 9, 0);
        time_t startDate = makeTime(2025, 6, 1);  // 6 months later (within 12 months)
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 12;  // 12 monthly occurrences

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::MONTHLY, count);

        // Should find occurrence (recurrence lasts 12 months from Jan)
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should be 2025-06-15
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_mday == 15);
    }

    TEST_CASE("YEARLY - COUNT=5 - event before range, still active") {
        time_t eventStart = makeTime(2023, 3, 15, 12, 0);
        time_t startDate = makeTime(2025, 1, 1);  // 2 years later (within 5 years)
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 5;  // 5 yearly occurrences (2023, 2024, 2025, 2026, 2027)

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::YEARLY, count);

        // Should find occurrence (recurrence still active in 2025)
        CHECK(result >= startDate);
        CHECK(result <= endDate);

        // Should be 2025-03-15
        struct tm* tm = localtime(&result);
        CHECK(tm->tm_year + 1900 == 2025);
        CHECK(tm->tm_mon == 2);  // March
        CHECK(tm->tm_mday == 15);
    }
}

TEST_SUITE("findFirstOccurrence - With COUNT Limit - Recurrence Completed") {
    CalendarStreamParser parser;

    TEST_CASE("DAILY - COUNT=10 - recurrence completed before query range") {
        time_t eventStart = makeTime(2025, 1, 1, 10, 0);
        time_t startDate = makeTime(2025, 2, 1);  // Query starts after 10 days have passed
        time_t endDate = makeTime(2025, 3, 1);
        int interval = 1;
        int count = 10;  // Only 10 daily occurrences (Jan 1-10)

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::DAILY, count);

        // Should return -1 (recurrence completed on Jan 10, query starts Feb 1)
        CHECK(result == -1);
    }

    TEST_CASE("WEEKLY - COUNT=4 - recurrence completed before query range") {
        time_t eventStart = makeTime(2025, 1, 1, 14, 0);
        time_t startDate = makeTime(2025, 3, 1);  // Query starts after 4 weeks
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 4;  // Only 4 weekly occurrences (weeks of Jan 1, 8, 15, 22)

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::WEEKLY, count);

        // Should return -1 (recurrence completed by end of January)
        CHECK(result == -1);
    }

    TEST_CASE("MONTHLY - COUNT=6 - recurrence completed before query range") {
        time_t eventStart = makeTime(2025, 1, 15, 9, 0);
        time_t startDate = makeTime(2025, 8, 1);  // Query starts after 6 months
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 6;  // Only 6 monthly occurrences (Jan-Jun)

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::MONTHLY, count);

        // Should return -1 (recurrence completed in June)
        CHECK(result == -1);
    }

    TEST_CASE("YEARLY - COUNT=5 - recurrence completed before query range") {
        time_t eventStart = makeTime(2020, 3, 15, 12, 0);
        time_t startDate = makeTime(2025, 1, 1);  // Query starts after 5 years
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 5;  // Only 5 yearly occurrences (2020-2024)

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::YEARLY, count);

        // Should return -1 (recurrence completed in 2024)
        CHECK(result == -1);
    }

    TEST_CASE("WEEKLY - COUNT=6 - user's original example from 2015") {
        // This is the exact scenario the user reported
        time_t eventStart = makeTime(2015, 5, 27, 19, 0);  // May 27, 2015, Wednesday
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 6;  // Only 6 weekly occurrences (May 27, Jun 3, 10, 17, 24, Jul 1)

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::WEEKLY, count);

        // Should return -1 (recurrence completed in July 2015, query is for 2025)
        CHECK(result == -1);
    }

    TEST_CASE("WEEKLY - COUNT=6 - Real calendar event from user") {
        // Real event from user's calendar:
        // DTSTART;TZID=America/New_York:20150527T190000
        // RRULE:FREQ=WEEKLY;COUNT=6;BYDAY=WE
        //
        // This event started on Wednesday, May 27, 2015 at 19:00 (7 PM)
        // With COUNT=6 and FREQ=WEEKLY, it should have 6 occurrences:
        // 1. 2015-05-27 (Wed)
        // 2. 2015-06-03 (Wed)
        // 3. 2015-06-10 (Wed)
        // 4. 2015-06-17 (Wed)
        // 5. 2015-06-24 (Wed)
        // 6. 2015-07-01 (Wed)
        // Last occurrence: July 1, 2015
        //
        // Query range: 2025 (10 years later)
        // Expected result: -1 (recurrence completed in 2015)

        time_t eventStart = makeTime(2015, 5, 27, 19, 0);  // May 27, 2015, 7 PM
        time_t startDate = makeTime(2025, 1, 1);
        time_t endDate = makeTime(2025, 12, 31);
        int interval = 1;
        int count = 6;

        time_t result = parser.findFirstOccurrence(eventStart, startDate, endDate, interval, RecurrenceFrequency::WEEKLY, count);

        // Should return -1 because the recurrence completed 10 years ago
        CHECK(result == -1);

        // Verify the last occurrence date for documentation
        struct tm lastOccurrenceTm;
        memcpy(&lastOccurrenceTm, localtime(&eventStart), sizeof(struct tm));
        lastOccurrenceTm.tm_mday += (count - 1) * interval * 7;  // Add 5 weeks
        time_t lastOccurrence = mktime(&lastOccurrenceTm);

        struct tm* lastTm = localtime(&lastOccurrence);
        // Last occurrence should be July 1, 2015
        CHECK(lastTm->tm_year + 1900 == 2015);
        CHECK(lastTm->tm_mon == 6);  // July (0-based)
        CHECK(lastTm->tm_mday == 1);
    }
}
