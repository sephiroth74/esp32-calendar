#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <ctime>

// Mock Arduino environment for native testing
#ifdef NATIVE_TEST
#include "mock_arduino.h"
MockSerial Serial;
MockLittleFS LittleFS;
#else
#include <Arduino.h>
#endif

// Include the actual parser and event classes
#include "../src/calendar_event.cpp"
#include "../src/ics_parser.cpp"

// Test fixtures - ICS file content
const char* BASIC_VALID_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Test Calendar
X-WR-CALDESC:A test calendar for unit testing
X-WR-TIMEZONE:Europe/Rome
END:VCALENDAR)";

const char* MINIMAL_VALID_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
END:VCALENDAR)";

const char* MISSING_VERSION_ICS = R"(BEGIN:VCALENDAR
PRODID:-//Test//Test Calendar 1.0//EN
END:VCALENDAR)";

const char* WRONG_VERSION_ICS = R"(BEGIN:VCALENDAR
VERSION:1.0
PRODID:-//Test//Test Calendar 1.0//EN
END:VCALENDAR)";

const char* MISSING_PRODID_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
END:VCALENDAR)";

const char* MISSING_BEGIN_ICS = R"(VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
END:VCALENDAR)";

const char* MISSING_END_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN)";

const char* FOLDED_LINES_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
X-WR-CALDESC:This is a very long description that needs to be folded beca
 use it exceeds the 75 character limit per line according to RFC 5545 spec
 ifications for content lines
END:VCALENDAR)";

const char* GOOGLE_CALENDAR_HEADER = R"(BEGIN:VCALENDAR
PRODID:-//Google Inc//Google Calendar 70.9054//EN
VERSION:2.0
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Festività in Svizzera
X-WR-TIMEZONE:Europe/Rome
X-WR-CALDESC:Festività e ricorrenze in Svizzera
BEGIN:VTIMEZONE
TZID:Europe/Rome
X-LIC-LOCATION:Europe/Rome
BEGIN:DAYLIGHT
TZOFFSETFROM:+0100
TZOFFSETTO:+0200
TZNAME:CEST
DTSTART:19700329T020000
RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=-1SU
END:DAYLIGHT
BEGIN:STANDARD
TZOFFSETFROM:+0200
TZOFFSETTO:+0100
TZNAME:CET
DTSTART:19701025T030000
RRULE:FREQ=YEARLY;BYMONTH=10;BYDAY=-1SU
END:STANDARD
END:VTIMEZONE
END:VCALENDAR)";

// Test cases
TEST_CASE("ICSParser - Basic Initialization") {
    MESSAGE("Testing ICSParser initialization");

    ICSParser parser;

    INFO("Checking initial state of parser");
    CHECK(parser.isValid() == false);
    CHECK(parser.getLastError() == "");
    CHECK(parser.getErrorCode() == ICSParseError::NONE);
    CHECK(parser.getEventCount() == 0);

    CAPTURE(parser.isValid());
    CAPTURE(parser.getEventCount());
}

TEST_CASE("ICSParser - Parse Valid Basic Calendar") {
    MESSAGE("Testing parsing of a valid basic calendar");

    ICSParser parser;

    INFO("Loading ICS data from string");
    bool result = parser.loadFromString(BASIC_VALID_ICS);

    INFO("Checking parse result");
    CHECK(result == true);
    CHECK(parser.isValid() == true);
    CHECK(parser.getErrorCode() == ICSParseError::NONE);

    INFO("Verifying parsed calendar properties");
    CHECK(parser.getVersion() == "2.0");
    CHECK(parser.getProductId() == "-//Test//Test Calendar 1.0//EN");
    CHECK(parser.getCalendarScale() == "GREGORIAN");
    CHECK(parser.getMethod() == "PUBLISH");
    CHECK(parser.getCalendarName() == "Test Calendar");
    CHECK(parser.getCalendarDescription() == "A test calendar for unit testing");
    CHECK(parser.getTimezone() == "Europe/Rome");

    // Log captured values for debugging
    CAPTURE(parser.getVersion());
    CAPTURE(parser.getProductId());
    CAPTURE(parser.getCalendarName());
    CAPTURE(parser.getTimezone());
}

TEST_CASE("ICSParser - Parse Minimal Valid Calendar") {
    ICSParser parser;

    bool result = parser.loadFromString(MINIMAL_VALID_ICS);

    CHECK(result == true);
    CHECK(parser.isValid() == true);
    CHECK(parser.getVersion() == "2.0");
    CHECK(parser.getProductId() == "-//Test//Test Calendar 1.0//EN");
    CHECK(parser.getCalendarScale() == "GREGORIAN"); // Default value
    CHECK(parser.getMethod() == "");
    CHECK(parser.getCalendarName() == "");
}

TEST_CASE("ICSParser - Missing VERSION") {
    ICSParser parser;

    bool result = parser.loadFromString(MISSING_VERSION_ICS);

    CHECK(result == false);
    CHECK(parser.isValid() == false);
    CHECK(parser.getErrorCode() == ICSParseError::MISSING_VERSION);
    CHECK(parser.getLastError() == "Missing VERSION property");
}

TEST_CASE("ICSParser - Wrong VERSION") {
    MESSAGE("Testing parser with unsupported version");

    ICSParser parser;

    INFO("Loading ICS with version 1.0");
    bool result = parser.loadFromString(WRONG_VERSION_ICS);

    INFO("Verifying parser rejects unsupported version");
    CHECK(result == false);
    CHECK(parser.isValid() == false);
    CHECK(parser.getErrorCode() == ICSParseError::UNSUPPORTED_VERSION);
    CHECK(parser.getLastError() == "Unsupported version: 1.0 (only 2.0 is supported)");

    CAPTURE(parser.getErrorCode());
    CAPTURE(parser.getLastError());
}

TEST_CASE("ICSParser - Missing PRODID") {
    ICSParser parser;

    bool result = parser.loadFromString(MISSING_PRODID_ICS);

    CHECK(result == false);
    CHECK(parser.isValid() == false);
    CHECK(parser.getErrorCode() == ICSParseError::MISSING_PRODID);
    CHECK(parser.getLastError() == "Missing PRODID property");
}

TEST_CASE("ICSParser - Missing BEGIN:VCALENDAR") {
    ICSParser parser;

    bool result = parser.loadFromString(MISSING_BEGIN_ICS);

    CHECK(result == false);
    CHECK(parser.isValid() == false);
    CHECK(parser.getErrorCode() == ICSParseError::MISSING_BEGIN_CALENDAR);
}

TEST_CASE("ICSParser - Missing END:VCALENDAR") {
    ICSParser parser;

    bool result = parser.loadFromString(MISSING_END_ICS);

    CHECK(result == false);
    CHECK(parser.isValid() == false);
    CHECK(parser.getErrorCode() == ICSParseError::MISSING_END_CALENDAR);
}

TEST_CASE("ICSParser - Folded Lines") {
    ICSParser parser;

    bool result = parser.loadFromString(FOLDED_LINES_ICS);

    CHECK(result == true);
    CHECK(parser.isValid() == true);

    // Check that the folded description was properly unfolded
    String expectedDesc = "This is a very long description that needs to be folded because it exceeds the 75 character limit per line according to RFC 5545 specifications for content lines";
    CHECK(parser.getCalendarDescription() == expectedDesc);
}

TEST_CASE("ICSParser - Google Calendar Header") {
    ICSParser parser;

    bool result = parser.loadFromString(GOOGLE_CALENDAR_HEADER);

    CHECK(result == true);
    CHECK(parser.isValid() == true);
    CHECK(parser.getVersion() == "2.0");
    CHECK(parser.getProductId() == "-//Google Inc//Google Calendar 70.9054//EN");
    CHECK(parser.getCalendarName() == "Festività in Svizzera");
    CHECK(parser.getTimezone() == "Europe/Rome");
    CHECK(parser.getCalendarDescription() == "Festività e ricorrenze in Svizzera");
}

TEST_CASE("ICSParser - Clear Method") {
    ICSParser parser;

    // Load a calendar
    parser.loadFromString(BASIC_VALID_ICS);
    CHECK(parser.isValid() == true);
    CHECK(parser.getVersion() == "2.0");

    // Clear it
    parser.clear();

    // Check everything is reset
    CHECK(parser.isValid() == false);
    CHECK(parser.getVersion() == "");
    CHECK(parser.getProductId() == "");
    CHECK(parser.getCalendarName() == "");
    CHECK(parser.getEventCount() == 0);
}

TEST_CASE("ICSParser - Parse Line Method") {
    ICSParser parser;

    String name, params, value;

    SUBCASE("Simple property") {
        bool result = parser.parseLine("VERSION:2.0", name, params, value);
        CHECK(result == true);
        CHECK(name == "VERSION");
        CHECK(params == "");
        CHECK(value == "2.0");
    }

    SUBCASE("Property with parameters") {
        bool result = parser.parseLine("DTSTART;VALUE=DATE:20251026", name, params, value);
        CHECK(result == true);
        CHECK(name == "DTSTART");
        CHECK(params == "VALUE=DATE");
        CHECK(value == "20251026");
    }

    SUBCASE("Property with multiple parameters") {
        bool result = parser.parseLine("DTSTART;TZID=Europe/Rome;VALUE=DATE-TIME:20251026T080000", name, params, value);
        CHECK(result == true);
        CHECK(name == "DTSTART");
        CHECK(params == "TZID=Europe/Rome;VALUE=DATE-TIME");
        CHECK(value == "20251026T080000");
    }

    SUBCASE("Invalid line without colon") {
        bool result = parser.parseLine("INVALID LINE", name, params, value);
        CHECK(result == false);
    }
}

// Test fixture files
TEST_CASE("ICSParser - Parse Fixture Files") {
    MESSAGE("Testing parsing of actual ICS fixture files");

    SUBCASE("Parse basic_calendar.ics") {
        MESSAGE("Loading basic_calendar.ics fixture");

        // Read the file content manually for testing
        const char* basic_calendar_content = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Test Calendar
X-WR-CALDESC:A test calendar for unit testing
X-WR-TIMEZONE:Europe/Rome
BEGIN:VEVENT
DTSTART:20251026T080000Z
DTEND:20251026T090000Z
DTSTAMP:20251020T112155Z
UID:test-event-1@example.com
SUMMARY:Test Event 1
DESCRIPTION:This is a test event
LOCATION:Test Location
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251027
DTEND;VALUE=DATE:20251028
DTSTAMP:20251020T112155Z
UID:test-event-2@example.com
SUMMARY:All Day Event
DESCRIPTION:This is an all-day event
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(basic_calendar_content);

        INFO("Verifying basic calendar loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getCalendarName() == "Test Calendar");
        CHECK(parser.getTimezone() == "Europe/Rome");

        CAPTURE(parser.getCalendarName());
        CHECK(parser.getEventCount() == 2);  // basic_calendar.ics has 2 events

        // Check events extracted
        CalendarEvent* evt1 = parser.getEvent(0);
        CalendarEvent* evt2 = parser.getEvent(1);
        CHECK(evt1 != nullptr);
        CHECK(evt2 != nullptr);
        CHECK(evt1->summary == "Test Event 1");
        CHECK(evt2->summary == "All Day Event");
        CHECK(evt2->allDay == true);  // Second event is all-day

        CAPTURE(parser.getEventCount());
    }

    SUBCASE("Parse google_calendar.ics") {
        MESSAGE("Loading google_calendar.ics fixture");

        const char* google_calendar_content = R"(BEGIN:VCALENDAR
PRODID:-//Google Inc//Google Calendar 70.9054//EN
VERSION:2.0
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Festività in Svizzera
X-WR-TIMEZONE:Europe/Rome
BEGIN:VTIMEZONE
TZID:Europe/Rome
X-LIC-LOCATION:Europe/Rome
BEGIN:DAYLIGHT
TZOFFSETFROM:+0100
TZOFFSETTO:+0200
TZNAME:CEST
DTSTART:19700329T020000
RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=-1SU
END:DAYLIGHT
BEGIN:STANDARD
TZOFFSETFROM:+0200
TZOFFSETTO:+0100
TZNAME:CET
DTSTART:19701025T030000
RRULE:FREQ=YEARLY;BYMONTH=10;BYDAY=-1SU
END:STANDARD
END:VTIMEZONE
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251026
DTEND;VALUE=DATE:20251027
DTSTAMP:20251020T112155Z
UID:20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com
CLASS:PUBLIC
CREATED:20240517T100942Z
DESCRIPTION:Ricorrenza\nPer nascondere le ricorrenze\, vai alle impostazion
 i di Google Calendar > Festività in Svizzera
LAST-MODIFIED:20240517T100942Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:l'ora legale termina
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251225
DTEND;VALUE=DATE:20251226
DTSTAMP:20251020T112155Z
UID:20251225_christmas@google.com
CLASS:PUBLIC
SUMMARY:Natale
TRANSP:TRANSPARENT
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(google_calendar_content);

        INFO("Verifying Google calendar loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getProductId() == "-//Google Inc//Google Calendar 70.9054//EN");
        CHECK(parser.getCalendarName() == "Festività in Svizzera");
        CHECK(parser.getTimezone() == "Europe/Rome");
        CHECK(parser.getEventCount() == 2);  // google_calendar.ics has 2 events

        // Check the DST event
        CalendarEvent* dstEvent = parser.getEvent(0);
        CHECK(dstEvent != nullptr);
        CHECK(dstEvent->summary == "l'ora legale termina");
        CHECK(dstEvent->allDay == true);

        // Check Christmas event
        CalendarEvent* xmasEvent = parser.getEvent(1);
        CHECK(xmasEvent != nullptr);
        CHECK(xmasEvent->summary == "Natale");
        CHECK(xmasEvent->allDay == true);

        CAPTURE(parser.getProductId());
        CAPTURE(parser.getCalendarName());
        CAPTURE(parser.getEventCount());
    }

    SUBCASE("Parse folded_lines.ics") {
        MESSAGE("Loading folded_lines.ics fixture");

        const char* folded_lines_content = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar with Folded Lines//EN
X-WR-CALDESC:This is a very long description that needs to be folded beca
 use it exceeds the 75 character limit per line according to RFC 5545 spec
 ifications for content lines
BEGIN:VEVENT
DTSTART:20251026T100000Z
DTEND:20251026T110000Z
UID:folded-event@example.com
SUMMARY:Event with a very long summary that will be folded across multiple
  lines to test the line folding functionality of the ICS parser
DESCRIPTION:This event has a description that is so incredibly long that i
 t will definitely need to be folded across multiple lines. The RFC 5545 s
 pecification states that content lines should be folded at 75 octets. Thi
 s is to ensure compatibility with older systems and to maintain readabili
 ty of the ICS file format.
LOCATION:A very long location string that might also need to be folded if
 it exceeds the maximum line length specified in the standard
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(folded_lines_content);

        INFO("Verifying folded lines are properly unfolded");
        CHECK(result == true);
        CHECK(parser.isValid() == true);

        String expectedDesc = "This is a very long description that needs to be folded because it exceeds the 75 character limit per line according to RFC 5545 specifications for content lines";
        CHECK(parser.getCalendarDescription() == expectedDesc);
        CHECK(parser.getEventCount() == 1);  // folded_lines.ics has 1 event

        // Check the event with folded summary
        CalendarEvent* event = parser.getEvent(0);
        CHECK(event != nullptr);
        CHECK(event->summary == "Event with a very long summary that will be folded across multiple lines to test the line folding functionality of the ICS parser");

        CAPTURE(parser.getCalendarDescription().length());
        CAPTURE(parser.getCalendarDescription());
        CAPTURE(parser.getEventCount());
    }

    SUBCASE("Summary: Event counts for all fixture files") {
        MESSAGE("Testing event counts across all fixture files");

        // Test empty calendar
        {
            ICSParser parser;
            bool result = parser.loadFromString(MINIMAL_VALID_ICS);
            CHECK(result == true);
            CHECK(parser.getEventCount() == 0);
            CAPTURE(parser.getEventCount());
        }

        // Test basic calendar with events
        {
            const char* basic_cal = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
BEGIN:VEVENT
SUMMARY:Event 1
DTSTART:20251026T080000Z
DTEND:20251026T090000Z
END:VEVENT
BEGIN:VEVENT
SUMMARY:All Day Event
DTSTART;VALUE=DATE:20251027
DTEND;VALUE=DATE:20251028
END:VEVENT
END:VCALENDAR)";

            ICSParser parser;
            bool result = parser.loadFromString(basic_cal);
            CHECK(result == true);
            CHECK(parser.getEventCount() == 2);
            MESSAGE("Total events loaded: 2");

            // Check for all-day event
            bool foundAllDay = false;
            for (size_t i = 0; i < parser.getEventCount(); i++) {
                CalendarEvent* evt = parser.getEvent(i);
                if (evt && evt->allDay) {
                    foundAllDay = true;
                    break;
                }
            }
            CHECK(foundAllDay == true);
            CAPTURE(parser.getEventCount());
        }

        // Summary output
        MESSAGE("Event count test summary:");
        MESSAGE("- Empty calendar: 0 events");
        MESSAGE("- Basic calendar: 2 events (1 all-day)");
        MESSAGE("- Google calendar: 2 events (both all-day)");
        MESSAGE("- Folded lines calendar: 1 event");
    }
}

// Test loadFromFile method
TEST_CASE("ICSParser - Load From File") {
    MESSAGE("Testing loadFromFile method");

    // Clear any existing files in mock filesystem
    LittleFS.clear();

    SUBCASE("Load valid file") {
        MESSAGE("Testing loading a valid ICS file");

        // Add a test file to the mock filesystem
        const String testContent = BASIC_VALID_ICS;
        LittleFS.addFile("/calendars/test.ics", testContent);

        ICSParser parser;
        bool result = parser.loadFromFile("/calendars/test.ics");

        INFO("Checking file loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getVersion() == "2.0");
        CHECK(parser.getCalendarName() == "Test Calendar");

        CAPTURE(parser.getCalendarName());
        CAPTURE(parser.getTimezone());
    }

    SUBCASE("File not found") {
        MESSAGE("Testing non-existent file");

        ICSParser parser;
        bool result = parser.loadFromFile("/nonexistent.ics");

        INFO("Checking proper error handling for missing file");
        CHECK(result == false);
        CHECK(parser.isValid() == false);
        CHECK(parser.getErrorCode() == ICSParseError::FILE_NOT_FOUND);

        CAPTURE(parser.getLastError());
    }

    SUBCASE("Load multiple files sequentially") {
        MESSAGE("Testing loading multiple files");

        // Add two different calendars
        LittleFS.addFile("/cal1.ics", MINIMAL_VALID_ICS);
        LittleFS.addFile("/cal2.ics", BASIC_VALID_ICS);

        ICSParser parser;

        INFO("Loading first calendar");
        bool result1 = parser.loadFromFile("/cal1.ics");
        CHECK(result1 == true);
        CHECK(parser.getCalendarName() == "");  // Minimal calendar has no name

        INFO("Loading second calendar (should clear first)");
        bool result2 = parser.loadFromFile("/cal2.ics");
        CHECK(result2 == true);
        CHECK(parser.getCalendarName() == "Test Calendar");

        CAPTURE(parser.getEventCount());
        MESSAGE("Event count after loading second calendar: (see captured value)");
    }

    SUBCASE("Load file with invalid content") {
        MESSAGE("Testing file with invalid ICS content");

        // Add a file with invalid content
        LittleFS.addFile("/invalid.ics", "This is not a valid ICS file");

        ICSParser parser;
        bool result = parser.loadFromFile("/invalid.ics");

        INFO("Checking parser rejects invalid content");
        CHECK(result == false);
        CHECK(parser.isValid() == false);
        CHECK(parser.getErrorCode() == ICSParseError::MISSING_BEGIN_CALENDAR);

        CAPTURE(parser.getLastError());
    }
}

// Test loadFromStream method
TEST_CASE("ICSParser - Load From Stream") {
    MESSAGE("Testing loadFromStream method");

    SUBCASE("Load valid stream") {
        MESSAGE("Testing loading from a valid stream");

        StringStream stream(BASIC_VALID_ICS);

        ICSParser parser;
        bool result = parser.loadFromStream(&stream);

        INFO("Checking stream loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getVersion() == "2.0");
        CHECK(parser.getCalendarName() == "Test Calendar");
        CHECK(parser.getTimezone() == "Europe/Rome");

        CAPTURE(parser.getProductId());
    }

    SUBCASE("Load from null stream") {
        MESSAGE("Testing null stream handling");

        ICSParser parser;
        bool result = parser.loadFromStream(nullptr);

        INFO("Checking proper error handling for null stream");
        CHECK(result == false);
        CHECK(parser.isValid() == false);
        CHECK(parser.getErrorCode() == ICSParseError::STREAM_READ_ERROR);
        CHECK(parser.getLastError() == "Invalid stream");
    }

    SUBCASE("Load from empty stream") {
        MESSAGE("Testing empty stream");

        StringStream stream("");

        ICSParser parser;
        bool result = parser.loadFromStream(&stream);

        INFO("Checking handling of empty stream");
        CHECK(result == false);
        CHECK(parser.isValid() == false);
        CHECK(parser.getErrorCode() == ICSParseError::MISSING_BEGIN_CALENDAR);
    }

    SUBCASE("Load Google Calendar from stream") {
        MESSAGE("Testing Google Calendar format from stream");

        StringStream stream(GOOGLE_CALENDAR_HEADER);

        ICSParser parser;
        bool result = parser.loadFromStream(&stream);

        INFO("Checking Google Calendar properties");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getProductId() == "-//Google Inc//Google Calendar 70.9054//EN");
        CHECK(parser.getCalendarName() == "Festività in Svizzera");

        CAPTURE(parser.getCalendarDescription());
    }

    SUBCASE("Stream with folded lines") {
        MESSAGE("Testing stream with folded lines");

        StringStream stream(FOLDED_LINES_ICS);

        ICSParser parser;
        bool result = parser.loadFromStream(&stream);

        INFO("Checking folded lines are properly handled");
        CHECK(result == true);
        CHECK(parser.isValid() == true);

        String expectedDesc = "This is a very long description that needs to be folded because it exceeds the 75 character limit per line according to RFC 5545 specifications for content lines";
        CHECK(parser.getCalendarDescription() == expectedDesc);
    }

    SUBCASE("Reuse stream after reading") {
        MESSAGE("Testing stream reuse");

        StringStream stream(MINIMAL_VALID_ICS);

        ICSParser parser1;
        bool result1 = parser1.loadFromStream(&stream);
        CHECK(result1 == true);

        // Reset stream and use with another parser
        stream.reset();

        ICSParser parser2;
        bool result2 = parser2.loadFromStream(&stream);
        CHECK(result2 == true);
        CHECK(parser2.getVersion() == "2.0");
    }
}

// Test event counting and extraction
TEST_CASE("ICSParser - Event Counting and Extraction") {
    MESSAGE("Testing event counting and basic extraction");

    SUBCASE("Count events in basic calendar") {
        MESSAGE("Counting events in basic_calendar.ics");

        const char* basic_calendar_with_events = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar 1.0//EN
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Test Calendar
X-WR-TIMEZONE:Europe/Rome
BEGIN:VEVENT
DTSTART:20251026T080000Z
DTEND:20251026T090000Z
DTSTAMP:20251020T112155Z
UID:test-event-1@example.com
CREATED:20240517T100942Z
LAST-MODIFIED:20240518T100942Z
SUMMARY:Test Event 1
DESCRIPTION:This is a test event
LOCATION:Test Location
STATUS:CONFIRMED
TRANSP:OPAQUE
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251027
DTEND;VALUE=DATE:20251028
DTSTAMP:20251020T112155Z
UID:test-event-2@example.com
CREATED:20240517T100942Z
LAST-MODIFIED:20240519T100942Z
SUMMARY:All Day Event
DESCRIPTION:This is an all-day event
STATUS:CONFIRMED
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART:20251028T140000Z
DTEND:20251028T150000Z
DTSTAMP:20251020T112155Z
UID:test-event-3@example.com
CREATED:20240517T100942Z
LAST-MODIFIED:20240520T100942Z
SUMMARY:Recurring Weekly Meeting
RRULE:FREQ=WEEKLY;BYDAY=TU
LOCATION:Conference Room
STATUS:CONFIRMED
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(basic_calendar_with_events);

        INFO("Verifying calendar loaded and events counted");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getEventCount() == 3);

        INFO("Checking first event properties");
        CalendarEvent* event1 = parser.getEvent(0);
        REQUIRE(event1 != nullptr);
        CHECK(event1->summary == "Test Event 1");
        CHECK(event1->dtStart == "20251026T080000Z");
        CHECK(event1->dtEnd == "20251026T090000Z");
        CHECK(event1->created == "20240517T100942Z");
        CHECK(event1->lastModified == "20240518T100942Z");
        CHECK(event1->dtStamp == "20251020T112155Z");
        CHECK(event1->isRecurring == false);
        CHECK(event1->allDay == false);

        INFO("Checking second event (all-day)");
        CalendarEvent* event2 = parser.getEvent(1);
        REQUIRE(event2 != nullptr);
        CHECK(event2->summary == "All Day Event");
        CHECK(event2->dtStart == "20251027");
        CHECK(event2->dtEnd == "20251028");
        CHECK(event2->allDay == true);
        CHECK(event2->isRecurring == false);

        INFO("Checking third event (recurring)");
        CalendarEvent* event3 = parser.getEvent(2);
        REQUIRE(event3 != nullptr);
        CHECK(event3->summary == "Recurring Weekly Meeting");
        CHECK(event3->isRecurring == true);
        CHECK(event3->rrule == "FREQ=WEEKLY;BYDAY=TU");

        CAPTURE(parser.getEventCount());
        CAPTURE(event1->summary);
        CAPTURE(event3->rrule);
    }

    SUBCASE("Count events in Google Calendar") {
        MESSAGE("Counting events in Google Calendar format");

        const char* google_cal_with_events = R"(BEGIN:VCALENDAR
PRODID:-//Google Inc//Google Calendar 70.9054//EN
VERSION:2.0
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Festività in Svizzera
X-WR-TIMEZONE:Europe/Rome
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251026
DTEND;VALUE=DATE:20251027
DTSTAMP:20251020T112155Z
UID:20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com
CLASS:PUBLIC
CREATED:20240517T100942Z
LAST-MODIFIED:20240517T100942Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:l'ora legale termina
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251225
DTEND;VALUE=DATE:20251226
DTSTAMP:20251020T112155Z
UID:20251225_christmas@google.com
CLASS:PUBLIC
CREATED:20240101T000000Z
LAST-MODIFIED:20240101T000000Z
SUMMARY:Natale
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20250101
DTEND;VALUE=DATE:20250102
DTSTAMP:20251020T112155Z
UID:20250101_newyear@google.com
CREATED:20240101T000000Z
LAST-MODIFIED:20240101T000000Z
SUMMARY:Capodanno
RRULE:FREQ=YEARLY
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(google_cal_with_events);

        INFO("Verifying Google Calendar events");
        CHECK(result == true);
        CHECK(parser.getEventCount() == 3);

        // Check DST event
        CalendarEvent* dstEvent = parser.getEvent(0);
        REQUIRE(dstEvent != nullptr);
        CHECK(dstEvent->summary == "l'ora legale termina");
        CHECK(dstEvent->allDay == true);

        // Check Christmas
        CalendarEvent* xmasEvent = parser.getEvent(1);
        REQUIRE(xmasEvent != nullptr);
        CHECK(xmasEvent->summary == "Natale");

        // Check New Year (recurring)
        CalendarEvent* newYearEvent = parser.getEvent(2);
        REQUIRE(newYearEvent != nullptr);
        CHECK(newYearEvent->summary == "Capodanno");
        CHECK(newYearEvent->isRecurring == true);
        CHECK(newYearEvent->rrule == "FREQ=YEARLY");

        CAPTURE(parser.getEventCount());
    }

    SUBCASE("Empty calendar (no events)") {
        MESSAGE("Testing calendar with no events");

        const char* empty_calendar = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Empty Calendar//EN
X-WR-CALNAME:Empty Calendar
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(empty_calendar);

        INFO("Verifying empty calendar");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getEventCount() == 0);
        CHECK(parser.getEvent(0) == nullptr);

        CAPTURE(parser.getEventCount());
    }

    SUBCASE("Calendar with single event") {
        MESSAGE("Testing calendar with one event");

        const char* single_event_cal = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Single Event//EN
BEGIN:VEVENT
DTSTART:20251031T200000Z
DTEND:20251031T230000Z
DTSTAMP:20251020T112155Z
UID:halloween@example.com
CREATED:20240901T100000Z
LAST-MODIFIED:20241001T100000Z
SUMMARY:Halloween Party
LOCATION:Haunted House
DESCRIPTION:Costume party for Halloween
STATUS:CONFIRMED
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(single_event_cal);

        INFO("Verifying single event calendar");
        CHECK(result == true);
        CHECK(parser.getEventCount() == 1);

        CalendarEvent* event = parser.getEvent(0);
        REQUIRE(event != nullptr);
        CHECK(event->summary == "Halloween Party");
        CHECK(event->location == "Haunted House");
        CHECK(event->description == "Costume party for Halloween");
        CHECK(event->status == "CONFIRMED");
        CHECK(event->created == "20240901T100000Z");
        CHECK(event->lastModified == "20241001T100000Z");

        CAPTURE(event->summary);
        CAPTURE(event->created);
    }

    SUBCASE("Get all events") {
        MESSAGE("Testing getAllEvents method");

        const char* multi_event_cal = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Multi Event//EN
BEGIN:VEVENT
UID:event1@test.com
SUMMARY:Event 1
DTSTART:20251101T100000Z
DTEND:20251101T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event2@test.com
SUMMARY:Event 2
DTSTART:20251102T100000Z
DTEND:20251102T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event3@test.com
SUMMARY:Event 3
DTSTART:20251103T100000Z
DTEND:20251103T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event4@test.com
SUMMARY:Event 4
DTSTART:20251104T100000Z
DTEND:20251104T110000Z
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(multi_event_cal);

        INFO("Verifying getAllEvents returns all events");
        CHECK(result == true);
        CHECK(parser.getEventCount() == 4);

        std::vector<CalendarEvent*> allEvents = parser.getAllEvents();
        CHECK(allEvents.size() == 4);

        // Verify each event
        for (size_t i = 0; i < allEvents.size(); i++) {
            String eventName = "Event " + String(i + 1);
            CHECK(allEvents[i] != nullptr);
            CHECK(allEvents[i]->summary == eventName);
        }

        CAPTURE(allEvents.size());
    }

    SUBCASE("Events with various timestamp formats") {
        MESSAGE("Testing events with different timestamp formats");

        const char* timestamp_test_cal = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Timestamp Test//EN
BEGIN:VEVENT
UID:utc-event@test.com
SUMMARY:UTC Event
DTSTART:20251026T080000Z
DTEND:20251026T090000Z
CREATED:20240101T120000Z
LAST-MODIFIED:20240201T120000Z
DTSTAMP:20241020T120000Z
END:VEVENT
BEGIN:VEVENT
UID:local-event@test.com
SUMMARY:Local Time Event
DTSTART:20251026T080000
DTEND:20251026T090000
CREATED:20240101T120000
LAST-MODIFIED:20240201T120000
DTSTAMP:20241020T120000
END:VEVENT
BEGIN:VEVENT
UID:date-only@test.com
SUMMARY:Date Only Event
DTSTART;VALUE=DATE:20251026
DTEND;VALUE=DATE:20251027
CREATED:20240101T000000Z
DTSTAMP:20241020T000000Z
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(timestamp_test_cal);

        INFO("Verifying events with different timestamp formats");
        CHECK(result == true);
        CHECK(parser.getEventCount() == 3);

        // UTC event
        CalendarEvent* utcEvent = parser.getEvent(0);
        CHECK(utcEvent->dtStart == "20251026T080000Z");
        CHECK(utcEvent->created == "20240101T120000Z");

        // Local time event
        CalendarEvent* localEvent = parser.getEvent(1);
        CHECK(localEvent->dtStart == "20251026T080000");
        CHECK(localEvent->created == "20240101T120000");

        // Date-only event
        CalendarEvent* dateEvent = parser.getEvent(2);
        CHECK(dateEvent->dtStart == "20251026");
        CHECK(dateEvent->allDay == true);

        CAPTURE(utcEvent->dtStart);
        CAPTURE(localEvent->dtStart);
        CAPTURE(dateEvent->dtStart);
    }

    SUBCASE("Recurring Birthday Event") {
        MESSAGE("Testing recurring birthday event parsing");

        const char* birthday_event_cal = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Google Inc//Google Calendar 70.9054//EN
BEGIN:VEVENT
DTSTART;VALUE=DATE:19861022
DTEND;VALUE=DATE:19861023
RRULE:FREQ=YEARLY
DTSTAMP:20251020T111925Z
UID:2i60odkbfk12k5mmbak1gostug@google.com
CLASS:PRIVATE
CREATED:20251020T111824Z
LAST-MODIFIED:20251020T111827Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:Karin Kiplok's birthday
TRANSP:TRANSPARENT
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(birthday_event_cal);

        INFO("Verifying birthday event loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getEventCount() == 1);

        CalendarEvent* birthdayEvent = parser.getEvent(0);
        REQUIRE(birthdayEvent != nullptr);

        INFO("Checking birthday event properties");
        CHECK(birthdayEvent->summary == "Karin Kiplok's birthday");
        CHECK(birthdayEvent->dtStart == "19861022");
        CHECK(birthdayEvent->dtEnd == "19861023");
        CHECK(birthdayEvent->allDay == true);
        CHECK(birthdayEvent->isRecurring == true);
        CHECK(birthdayEvent->rrule == "FREQ=YEARLY");
        CHECK(birthdayEvent->status == "CONFIRMED");
        CHECK(birthdayEvent->created == "20251020T111824Z");
        CHECK(birthdayEvent->lastModified == "20251020T111827Z");
        CHECK(birthdayEvent->uid == "2i60odkbfk12k5mmbak1gostug@google.com");

        // Log the important event details
        MESSAGE("Birthday Event Details:");
        MESSAGE("  Summary: Karin Kiplok's birthday");
        MESSAGE("  Start Date: 19861022");
        MESSAGE("  Recurring: Yes (YEARLY)");
        MESSAGE("  All-day event: Yes");

        CAPTURE(birthdayEvent->summary);
        CAPTURE(birthdayEvent->dtStart);
        CAPTURE(birthdayEvent->isRecurring);
        CAPTURE(birthdayEvent->rrule);
        CAPTURE(birthdayEvent->allDay);
    }
}

// Test date range filtering
TEST_CASE("ICSParser - Date Range Filtering") {
    MESSAGE("Testing date range filtering with getEventsInRange");

    SUBCASE("Filter non-recurring events by date range") {
        MESSAGE("Testing filtering of non-recurring events");

        const char* calendar_with_events = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test Calendar//EN
BEGIN:VEVENT
UID:event1@test.com
SUMMARY:Event in October 2025
DTSTART:20251015T100000Z
DTEND:20251015T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event2@test.com
SUMMARY:Event in November 2025
DTSTART:20251115T100000Z
DTEND:20251115T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event3@test.com
SUMMARY:Event in December 2025
DTSTART:20251215T100000Z
DTEND:20251215T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event4@test.com
SUMMARY:Event in January 2026
DTSTART:20260115T100000Z
DTEND:20260115T110000Z
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(calendar_with_events);

        CHECK(result == true);
        CHECK(parser.getEventCount() == 4);

        // Create date range for November 2025
        // Using simplified date creation for testing
        // Nov 1, 2025 00:00:00 = approximately 1762041600
        // Dec 1, 2025 00:00:00 = approximately 1764633600
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 10;  // November (0-based)
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2025 - 1900;
        endTm.tm_mon = 10;  // November
        endTm.tm_mday = 30;
        endTm.tm_hour = 23;
        endTm.tm_min = 59;
        endTm.tm_sec = 59;
        time_t endDate = mktime(&endTm);

        // Get events in November
        std::vector<CalendarEvent*> novemberEvents = parser.getEventsInRange(startDate, endDate);

        INFO("Checking events filtered for November 2025");
        CHECK(novemberEvents.size() == 1);
        if (novemberEvents.size() > 0) {
            CHECK(novemberEvents[0]->summary == "Event in November 2025");
        }

        CAPTURE(novemberEvents.size());
    }

    SUBCASE("Filter all-day events by date range") {
        MESSAGE("Testing filtering of all-day events");

        const char* allday_calendar = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//All Day Events//EN
BEGIN:VEVENT
UID:allday1@test.com
SUMMARY:Holiday in October
DTSTART;VALUE=DATE:20251020
DTEND;VALUE=DATE:20251021
END:VEVENT
BEGIN:VEVENT
UID:allday2@test.com
SUMMARY:Holiday in November
DTSTART;VALUE=DATE:20251125
DTEND;VALUE=DATE:20251126
END:VEVENT
BEGIN:VEVENT
UID:allday3@test.com
SUMMARY:Holiday in December
DTSTART;VALUE=DATE:20251225
DTEND;VALUE=DATE:20251226
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(allday_calendar);

        CHECK(result == true);
        CHECK(parser.getEventCount() == 3);

        // Get events in November 2025
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 10;  // November
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2025 - 1900;
        endTm.tm_mon = 10;  // November
        endTm.tm_mday = 30;
        time_t endDate = mktime(&endTm);

        std::vector<CalendarEvent*> novemberEvents = parser.getEventsInRange(startDate, endDate);

        INFO("Checking all-day events filtered for November");
        CHECK(novemberEvents.size() == 1);
        if (novemberEvents.size() > 0) {
            CHECK(novemberEvents[0]->summary == "Holiday in November");
            CHECK(novemberEvents[0]->allDay == true);
        }

        CAPTURE(novemberEvents.size());
    }

    SUBCASE("Filter recurring yearly events") {
        MESSAGE("Testing filtering of yearly recurring events");

        const char* recurring_calendar = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Recurring Events//EN
BEGIN:VEVENT
UID:birthday@test.com
SUMMARY:Annual Birthday
DTSTART;VALUE=DATE:20241022
DTEND;VALUE=DATE:20241023
RRULE:FREQ=YEARLY
END:VEVENT
BEGIN:VEVENT
UID:anniversary@test.com
SUMMARY:Annual Anniversary
DTSTART;VALUE=DATE:20240515
DTEND;VALUE=DATE:20240516
RRULE:FREQ=YEARLY
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(recurring_calendar);

        CHECK(result == true);
        CHECK(parser.getEventCount() == 2);

        // Get events for October 2025 (should include the birthday)
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 9;  // October
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2025 - 1900;
        endTm.tm_mon = 9;  // October
        endTm.tm_mday = 31;
        time_t endDate = mktime(&endTm);

        std::vector<CalendarEvent*> octoberEvents = parser.getEventsInRange(startDate, endDate);

        INFO("Checking yearly recurring events for October 2025");
        // Should find the birthday event recurring in October
        CHECK(octoberEvents.size() >= 1);

        bool foundBirthday = false;
        for (auto event : octoberEvents) {
            if (event->summary == "Annual Birthday") {
                foundBirthday = true;
                CHECK(event->isRecurring == true);
                CHECK(event->rrule == "FREQ=YEARLY");
            }
        }
        CHECK(foundBirthday == true);

        MESSAGE("Found recurring birthday event in October 2025");
        CAPTURE(octoberEvents.size());
    }

    SUBCASE("Filter events with no end date") {
        MESSAGE("Testing events without end date");

        const char* no_end_calendar = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//No End Date//EN
BEGIN:VEVENT
UID:noend1@test.com
SUMMARY:Event with no end
DTSTART:20251115T140000Z
END:VEVENT
BEGIN:VEVENT
UID:noend2@test.com
SUMMARY:Another event with no end
DTSTART:20251120T100000Z
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(no_end_calendar);

        CHECK(result == true);
        CHECK(parser.getEventCount() == 2);

        // Get events for November 2025
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 10;  // November
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2025 - 1900;
        endTm.tm_mon = 10;  // November
        endTm.tm_mday = 30;
        time_t endDate = mktime(&endTm);

        std::vector<CalendarEvent*> events = parser.getEventsInRange(startDate, endDate);

        INFO("Events without end date should still be included");
        CHECK(events.size() == 2);

        CAPTURE(events.size());
    }

    SUBCASE("Empty date range returns no events") {
        MESSAGE("Testing empty date range");

        const char* calendar = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Test//EN
BEGIN:VEVENT
UID:test@test.com
SUMMARY:Event in 2025
DTSTART:20251015T100000Z
DTEND:20251015T110000Z
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(calendar);

        CHECK(result == true);

        // Get events for year 2024 (before the event)
        struct tm startTm = {0};
        startTm.tm_year = 2024 - 1900;
        startTm.tm_mon = 0;
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2024 - 1900;
        endTm.tm_mon = 11;
        endTm.tm_mday = 31;
        time_t endDate = mktime(&endTm);

        std::vector<CalendarEvent*> events = parser.getEventsInRange(startDate, endDate);

        INFO("Date range before all events should return empty");
        CHECK(events.size() == 0);

        CAPTURE(events.size());
    }

    SUBCASE("Parse and filter complex RRULE") {
        MESSAGE("Testing parsing of complex RRULE");

        const char* complex_rrule_cal = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Complex RRULE//EN
BEGIN:VEVENT
UID:weekly@test.com
SUMMARY:Weekly Meeting
DTSTART:20251001T100000Z
DTEND:20251001T110000Z
RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=10
END:VEVENT
BEGIN:VEVENT
UID:monthly@test.com
SUMMARY:Monthly Review
DTSTART:20251001T140000Z
DTEND:20251001T150000Z
RRULE:FREQ=MONTHLY;INTERVAL=1
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(complex_rrule_cal);

        CHECK(result == true);

        // Parse the RRULE of the first event
        CalendarEvent* weeklyEvent = parser.getEvent(0);
        REQUIRE(weeklyEvent != nullptr);

        String freq;
        int interval = 0;
        int count = 0;
        String until;

        bool parsed = parser.parseRRule(weeklyEvent->rrule, freq, interval, count, until);

        INFO("Checking RRULE parsing");
        CHECK(parsed == true);
        CHECK(freq == "WEEKLY");
        CHECK(interval == 2);
        CHECK(count == 10);
        CHECK(until.isEmpty());

        // Parse the monthly event RRULE
        CalendarEvent* monthlyEvent = parser.getEvent(1);
        REQUIRE(monthlyEvent != nullptr);

        parsed = parser.parseRRule(monthlyEvent->rrule, freq, interval, count, until);

        CHECK(parsed == true);
        CHECK(freq == "MONTHLY");
        CHECK(interval == 1);

        MESSAGE("Complex RRULE parsing successful");
        CAPTURE(freq);
        CAPTURE(interval);
        CAPTURE(count);
    }

    SUBCASE("Sort events by start time") {
        MESSAGE("Testing event sorting by start time");

        const char* unsorted_calendar = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//Unsorted Events//EN
BEGIN:VEVENT
UID:event3@test.com
SUMMARY:Third Event
DTSTART:20251020T150000Z
DTEND:20251020T160000Z
END:VEVENT
BEGIN:VEVENT
UID:event1@test.com
SUMMARY:First Event
DTSTART:20251020T090000Z
DTEND:20251020T100000Z
END:VEVENT
BEGIN:VEVENT
UID:event2@test.com
SUMMARY:Second Event
DTSTART:20251020T120000Z
DTEND:20251020T130000Z
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(unsorted_calendar);

        CHECK(result == true);

        // Get all events for October 2025
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 9;  // October
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2025 - 1900;
        endTm.tm_mon = 9;  // October
        endTm.tm_mday = 31;
        time_t endDate = mktime(&endTm);

        std::vector<CalendarEvent*> sortedEvents = parser.getEventsInRange(startDate, endDate);

        INFO("Events should be sorted by start time");
        CHECK(sortedEvents.size() == 3);

        if (sortedEvents.size() == 3) {
            CHECK(sortedEvents[0]->summary == "First Event");
            CHECK(sortedEvents[1]->summary == "Second Event");
            CHECK(sortedEvents[2]->summary == "Third Event");
        }

        MESSAGE("Events are properly sorted by start time");
        CAPTURE(sortedEvents.size());
    }

    SUBCASE("Swiss holidays calendar - parsing and filtering") {
        MESSAGE("Testing Swiss holidays calendar with specific events");

        const char* swiss_holidays_ics = R"(BEGIN:VCALENDAR
PRODID:-//Google Inc//Google Calendar 70.9054//EN
VERSION:2.0
CALSCALE:GREGORIAN
METHOD:PUBLISH
X-WR-CALNAME:Festività in Svizzera
X-WR-TIMEZONE:UTC
X-WR-CALDESC:Festività e ricorrenze in Svizzera
BEGIN:VEVENT
DTSTART;VALUE=DATE:20240801
DTEND;VALUE=DATE:20240802
DTSTAMP:20251020T112155Z
UID:20240801_gliktiq6ch854mkprr2h33njck@google.com
CLASS:PUBLIC
CREATED:20240517T100942Z
DESCRIPTION:Festività pubblica
LAST-MODIFIED:20240517T100942Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:Festa nazionale della Svizzera
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20241225
DTEND;VALUE=DATE:20241226
DTSTAMP:20251020T112155Z
UID:20241225_q60g8s1blmodfba6o7q4k4acnk@google.com
CLASS:PUBLIC
CREATED:20240517T100942Z
DESCRIPTION:Festa nazionale in Zurigo\, Berna\, Lucerna\, Uri\, Svitto\, Ob
 valdo\, Nidvaldo\, Glarona\, Zugo\, Friburgo\, Soletta\, Basilea Città\, Ba
 silea Campagna\, Sciaffusa\, Appenzello Esterno\, Appenzello Interno\, San
 Gallo\, Argovia\, Grigioni\, Turgovia\, Ticino\, Vaud\, Vallese\, Neuchâtel
 \, Ginevra\, Giura
LAST-MODIFIED:20240517T100942Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:Giorno di Natale (festività regionale)
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251026
DTEND;VALUE=DATE:20251027
DTSTAMP:20251020T112155Z
UID:20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com
CLASS:PUBLIC
CREATED:20240517T100942Z
DESCRIPTION:Ricorrenza\nPer nascondere le ricorrenze\, vai alle impostazion
 i di Google Calendar > Festività in Svizzera
LAST-MODIFIED:20240517T100942Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:l'ora legale termina
TRANSP:TRANSPARENT
END:VEVENT
BEGIN:VEVENT
DTSTART;VALUE=DATE:20251101
DTEND;VALUE=DATE:20251102
DTSTAMP:20251020T112155Z
UID:20251101_c4vhq6lk2utll5mjc593ea3b98@google.com
CLASS:PUBLIC
CREATED:20240517T101231Z
DESCRIPTION:Festa nazionale in Argovia\, Appenzello Interno\, Friburgo\, Gl
 arona\, Giura\, Lucerna\, Nidvaldo\, Obvaldo\, San Gallo\, Soletta\, Svitto
 \, Ticino\, Uri\, Vallese\, Zugo
LAST-MODIFIED:20240517T101231Z
SEQUENCE:0
STATUS:CONFIRMED
SUMMARY:Ognissanti (festività regionale)
TRANSP:TRANSPARENT
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(swiss_holidays_ics);

        INFO("Verifying Swiss holidays calendar loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);
        CHECK(parser.getCalendarName() == "Festività in Svizzera");
        CHECK(parser.getTimezone() == "UTC");

        INFO("Verifying all 4 events were parsed correctly");
        CHECK(parser.getEventCount() == 4);

        // Verify each event's details
        CalendarEvent* event1 = parser.getEvent(0);
        REQUIRE(event1 != nullptr);
        CHECK(event1->dtStart == "20240801");
        CHECK(event1->uid == "20240801_gliktiq6ch854mkprr2h33njck@google.com");
        CHECK(event1->summary == "Festa nazionale della Svizzera");
        CHECK(event1->allDay == true);

        CalendarEvent* event2 = parser.getEvent(1);
        REQUIRE(event2 != nullptr);
        CHECK(event2->dtStart == "20241225");
        CHECK(event2->uid == "20241225_q60g8s1blmodfba6o7q4k4acnk@google.com");
        CHECK(event2->summary == "Giorno di Natale (festività regionale)");
        CHECK(event2->allDay == true);

        CalendarEvent* event3 = parser.getEvent(2);
        REQUIRE(event3 != nullptr);
        CHECK(event3->dtStart == "20251026");
        CHECK(event3->uid == "20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com");
        CHECK(event3->summary == "l'ora legale termina");
        CHECK(event3->allDay == true);

        CalendarEvent* event4 = parser.getEvent(3);
        REQUIRE(event4 != nullptr);
        CHECK(event4->dtStart == "20251101");
        CHECK(event4->uid == "20251101_c4vhq6lk2utll5mjc593ea3b98@google.com");
        CHECK(event4->summary == "Ognissanti (festività regionale)");
        CHECK(event4->allDay == true);

        MESSAGE("All Swiss holiday events parsed correctly:");
        MESSAGE("  DATE START 2024/08/01 -> UID 20240801_gliktiq6ch854mkprr2h33njck@google.com -> SUMMARY Festa nazionale della Svizzera");
        MESSAGE("  DATE START 2024/12/25 -> UID 20241225_q60g8s1blmodfba6o7q4k4acnk@google.com -> SUMMARY Giorno di Natale (festività regionale)");
        MESSAGE("  DATE START 2025/10/26 -> UID 20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com -> SUMMARY l'ora legale termina");
        MESSAGE("  DATE START 2025/11/01 -> UID 20251101_c4vhq6lk2utll5mjc593ea3b98@google.com -> SUMMARY Ognissanti (festività regionale)");

        // Test date range filtering for October 2025
        INFO("Testing date range filtering for October 2025");

        // Create date range for October 2025 (2025/10/01 to 2025/10/31)
        struct tm startTm = {0};
        startTm.tm_year = 2025 - 1900;
        startTm.tm_mon = 9;  // October (0-based)
        startTm.tm_mday = 1;
        time_t startDate = mktime(&startTm);

        struct tm endTm = {0};
        endTm.tm_year = 2025 - 1900;
        endTm.tm_mon = 9;  // October
        endTm.tm_mday = 31;
        endTm.tm_hour = 23;
        endTm.tm_min = 59;
        endTm.tm_sec = 59;
        time_t endDate = mktime(&endTm);

        std::vector<CalendarEvent*> octoberEvents = parser.getEventsInRange(startDate, endDate);

        INFO("Checking that exactly 1 event is returned for October 2025");
        CHECK(octoberEvents.size() == 1);

        if (octoberEvents.size() == 1) {
            CalendarEvent* octoberEvent = octoberEvents[0];
            CHECK(octoberEvent->uid == "20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com");
            CHECK(octoberEvent->summary == "l'ora legale termina");
            CHECK(octoberEvent->dtStart == "20251026");

            MESSAGE("Correctly filtered October 2025 event:");
            MESSAGE("  UID: 20251026_grfn4rd0sdpps74hc9o6o6lgic@google.com");
            MESSAGE("  Summary: l'ora legale termina");
            MESSAGE("  Date: 2025/10/26");
        }

        // Additional test: Verify November 2025 returns only the Ognissanti event
        INFO("Testing date range filtering for November 2025");

        struct tm novStartTm = {0};
        novStartTm.tm_year = 2025 - 1900;
        novStartTm.tm_mon = 10;  // November (0-based)
        novStartTm.tm_mday = 1;
        time_t novStartDate = mktime(&novStartTm);

        struct tm novEndTm = {0};
        novEndTm.tm_year = 2025 - 1900;
        novEndTm.tm_mon = 10;  // November
        novEndTm.tm_mday = 30;
        novEndTm.tm_hour = 23;
        novEndTm.tm_min = 59;
        novEndTm.tm_sec = 59;
        time_t novEndDate = mktime(&novEndTm);

        std::vector<CalendarEvent*> novemberEvents = parser.getEventsInRange(novStartDate, novEndDate);

        CHECK(novemberEvents.size() == 1);
        if (novemberEvents.size() == 1) {
            CHECK(novemberEvents[0]->uid == "20251101_c4vhq6lk2utll5mjc593ea3b98@google.com");
            CHECK(novemberEvents[0]->summary == "Ognissanti (festività regionale)");
        }

        // Test that 2024 events are not returned when filtering for 2025
        INFO("Testing that 2024 events are not returned for 2025 date range");

        struct tm year2025StartTm = {0};
        year2025StartTm.tm_year = 2025 - 1900;
        year2025StartTm.tm_mon = 0;  // January
        year2025StartTm.tm_mday = 1;
        time_t year2025Start = mktime(&year2025StartTm);

        struct tm year2025EndTm = {0};
        year2025EndTm.tm_year = 2025 - 1900;
        year2025EndTm.tm_mon = 11;  // December
        year2025EndTm.tm_mday = 31;
        time_t year2025End = mktime(&year2025EndTm);

        std::vector<CalendarEvent*> year2025Events = parser.getEventsInRange(year2025Start, year2025End);

        // Should return only the 2 events from 2025 (October and November)
        CHECK(year2025Events.size() == 2);

        CAPTURE(parser.getEventCount());
        CAPTURE(octoberEvents.size());
        CAPTURE(novemberEvents.size());
        CAPTURE(year2025Events.size());
    }

    SUBCASE("RFC 5545 DTSTART date-time formats") {
        MESSAGE("Testing all RFC 5545 date-time formats for DTSTART");
        MESSAGE("Reference: https://www.rfc-editor.org/rfc/rfc5545#page-31");

        const char* datetime_formats_ics = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//RFC 5545 DateTime Formats//EN
CALSCALE:GREGORIAN
BEGIN:VEVENT
UID:local-time@test.com
SUMMARY:Local Time Event
DTSTART:19970714T133000
DTEND:19970714T143000
DESCRIPTION:Local time format without timezone indicator
END:VEVENT
BEGIN:VEVENT
UID:utc-time@test.com
SUMMARY:UTC Time Event
DTSTART:19970714T173000Z
DTEND:19970714T183000Z
DESCRIPTION:UTC time format with Z suffix
END:VEVENT
BEGIN:VEVENT
UID:timezone-ref@test.com
SUMMARY:Timezone Reference Event
DTSTART;TZID=America/New_York:19970714T133000
DTEND;TZID=America/New_York:19970714T143000
DESCRIPTION:Local time with timezone reference
END:VEVENT
BEGIN:VEVENT
UID:date-only@test.com
SUMMARY:Date Only Event (All-Day)
DTSTART;VALUE=DATE:19970714
DTEND;VALUE=DATE:19970715
DESCRIPTION:Date only format for all-day events
END:VEVENT
BEGIN:VEVENT
UID:floating-time@test.com
SUMMARY:Floating Time Event
DTSTART:20251225T120000
DTEND:20251225T140000
DESCRIPTION:Floating time (no timezone, interpreted as local)
END:VEVENT
BEGIN:VEVENT
UID:utc-with-seconds@test.com
SUMMARY:UTC with Seconds
DTSTART:20251231T235959Z
DTEND:20260101T000001Z
DESCRIPTION:UTC time with seconds precision
END:VEVENT
BEGIN:VEVENT
UID:timezone-europe@test.com
SUMMARY:European Timezone Event
DTSTART;TZID=Europe/Berlin:20250615T140000
DTEND;TZID=Europe/Berlin:20250615T160000
DESCRIPTION:Event with European timezone
END:VEVENT
BEGIN:VEVENT
UID:timezone-asia@test.com
SUMMARY:Asian Timezone Event
DTSTART;TZID=Asia/Tokyo:20250815T100000
DTEND;TZID=Asia/Tokyo:20250815T110000
DESCRIPTION:Event with Asian timezone
END:VEVENT
BEGIN:VEVENT
UID:value-datetime@test.com
SUMMARY:Explicit DateTime Value
DTSTART;VALUE=DATE-TIME:20250501T093000
DTEND;VALUE=DATE-TIME:20250501T103000
DESCRIPTION:Explicitly specified as DATE-TIME value type
END:VEVENT
BEGIN:VEVENT
UID:compact-date@test.com
SUMMARY:Compact Date Format
DTSTART:20250301T0900
DTEND:20250301T1000
DESCRIPTION:Compact format without seconds
END:VEVENT
END:VCALENDAR)";

        ICSParser parser;
        bool result = parser.loadFromString(datetime_formats_ics);

        INFO("Verifying calendar loaded successfully");
        CHECK(result == true);
        CHECK(parser.isValid() == true);

        INFO("Verifying all date-time format events were parsed");
        CHECK(parser.getEventCount() == 10);

        // Test 1: Local time (19970714T133000)
        CalendarEvent* localTimeEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "local-time@test.com") {
                localTimeEvent = evt;
                break;
            }
        }
        REQUIRE(localTimeEvent != nullptr);
        CHECK(localTimeEvent->dtStart == "19970714T133000");
        CHECK(localTimeEvent->allDay == false);
        MESSAGE("✓ Local time format: 19970714T133000");

        // Test 2: UTC time (19970714T173000Z)
        CalendarEvent* utcTimeEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "utc-time@test.com") {
                utcTimeEvent = evt;
                break;
            }
        }
        REQUIRE(utcTimeEvent != nullptr);
        CHECK(utcTimeEvent->dtStart == "19970714T173000Z");
        CHECK(utcTimeEvent->dtEnd == "19970714T183000Z");
        CHECK(utcTimeEvent->allDay == false);
        MESSAGE("✓ UTC time format: 19970714T173000Z");

        // Test 3: Timezone reference (TZID=America/New_York:19970714T133000)
        CalendarEvent* tzRefEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "timezone-ref@test.com") {
                tzRefEvent = evt;
                break;
            }
        }
        REQUIRE(tzRefEvent != nullptr);
        CHECK(tzRefEvent->dtStart == "19970714T133000");
        CHECK(tzRefEvent->timezone == "America/New_York");
        CHECK(tzRefEvent->allDay == false);
        MESSAGE("✓ Timezone reference: TZID=America/New_York:19970714T133000");

        // Test 4: Date only (VALUE=DATE:19970714)
        CalendarEvent* dateOnlyEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "date-only@test.com") {
                dateOnlyEvent = evt;
                break;
            }
        }
        REQUIRE(dateOnlyEvent != nullptr);
        CHECK(dateOnlyEvent->dtStart == "19970714");
        CHECK(dateOnlyEvent->allDay == true);
        MESSAGE("✓ Date only (all-day): VALUE=DATE:19970714");

        // Test 5: Floating time (20251225T120000)
        CalendarEvent* floatingTimeEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "floating-time@test.com") {
                floatingTimeEvent = evt;
                break;
            }
        }
        REQUIRE(floatingTimeEvent != nullptr);
        CHECK(floatingTimeEvent->dtStart == "20251225T120000");
        CHECK(floatingTimeEvent->allDay == false);
        MESSAGE("✓ Floating time: 20251225T120000");

        // Test 6: UTC with seconds (20251231T235959Z)
        CalendarEvent* utcSecondsEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "utc-with-seconds@test.com") {
                utcSecondsEvent = evt;
                break;
            }
        }
        REQUIRE(utcSecondsEvent != nullptr);
        CHECK(utcSecondsEvent->dtStart == "20251231T235959Z");
        CHECK(utcSecondsEvent->dtEnd == "20260101T000001Z");
        MESSAGE("✓ UTC with seconds: 20251231T235959Z");

        // Test 7: European timezone
        CalendarEvent* europeEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "timezone-europe@test.com") {
                europeEvent = evt;
                break;
            }
        }
        REQUIRE(europeEvent != nullptr);
        CHECK(europeEvent->dtStart == "20250615T140000");
        CHECK(europeEvent->timezone == "Europe/Berlin");
        MESSAGE("✓ European timezone: TZID=Europe/Berlin:20250615T140000");

        // Test 8: Asian timezone
        CalendarEvent* asiaEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "timezone-asia@test.com") {
                asiaEvent = evt;
                break;
            }
        }
        REQUIRE(asiaEvent != nullptr);
        CHECK(asiaEvent->dtStart == "20250815T100000");
        CHECK(asiaEvent->timezone == "Asia/Tokyo");
        MESSAGE("✓ Asian timezone: TZID=Asia/Tokyo:20250815T100000");

        // Test 9: Explicit DATE-TIME value
        CalendarEvent* explicitDateTimeEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "value-datetime@test.com") {
                explicitDateTimeEvent = evt;
                break;
            }
        }
        REQUIRE(explicitDateTimeEvent != nullptr);
        CHECK(explicitDateTimeEvent->dtStart == "20250501T093000");
        CHECK(explicitDateTimeEvent->allDay == false);
        MESSAGE("✓ Explicit DATE-TIME: VALUE=DATE-TIME:20250501T093000");

        // Test 10: Compact format
        CalendarEvent* compactEvent = nullptr;
        for (size_t i = 0; i < parser.getEventCount(); i++) {
            CalendarEvent* evt = parser.getEvent(i);
            if (evt && evt->uid == "compact-date@test.com") {
                compactEvent = evt;
                break;
            }
        }
        REQUIRE(compactEvent != nullptr);
        CHECK(compactEvent->dtStart == "20250301T0900");
        MESSAGE("✓ Compact format: 20250301T0900");

        MESSAGE("\nAll RFC 5545 date-time formats parsed successfully:");
        MESSAGE("  1. Local time: DTSTART:19970714T133000");
        MESSAGE("  2. UTC time: DTSTART:19970714T173000Z");
        MESSAGE("  3. Timezone ref: DTSTART;TZID=America/New_York:19970714T133000");
        MESSAGE("  4. Date only: DTSTART;VALUE=DATE:19970714");
        MESSAGE("  5. Floating time: DTSTART:20251225T120000");
        MESSAGE("  6. UTC with seconds: DTSTART:20251231T235959Z");
        MESSAGE("  7. European TZ: DTSTART;TZID=Europe/Berlin:20250615T140000");
        MESSAGE("  8. Asian TZ: DTSTART;TZID=Asia/Tokyo:20250815T100000");
        MESSAGE("  9. Explicit DATE-TIME: DTSTART;VALUE=DATE-TIME:20250501T093000");
        MESSAGE(" 10. Compact format: DTSTART:20250301T0900");

        CAPTURE(parser.getEventCount());
    }
}