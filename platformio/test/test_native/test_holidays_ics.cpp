#include <doctest.h>
#include <fstream>
#include <sstream>
#include <ctime>

// Mock Arduino environment for native testing
#ifdef NATIVE_TEST
#include "../mock_arduino.h"
// Don't declare globals here - they're already declared in test_calendar_stream_parser.cpp
extern MockSerial Serial;
extern MockLittleFS LittleFS;
#else
#include <Arduino.h>
#endif

// Include only headers - implementations are in test_calendar_stream_parser.cpp
#include "calendar_event.h"
#include "calendar_stream_parser.h"

// Helper function to load a local file
String loadLocalFile(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Serial.print("Failed to open file: ");
        Serial.println(filepath);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return String(buffer.str().c_str());
}

TEST_SUITE("Holidays ICS File - Basic Validation") {

    TEST_CASE("Load holidays.ics file") {
        MESSAGE("Loading holidays.ics file from disk");

        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        CHECK(content.length() > 0);
        MESSAGE("File loaded, size: ", content.length(), " bytes");
    }

    TEST_CASE("Validate ICS structure") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Check for required VCALENDAR structure
        CHECK(content.indexOf("BEGIN:VCALENDAR") != -1);
        CHECK(content.indexOf("END:VCALENDAR") != -1);
        CHECK(content.indexOf("VERSION:2.0") != -1);
        CHECK(content.indexOf("PRODID:") != -1);
    }

    TEST_CASE("Validate calendar metadata") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Check for calendar name (X-WR-CALNAME)
        int namePos = content.indexOf("X-WR-CALNAME:");
        CHECK(namePos != -1);

        if (namePos != -1) {
            int lineEnd = content.indexOf('\n', namePos);
            String nameLine = content.substring(namePos, lineEnd);
            MESSAGE("Calendar name: ", nameLine.c_str());

            // Should contain "Festività in Svizzera"
            CHECK(nameLine.indexOf("Festività") != -1);
        }

        // Check for timezone (X-WR-TIMEZONE)
        int tzPos = content.indexOf("X-WR-TIMEZONE:");
        CHECK(tzPos != -1);

        if (tzPos != -1) {
            int lineEnd = content.indexOf('\n', tzPos);
            String tzLine = content.substring(tzPos, lineEnd);
            MESSAGE("Timezone: ", tzLine.c_str());
        }
    }

    TEST_CASE("Count events in holidays.ics") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Count BEGIN:VEVENT occurrences
        int eventCount = 0;
        int pos = 0;
        while ((pos = content.indexOf("BEGIN:VEVENT", pos)) != -1) {
            eventCount++;
            pos += 12; // length of "BEGIN:VEVENT"
        }

        MESSAGE("Total events found: ", eventCount);
        CHECK(eventCount > 0);

        // The file should have many events (it's a multi-year holiday calendar)
        CHECK(eventCount > 100);
    }
}

TEST_SUITE("Holidays ICS File - Event Properties") {

    TEST_CASE("Verify event structure") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Find first event
        int eventStart = content.indexOf("BEGIN:VEVENT");
        REQUIRE(eventStart != -1);

        int eventEnd = content.indexOf("END:VEVENT", eventStart);
        REQUIRE(eventEnd != -1);

        String firstEvent = content.substring(eventStart, eventEnd + 10);

        // Check for required properties
        CHECK(firstEvent.indexOf("DTSTART") != -1);
        CHECK(firstEvent.indexOf("DTEND") != -1);
        CHECK(firstEvent.indexOf("SUMMARY:") != -1);
        CHECK(firstEvent.indexOf("UID:") != -1);
    }

    TEST_CASE("Check for all-day events") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Count DTSTART;VALUE=DATE occurrences (all-day events)
        int allDayCount = 0;
        int pos = 0;
        while ((pos = content.indexOf("DTSTART;VALUE=DATE:", pos)) != -1) {
            allDayCount++;
            pos += 19;
        }

        MESSAGE("All-day events found: ", allDayCount);
        CHECK(allDayCount > 0);
    }

    TEST_CASE("Check for event summaries") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Look for specific holidays
        CHECK(content.indexOf("SUMMARY:Epifania") != -1);
        CHECK(content.indexOf("SUMMARY:Capodanno") != -1);
        CHECK(content.indexOf("SUMMARY:Giorno di Natale") != -1);

        MESSAGE("Holiday event summaries validated");
    }

    TEST_CASE("Check date ranges") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Should have events from multiple years (2020-2030)
        CHECK(content.indexOf("DTSTART;VALUE=DATE:2020") != -1);
        CHECK(content.indexOf("DTSTART;VALUE=DATE:2025") != -1);
        CHECK(content.indexOf("DTSTART;VALUE=DATE:2030") != -1);

        MESSAGE("Multi-year date range validated");
    }
}

TEST_SUITE("Holidays ICS File - Recurring Events") {

    TEST_CASE("Check for recurring events") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Note: This ICS file appears to use explicit instances rather than RRULE
        // Check if there are any RRULE entries
        int rruleCount = 0;
        int pos = 0;
        while ((pos = content.indexOf("RRULE:", pos)) != -1) {
            rruleCount++;
            pos += 6;
        }

        MESSAGE("RRULE entries found: ", rruleCount);

        // This calendar may use explicit event instances instead of recurrence rules
        if (rruleCount == 0) {
            MESSAGE("Note: This calendar uses explicit event instances rather than RRULE");
        }
    }

    TEST_CASE("Check for duplicate event summaries") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Count occurrences of "Capodanno" (New Year's Day) - should appear multiple years
        int newYearCount = 0;
        int pos = 0;
        while ((pos = content.indexOf("SUMMARY:Capodanno", pos)) != -1) {
            newYearCount++;
            pos += 17;
        }

        MESSAGE("'Capodanno' events found: ", newYearCount);
        CHECK(newYearCount > 1); // Should have multiple years

        // Count occurrences of "Natale" (Christmas)
        int christmasCount = 0;
        pos = 0;
        while ((pos = content.indexOf("SUMMARY:Giorno di Natale", pos)) != -1) {
            christmasCount++;
            pos += 24;
        }

        MESSAGE("'Giorno di Natale' events found: ", christmasCount);
        CHECK(christmasCount > 1); // Should have multiple years
    }
}

TEST_SUITE("Holidays ICS File - Special Properties") {

    TEST_CASE("Check for event descriptions") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Count DESCRIPTION properties
        int descCount = 0;
        int pos = 0;
        while ((pos = content.indexOf("DESCRIPTION:", pos)) != -1) {
            descCount++;
            pos += 12;
        }

        MESSAGE("Events with descriptions: ", descCount);
        CHECK(descCount > 0);
    }

    TEST_CASE("Check for regional holidays") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Look for regional indicators in descriptions
        CHECK(content.indexOf("festività regionale") != -1);
        CHECK(content.indexOf("Festa nazionale") != -1);

        MESSAGE("Regional holiday indicators validated");
    }

    TEST_CASE("Check for transparency settings") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Check for TRANSP property (should be TRANSPARENT for holidays)
        int transparentCount = 0;
        int pos = 0;
        while ((pos = content.indexOf("TRANSP:TRANSPARENT", pos)) != -1) {
            transparentCount++;
            pos += 18;
        }

        MESSAGE("Transparent events: ", transparentCount);
        CHECK(transparentCount > 0);
    }

    TEST_CASE("Check for line folding") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Check for line continuation (lines starting with space)
        // RFC 5545: Lines are folded by inserting CRLF + space/tab
        int foldedLines = 0;
        int pos = 0;
        while ((pos = content.indexOf("\n ", pos)) != -1) {
            foldedLines++;
            pos += 2;
        }

        MESSAGE("Folded lines found: ", foldedLines);

        // The holidays.ics file has some long descriptions that are folded
        CHECK(foldedLines > 0);
    }
}

// ============================================================================
// Parser Integration Tests
// ============================================================================

TEST_SUITE("Holidays ICS File - Parser Integration") {

    CalendarStreamParser parser;

    TEST_CASE("Parse holidays.ics and extract all events") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Count and parse all events
        std::vector<CalendarEvent*> allEvents;
        int pos = 0;
        int eventStart = -1;

        while ((eventStart = content.indexOf("BEGIN:VEVENT", pos)) != -1) {
            int eventEnd = content.indexOf("END:VEVENT", eventStart);
            if (eventEnd == -1) break;

            String eventBlock = content.substring(eventStart, eventEnd + 10);
            CalendarEvent* event = parser.parseEventFromBuffer(eventBlock);

            if (event != nullptr && event->isValid()) {
                allEvents.push_back(event);
            }

            pos = eventEnd + 10;
        }

        MESSAGE("Total valid events parsed: ", allEvents.size());
        CHECK(allEvents.size() > 100);

        // Clean up
        for (auto event : allEvents) {
            delete event;
        }
    }

    TEST_CASE("Parse and validate specific holiday: Ognissanti") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Find Ognissanti event for 2025
        int summaryPos = content.indexOf("SUMMARY:Ognissanti");
        REQUIRE(summaryPos != -1);

        // Search backwards from summaryPos to find BEGIN:VEVENT
        int blockStart = -1;
        for (int i = summaryPos; i >= 0; i--) {
            if (content.substring(i, i + 12) == "BEGIN:VEVENT") {
                blockStart = i;
                break;
            }
        }
        REQUIRE(blockStart != -1);

        int blockEnd = content.indexOf("END:VEVENT", summaryPos);
        REQUIRE(blockEnd != -1);

        String eventBlock = content.substring(blockStart, blockEnd + 10);
        CalendarEvent* event = parser.parseEventFromBuffer(eventBlock);

        REQUIRE(event != nullptr);
        CHECK(event->isValid());
        CHECK(event->summary.indexOf("Ognissanti") != -1);
        CHECK(event->allDay == true);

        MESSAGE("Ognissanti event parsed successfully");
        MESSAGE("Summary: ", event->summary.c_str());
        MESSAGE("Start: ", event->getStartDate().c_str(), " ", event->getStartTimeStr().c_str());

        delete event;
    }

    TEST_CASE("Expand recurring events - Nov 1, 2025 to Jan 4, 2026") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Define date range: Nov 1, 2025 00:00:00 to Jan 4, 2026 23:59:59
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 10;  // November (0-based)
        startTm.tm_mday = 1;
        startTm.tm_hour = 0;
        startTm.tm_min = 0;
        startTm.tm_sec = 0;
        startTm.tm_isdst = -1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2026 - 1900;
        endTm.tm_mon = 0;  // January (0-based)
        endTm.tm_mday = 4;
        endTm.tm_hour = 23;
        endTm.tm_min = 59;
        endTm.tm_sec = 59;
        endTm.tm_isdst = -1;
        time_t endDate = mktime(&endTm);

        MESSAGE("Date range: Nov 1, 2025 to Jan 4, 2026");
        MESSAGE("Start timestamp: ", (long)startDate);
        MESSAGE("End timestamp: ", (long)endDate);

        // Create a StringStream from the file content
        StringStream stream(content);

        // Parse using streamParseFromStream
        std::vector<CalendarEvent*> eventsInRange;
        int totalParsed = 0;

        bool success = parser.streamParseFromStream(&stream,
            [&](CalendarEvent* event) {
                totalParsed++;
                // Event is already filtered by date range and expanded if recurring
                eventsInRange.push_back(event);
            },
            startDate, endDate);

        CHECK(success);
        MESSAGE("Total events parsed: ", totalParsed);

        MESSAGE("Events in range: ", eventsInRange.size());

        // Sort events by start time
        std::sort(eventsInRange.begin(), eventsInRange.end(),
            [](CalendarEvent* a, CalendarEvent* b) {
                return a->startTime < b->startTime;
            });

        // Print all events for debugging
        MESSAGE("Events in range (Nov 1, 2025 - Jan 4, 2026):");
        for (auto event : eventsInRange) {
            struct tm* tm = localtime(&event->startTime);
            char dateStr[64];
            snprintf(dateStr, sizeof(dateStr), "  %04d-%02d-%02d: %s",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, event->summary.c_str());
            MESSAGE(dateStr);
        }

        // Validate expected events are present
        bool foundOgnissanti = false;
        bool foundImmacolata = false;
        bool foundVigiliaNatale = false;
        bool foundNatale = false;
        bool foundSantoStefano = false;
        bool foundVigiliaCapodanno = false;
        bool foundCapodanno = false;
        bool foundSanBasilio = false;

        for (auto event : eventsInRange) {
            if (event->summary.indexOf("Ognissanti") != -1) {
                foundOgnissanti = true;
                MESSAGE("✓ Found: Ognissanti");
            }
            if (event->summary.indexOf("Immacolata Concezione") != -1) {
                foundImmacolata = true;
                MESSAGE("✓ Found: Immacolata Concezione");
            }
            if (event->summary.indexOf("Vigilia di Natale") != -1) {
                foundVigiliaNatale = true;
                MESSAGE("✓ Found: Vigilia di Natale");
            }
            if (event->summary.indexOf("Giorno di Natale") != -1) {
                foundNatale = true;
                MESSAGE("✓ Found: Giorno di Natale");
            }
            if (event->summary.indexOf("Giorno di Santo Stefano") != -1) {
                foundSantoStefano = true;
                MESSAGE("✓ Found: Giorno di Santo Stefano");
            }
            if (event->summary.indexOf("Vigilia di Capodanno") != -1) {
                foundVigiliaCapodanno = true;
                MESSAGE("✓ Found: Vigilia di Capodanno");
            }
            if (event->summary.indexOf("Capodanno") != -1 && event->summary.indexOf("Vigilia") == -1) {
                foundCapodanno = true;
                MESSAGE("✓ Found: Capodanno");
            }
            if (event->summary.indexOf("San Basilio") != -1) {
                foundSanBasilio = true;
                MESSAGE("✓ Found: San Basilio");
            }
        }

        // Check all expected events are found
        CHECK(foundOgnissanti);
        CHECK(foundImmacolata);
        CHECK(foundVigiliaNatale);
        CHECK(foundNatale);
        CHECK(foundSantoStefano);
        CHECK(foundVigiliaCapodanno);
        CHECK(foundCapodanno);
        CHECK(foundSanBasilio);

        // Clean up
        for (auto event : eventsInRange) {
            delete event;
        }
    }

    TEST_CASE("Validate specific dates for expected holidays") {
        const char* filepath = "test/fixtures/holidays.ics";
        String content = loadLocalFile(filepath);

        REQUIRE(content.length() > 0);

        // Define date range
        struct tm startTm = {0};
        POPULATE_TM_DATE_TIME(startTm, 2025, 11, 1, 0, 0, 0, -1); // Nov 1, 2025
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        POPULATE_TM_DATE_TIME(endTm, 2026, 1, 4, 23, 59, 59, -1); // Jan 4, 2026
        time_t endDate = mktime(&endTm);

        // Create a StringStream and parse
        StringStream stream(content);
        std::vector<CalendarEvent*> eventsInRange;

        bool success = parser.streamParseFromStream(&stream,
            [&](CalendarEvent* event) {
                eventsInRange.push_back(event);
            },
            startDate, endDate);

        CHECK(success);

        MESSAGE("Total events in range: ", eventsInRange.size());
        CHECK(eventsInRange.size() == 8);
        
        MESSAGE("Validating specific holiday dates between Nov 1, 2025 and Jan 4, 2026");
        // Check specific dates
        // Nov 1, 2025 - Ognissanti
        time_t nov1 = startDate;
        bool foundNov1Event = false;
        for (auto event : eventsInRange) {
            struct tm* tm = localtime(&event->startTime);
            if (tm->tm_year + 1900 == 2025 && tm->tm_mon == 10 && tm->tm_mday == 1) {
                if (event->summary.indexOf("Ognissanti") != -1) {
                    foundNov1Event = true;
                    char msg[128];
                    snprintf(msg, sizeof(msg), "✓ Nov 1, 2025: %s", event->summary.c_str());
                    MESSAGE(msg);
                    break;
                }
            }
        }
        CHECK(foundNov1Event);

        // Dec 8, 2025 - Immacolata Concezione
        bool foundDec8Event = false;
        for (auto event : eventsInRange) {
            struct tm* tm = localtime(&event->startTime);
            if (tm->tm_year + 1900 == 2025 && tm->tm_mon == 11 && tm->tm_mday == 8) {
                if (event->summary.indexOf("Immacolata Concezione") != -1) {
                    foundDec8Event = true;
                    char msg[128];
                    snprintf(msg, sizeof(msg), "✓ Dec 8, 2025: %s", event->summary.c_str());
                    MESSAGE(msg);
                    break;
                }
            }
        }
        CHECK(foundDec8Event);

        // Dec 25, 2025 - Natale
        bool foundDec25Event = false;
        for (auto event : eventsInRange) {
            struct tm* tm = localtime(&event->startTime);
            if (tm->tm_year + 1900 == 2025 && tm->tm_mon == 11 && tm->tm_mday == 25) {
                if (event->summary.indexOf("Giorno di Natale") != -1) {
                    foundDec25Event = true;
                    char msg[128];
                    snprintf(msg, sizeof(msg), "✓ Dec 25, 2025: %s", event->summary.c_str());
                    MESSAGE(msg);
                    break;
                }
            }
        }
        CHECK(foundDec25Event);

        // Jan 1, 2026 - Capodanno
        bool foundJan1Event = false;
        for (auto event : eventsInRange) {
            struct tm* tm = localtime(&event->startTime);
            if (tm->tm_year + 1900 == 2026 && tm->tm_mon == 0 && tm->tm_mday == 1) {
                if (event->summary.indexOf("Capodanno") != -1 && event->summary.indexOf("Vigilia") == -1) {
                    foundJan1Event = true;
                    char msg[128];
                    snprintf(msg, sizeof(msg), "✓ Jan 1, 2026: %s", event->summary.c_str());
                    MESSAGE(msg);
                    break;
                }
            }
        }
        CHECK(foundJan1Event);

        // Clean up
        for (auto event : eventsInRange) {
            delete event;
        }
    }
}
