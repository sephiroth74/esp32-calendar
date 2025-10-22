// Remote test for ICS parser on ESP32
// This test runs on the actual device with real memory constraints

#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include "ics_parser.h"
#include "calendar_event.h"

// Test ICS content
const char* SIMPLE_VALID_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//ESP32 Test Calendar//EN
CALSCALE:GREGORIAN
X-WR-CALNAME:ESP32 Test Calendar
X-WR-TIMEZONE:Europe/Zurich
END:VCALENDAR)";

const char* ICS_WITH_EVENTS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//ESP32 Test Calendar//EN
BEGIN:VEVENT
DTSTART:20251026T100000Z
DTEND:20251026T110000Z
UID:test-event-1@esp32.local
SUMMARY:Test Event 1
LOCATION:ESP32 Lab
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251225
DTEND;VALUE=DATE:20251226
UID:christmas@esp32.local
SUMMARY:Christmas Day
DESCRIPTION:All day event
END:VEVENT
BEGIN:VEVENT
DTSTART:20250101T000000Z
DTEND:20250101T010000Z
UID:newyear@esp32.local
SUMMARY:New Year Celebration
RRULE:FREQ=YEARLY
END:VEVENT
END:VCALENDAR)";

// setUp and tearDown are now defined in test_main.cpp

void test_ics_parser_initialization() {
    Serial.println("\n=== Testing ICS Parser Initialization ===");

    ICSParser parser;

    TEST_ASSERT_FALSE_MESSAGE(parser.isValid(), "Parser should not be valid before loading");
    TEST_ASSERT_EQUAL_STRING("", parser.getLastError().c_str());
    TEST_ASSERT_EQUAL_INT(0, parser.getEventCount());

    Serial.println("✓ Parser initialized correctly");
}

void test_parse_simple_calendar() {
    Serial.println("\n=== Testing Simple Calendar Parsing ===");

    ICSParser parser;
    bool result = parser.loadFromString(SIMPLE_VALID_ICS);

    TEST_ASSERT_TRUE_MESSAGE(result, "Simple calendar should parse successfully");
    TEST_ASSERT_TRUE_MESSAGE(parser.isValid(), "Parser should be valid after successful parse");
    TEST_ASSERT_EQUAL_STRING("2.0", parser.getVersion().c_str());
    TEST_ASSERT_EQUAL_STRING("-//Test//ESP32 Test Calendar//EN", parser.getProductId().c_str());
    TEST_ASSERT_EQUAL_STRING("GREGORIAN", parser.getCalendarScale().c_str());
    TEST_ASSERT_EQUAL_STRING("ESP32 Test Calendar", parser.getCalendarName().c_str());
    TEST_ASSERT_EQUAL_STRING("Europe/Zurich", parser.getTimezone().c_str());

    Serial.println("✓ Simple calendar parsed successfully");
    Serial.println("  Calendar: " + parser.getCalendarName());
    Serial.println("  Timezone: " + parser.getTimezone());
}

void test_parse_events() {
    Serial.println("\n=== Testing Event Parsing ===");

    ICSParser parser;
    bool result = parser.loadFromString(ICS_WITH_EVENTS);

    TEST_ASSERT_TRUE_MESSAGE(result, "Calendar with events should parse successfully");
    TEST_ASSERT_EQUAL_INT(3, parser.getEventCount());

    // Test first event (regular timed event)
    CalendarEvent* event1 = parser.getEvent(0);
    TEST_ASSERT_NOT_NULL(event1);
    TEST_ASSERT_EQUAL_STRING("test-event-1@esp32.local", event1->uid.c_str());
    TEST_ASSERT_EQUAL_STRING("Test Event 1", event1->summary.c_str());
    TEST_ASSERT_EQUAL_STRING("ESP32 Lab", event1->location.c_str());
    TEST_ASSERT_FALSE(event1->allDay);
    TEST_ASSERT_FALSE(event1->isRecurring);

    // Test second event (all-day event)
    CalendarEvent* event2 = parser.getEvent(1);
    TEST_ASSERT_NOT_NULL(event2);
    TEST_ASSERT_EQUAL_STRING("christmas@esp32.local", event2->uid.c_str());
    TEST_ASSERT_EQUAL_STRING("Christmas Day", event2->summary.c_str());
    TEST_ASSERT_TRUE(event2->allDay);
    TEST_ASSERT_FALSE(event2->isRecurring);

    // Test third event (recurring event)
    CalendarEvent* event3 = parser.getEvent(2);
    TEST_ASSERT_NOT_NULL(event3);
    TEST_ASSERT_EQUAL_STRING("newyear@esp32.local", event3->uid.c_str());
    TEST_ASSERT_EQUAL_STRING("New Year Celebration", event3->summary.c_str());
    TEST_ASSERT_TRUE(event3->isRecurring);
    TEST_ASSERT_EQUAL_STRING("FREQ=YEARLY", event3->rrule.c_str());

    Serial.println("✓ All 3 events parsed correctly");
    Serial.println("  Event 1: " + event1->summary + " (timed)");
    Serial.println("  Event 2: " + event2->summary + " (all-day)");
    Serial.println("  Event 3: " + event3->summary + " (recurring)");
}

void test_date_range_filtering() {
    Serial.println("\n=== Testing Date Range Filtering ===");

    ICSParser parser;
    parser.loadFromString(ICS_WITH_EVENTS);

    // Create date range for October 2025
    struct tm startTm = {0};
    startTm.tm_year = 2025 - 1900;
    startTm.tm_mon = 9;  // October (0-based)
    startTm.tm_mday = 1;
    time_t startDate = mktime(&startTm);

    struct tm endTm = {0};
    endTm.tm_year = 2025 - 1900;
    endTm.tm_mon = 9;  // October
    endTm.tm_mday = 31;
    time_t endDate = mktime(&endTm);

    std::vector<CalendarEvent*> octoberEvents = parser.getEventsInRange(startDate, endDate);

    TEST_ASSERT_EQUAL_INT(1, octoberEvents.size());
    if (octoberEvents.size() > 0) {
        TEST_ASSERT_EQUAL_STRING("Test Event 1", octoberEvents[0]->summary.c_str());
    }

    Serial.println("✓ Date range filtering works");
    Serial.printf("  Found %d event(s) in October 2025\n", octoberEvents.size());
}

void test_memory_management() {
    Serial.println("\n=== Testing Memory Management ===");

    uint32_t freeHeapBefore = ESP.getFreeHeap();
    Serial.printf("  Free heap before: %d bytes\n", freeHeapBefore);

    {
        ICSParser parser;
        parser.loadFromString(ICS_WITH_EVENTS);
        TEST_ASSERT_EQUAL_INT(3, parser.getEventCount());

        uint32_t freeHeapDuring = ESP.getFreeHeap();
        Serial.printf("  Free heap with parser loaded: %d bytes\n", freeHeapDuring);
    }

    uint32_t freeHeapAfter = ESP.getFreeHeap();
    Serial.printf("  Free heap after cleanup: %d bytes\n", freeHeapAfter);

    // Memory should be mostly restored after parser destruction
    int32_t memoryLeak = freeHeapBefore - freeHeapAfter;
    Serial.printf("  Memory difference: %d bytes\n", memoryLeak);

    TEST_ASSERT_LESS_THAN_INT32_MESSAGE(1024, memoryLeak,
        "Memory leak should be less than 1KB");

    Serial.println("✓ Memory properly cleaned up");
}

void test_load_from_file() {
    Serial.println("\n=== Testing Load from File ===");

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        TEST_FAIL_MESSAGE("Failed to mount LittleFS");
        return;
    }

    // Create a test ICS file
    File testFile = LittleFS.open("/test_calendar.ics", "w");
    if (!testFile) {
        TEST_FAIL_MESSAGE("Failed to create test file");
        return;
    }

    testFile.print(ICS_WITH_EVENTS);
    testFile.close();
    Serial.println("  Test file created");

    // Test loading from file
    ICSParser parser;
    bool result = parser.loadFromFile("/test_calendar.ics");

    TEST_ASSERT_TRUE_MESSAGE(result, "Should load from file successfully");
    TEST_ASSERT_TRUE(parser.isValid());
    TEST_ASSERT_EQUAL_INT(3, parser.getEventCount());

    // Clean up
    LittleFS.remove("/test_calendar.ics");

    Serial.println("✓ File loading works correctly");
}

void test_rrule_parsing() {
    Serial.println("\n=== Testing RRULE Parsing ===");

    const char* rruleTest = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//RRULE Test//EN
BEGIN:VEVENT
UID:weekly@test.com
SUMMARY:Weekly Meeting
DTSTART:20250101T100000Z
DTEND:20250101T110000Z
RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=10
END:VEVENT
BEGIN:VEVENT
UID:monthly@test.com
SUMMARY:Monthly Review
DTSTART:20250115T140000Z
DTEND:20250115T150000Z
RRULE:FREQ=MONTHLY;INTERVAL=1
END:VEVENT
END:VCALENDAR)";

    ICSParser parser;
    parser.loadFromString(rruleTest);

    TEST_ASSERT_EQUAL_INT(2, parser.getEventCount());

    CalendarEvent* weekly = parser.getEvent(0);
    TEST_ASSERT_NOT_NULL(weekly);
    TEST_ASSERT_TRUE(weekly->isRecurring);
    TEST_ASSERT_EQUAL_STRING("FREQ=WEEKLY;INTERVAL=2;COUNT=10", weekly->rrule.c_str());

    CalendarEvent* monthly = parser.getEvent(1);
    TEST_ASSERT_NOT_NULL(monthly);
    TEST_ASSERT_TRUE(monthly->isRecurring);
    TEST_ASSERT_EQUAL_STRING("FREQ=MONTHLY;INTERVAL=1", monthly->rrule.c_str());

    // Test RRULE parsing
    String freq;
    int interval = 0;
    int count = 0;
    String until;

    bool parsed = parser.parseRRule(weekly->rrule, freq, interval, count, until);
    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_EQUAL_STRING("WEEKLY", freq.c_str());
    TEST_ASSERT_EQUAL_INT(2, interval);
    TEST_ASSERT_EQUAL_INT(10, count);

    Serial.println("✓ RRULE parsing works correctly");
    Serial.println("  Weekly: " + weekly->rrule);
    Serial.println("  Monthly: " + monthly->rrule);
}

void test_folded_lines() {
    Serial.println("\n=== Testing Folded Lines ===");

    const char* foldedICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Folded Lines Test//EN
X-WR-CALDESC:This is a very long description that needs to be folded beca
 use it exceeds the 75 character limit per line according to RFC 5545 spec
 ifications for content lines
BEGIN:VEVENT
UID:folded@test.com
SUMMARY:Event with a very long summary that will be folded across multiple
  lines to test the line folding functionality
DTSTART:20250101T100000Z
DTEND:20250101T110000Z
END:VEVENT
END:VCALENDAR)";

    ICSParser parser;
    bool result = parser.loadFromString(foldedICS);

    TEST_ASSERT_TRUE(result);

    String expectedDesc = "This is a very long description that needs to be folded because it exceeds the 75 character limit per line according to RFC 5545 specifications for content lines";
    TEST_ASSERT_EQUAL_STRING(expectedDesc.c_str(), parser.getCalendarDescription().c_str());

    CalendarEvent* event = parser.getEvent(0);
    TEST_ASSERT_NOT_NULL(event);

    String expectedSummary = "Event with a very long summary that will be folded across multiple lines to test the line folding functionality";
    TEST_ASSERT_EQUAL_STRING(expectedSummary.c_str(), event->summary.c_str());

    Serial.println("✓ Folded lines properly unfolded");
}

void test_timezone_support() {
    Serial.println("\n=== Testing Timezone Support ===");

    const char* tzTest = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Timezone Test//EN
BEGIN:VEVENT
UID:tz-ny@test.com
SUMMARY:New York Event
DTSTART;TZID=America/New_York:20250714T133000
DTEND;TZID=America/New_York:20250714T143000
END:VEVENT
BEGIN:VEVENT
UID:tz-utc@test.com
SUMMARY:UTC Event
DTSTART:20250714T173000Z
DTEND:20250714T183000Z
END:VEVENT
END:VCALENDAR)";

    ICSParser parser;
    parser.loadFromString(tzTest);

    TEST_ASSERT_EQUAL_INT(2, parser.getEventCount());

    CalendarEvent* nyEvent = parser.getEvent(0);
    TEST_ASSERT_NOT_NULL(nyEvent);
    TEST_ASSERT_EQUAL_STRING("20250714T133000", nyEvent->dtStart.c_str());
    TEST_ASSERT_EQUAL_STRING("America/New_York", nyEvent->timezone.c_str());

    CalendarEvent* utcEvent = parser.getEvent(1);
    TEST_ASSERT_NOT_NULL(utcEvent);
    TEST_ASSERT_EQUAL_STRING("20250714T173000Z", utcEvent->dtStart.c_str());

    Serial.println("✓ Timezone handling works");
    Serial.println("  NY Event: " + nyEvent->timezone);
    Serial.println("  UTC Event: " + utcEvent->dtStart);
}

void test_clear_and_reload() {
    Serial.println("\n=== Testing Clear and Reload ===");

    ICSParser parser;

    // First load
    parser.loadFromString(ICS_WITH_EVENTS);
    TEST_ASSERT_EQUAL_INT(3, parser.getEventCount());

    // Clear
    parser.clear();
    TEST_ASSERT_FALSE(parser.isValid());
    TEST_ASSERT_EQUAL_INT(0, parser.getEventCount());
    TEST_ASSERT_EQUAL_STRING("", parser.getVersion().c_str());

    // Reload with different content
    parser.loadFromString(SIMPLE_VALID_ICS);
    TEST_ASSERT_TRUE(parser.isValid());
    TEST_ASSERT_EQUAL_INT(0, parser.getEventCount());  // Simple calendar has no events
    TEST_ASSERT_EQUAL_STRING("2.0", parser.getVersion().c_str());

    Serial.println("✓ Clear and reload works correctly");
}

// Setup function for ICS parser tests (called from test_main.cpp)
void setup_ics_parser_tests() {
    // Print memory info
    Serial.println("Initializing ICS Parser tests...");
    Serial.printf("Total heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    if (ESP.getPsramSize() > 0) {
        Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    }
}