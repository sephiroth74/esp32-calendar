// Test PSRAM usage in ICS parser
// This test verifies that events are allocated in PSRAM when available

#include <Arduino.h>
#include <unity.h>
#include "ics_parser.h"

const char* TEST_ICS = R"(BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//Test//PSRAM Test//EN
BEGIN:VEVENT
UID:event1@test.com
SUMMARY:Test Event 1
DTSTART:20250101T100000Z
DTEND:20250101T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event2@test.com
SUMMARY:Test Event 2
DTSTART:20250102T100000Z
DTEND:20250102T110000Z
END:VEVENT
BEGIN:VEVENT
UID:event3@test.com
SUMMARY:Test Event 3
DTSTART:20250103T100000Z
DTEND:20250103T110000Z
END:VEVENT
END:VCALENDAR)";

void test_psram_allocation() {
    Serial.println("\n=== Testing PSRAM Allocation ===");

#ifdef BOARD_HAS_PSRAM
    size_t psramBefore = ESP.getFreePsram();
    Serial.printf("Free PSRAM before: %d KB\n", psramBefore / 1024);
#endif

    uint32_t heapBefore = ESP.getFreeHeap();
    Serial.printf("Free heap before: %d KB\n", heapBefore / 1024);

    ICSParser parser;
    parser.setDebug(true);

    bool result = parser.loadFromString(TEST_ICS);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, parser.getEventCount());

#ifdef BOARD_HAS_PSRAM
    size_t psramAfter = ESP.getFreePsram();
    size_t psramUsed = psramBefore - psramAfter;
    Serial.printf("Free PSRAM after: %d KB\n", psramAfter / 1024);
    Serial.printf("PSRAM used by events: %d bytes\n", psramUsed);

    // Each CalendarEvent should use approximately sizeof(CalendarEvent) bytes
    // With 3 events, we expect some PSRAM usage
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, psramUsed,
        "PSRAM should be used for event allocation");
#endif

    uint32_t heapAfter = ESP.getFreeHeap();
    Serial.printf("Free heap after: %d KB\n", heapAfter / 1024);

    // Print memory info
    parser.printMemoryInfo();

    Serial.println("✓ PSRAM allocation test completed");
}

void test_large_calendar_psram() {
    Serial.println("\n=== Testing Large Calendar in PSRAM ===");

    // Create a large calendar with many events
    String largeICS = "BEGIN:VCALENDAR\nVERSION:2.0\nPRODID:-//Test//Large Calendar//EN\n";

    // Add 100 events
    for (int i = 0; i < 100; i++) {
        largeICS += "BEGIN:VEVENT\n";
        largeICS += "UID:event" + String(i) + "@test.com\n";
        largeICS += "SUMMARY:Test Event " + String(i) + "\n";
        largeICS += "DTSTART:2025" + String(i % 12 + 1, DEC) + "01T100000Z\n";
        largeICS += "DTEND:2025" + String(i % 12 + 1, DEC) + "01T110000Z\n";
        largeICS += "END:VEVENT\n";
    }
    largeICS += "END:VCALENDAR";

#ifdef BOARD_HAS_PSRAM
    size_t psramBefore = ESP.getFreePsram();
#endif
    uint32_t heapBefore = ESP.getFreeHeap();

    ICSParser parser;
    bool result = parser.loadFromString(largeICS);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(100, parser.getEventCount());

#ifdef BOARD_HAS_PSRAM
    size_t psramAfter = ESP.getFreePsram();
    size_t psramUsed = psramBefore - psramAfter;

    Serial.printf("PSRAM used for 100 events: %d bytes\n", psramUsed);
    Serial.printf("Average per event: %d bytes\n", psramUsed / 100);

    TEST_ASSERT_GREATER_THAN_MESSAGE(0, psramUsed,
        "Large calendar should use PSRAM");
#else
    uint32_t heapAfter = ESP.getFreeHeap();
    uint32_t heapUsed = heapBefore - heapAfter;
    Serial.printf("Heap used for 100 events: %d bytes\n", heapUsed);
    Serial.printf("Average per event: %d bytes\n", heapUsed / 100);
#endif

    Serial.println("✓ Large calendar test completed");
}

void test_memory_cleanup() {
    Serial.println("\n=== Testing Memory Cleanup ===");

#ifdef BOARD_HAS_PSRAM
    size_t psramInitial = ESP.getFreePsram();
#endif
    uint32_t heapInitial = ESP.getFreeHeap();

    {
        ICSParser parser;
        parser.loadFromString(TEST_ICS);
        TEST_ASSERT_EQUAL_INT(3, parser.getEventCount());

        // Parser will be destroyed here
    }

    // After parser is destroyed, memory should be reclaimed
#ifdef BOARD_HAS_PSRAM
    size_t psramFinal = ESP.getFreePsram();
    int32_t psramLeak = psramInitial - psramFinal;
    Serial.printf("PSRAM leak: %d bytes\n", psramLeak);

    TEST_ASSERT_LESS_THAN_INT32_MESSAGE(100, psramLeak,
        "PSRAM should be properly freed");
#endif

    uint32_t heapFinal = ESP.getFreeHeap();
    int32_t heapLeak = heapInitial - heapFinal;
    Serial.printf("Heap leak: %d bytes\n", heapLeak);

    TEST_ASSERT_LESS_THAN_INT32_MESSAGE(500, heapLeak,
        "Memory should be properly freed");

    Serial.println("✓ Memory cleanup verified");
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    Serial.println("\n\n========================================");
    Serial.println("ESP32 PSRAM Usage Tests");
    Serial.println("========================================");

    // Print system info
    Serial.printf("Total heap: %d KB\n", ESP.getHeapSize() / 1024);
    Serial.printf("Free heap: %d KB\n", ESP.getFreeHeap() / 1024);

#ifdef BOARD_HAS_PSRAM
    if (ESP.getPsramSize() > 0) {
        Serial.printf("PSRAM size: %d KB\n", ESP.getPsramSize() / 1024);
        Serial.printf("Free PSRAM: %d KB\n", ESP.getFreePsram() / 1024);
        Serial.println("PSRAM is AVAILABLE and will be used");
    } else {
        Serial.println("PSRAM is CONFIGURED but NOT DETECTED");
    }
#else
    Serial.println("PSRAM support is NOT ENABLED in build");
#endif

    UNITY_BEGIN();

    RUN_TEST(test_psram_allocation);
    RUN_TEST(test_large_calendar_psram);
    RUN_TEST(test_memory_cleanup);

    UNITY_END();
}

void loop() {
    // Nothing to do here
}