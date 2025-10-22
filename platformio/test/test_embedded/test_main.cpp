/**
 * Main Test Runner for ESP32 Embedded Tests
 * This file serves as the single entry point for all embedded tests
 */

#include <Arduino.h>
#include <unity.h>

// Forward declarations of test functions from other files
// test_config_loader_esp32.cpp tests
void test_littlefs_initialization();
void test_default_configuration();
void test_save_and_load_configuration();
void test_multiple_calendars();
void test_local_calendar_url();
void test_calendar_management();
void test_config_save();
void test_backward_compatibility();

// test_ics_parser_esp32.cpp tests
void test_ics_parser_initialization();
void test_parse_simple_calendar();
void test_parse_events();
void test_date_range_filtering();
void test_memory_management();
void test_load_from_file();
void test_rrule_parsing();
void test_folded_lines();
void test_timezone_support();
void test_clear_and_reload();

// test_psram_usage.cpp tests
void test_psram_allocation();
void test_large_calendar_psram();
void test_memory_cleanup();

// Setup functions from individual test files
void setup_config_tests();
void setup_ics_parser_tests();
void setup_psram_tests();

void setUp() {
    // Called before each test
}

void tearDown() {
    // Called after each test
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n\n========================================");
    Serial.println("ESP32-S3 Complete Test Suite");
    Serial.println("========================================");

    UNITY_BEGIN();

    // Configuration tests
    Serial.println("\n--- Configuration Loader Tests ---");
    setup_config_tests();
    RUN_TEST(test_littlefs_initialization);
    RUN_TEST(test_default_configuration);
    RUN_TEST(test_save_and_load_configuration);
    RUN_TEST(test_multiple_calendars);
    RUN_TEST(test_local_calendar_url);
    RUN_TEST(test_calendar_management);
    RUN_TEST(test_config_save);
    RUN_TEST(test_backward_compatibility);

    // ICS Parser tests
    Serial.println("\n--- ICS Parser Tests ---");
    setup_ics_parser_tests();
    RUN_TEST(test_ics_parser_initialization);
    RUN_TEST(test_parse_simple_calendar);
    RUN_TEST(test_parse_events);
    RUN_TEST(test_date_range_filtering);
    RUN_TEST(test_memory_management);
    RUN_TEST(test_load_from_file);
    RUN_TEST(test_rrule_parsing);
    RUN_TEST(test_folded_lines);
    RUN_TEST(test_timezone_support);
    RUN_TEST(test_clear_and_reload);

    // PSRAM tests
    Serial.println("\n--- PSRAM Usage Tests ---");
    setup_psram_tests();
    RUN_TEST(test_psram_allocation);
    RUN_TEST(test_large_calendar_psram);
    RUN_TEST(test_memory_cleanup);

    UNITY_END();

    Serial.println("\n========================================");
    Serial.println("All tests completed!");
    Serial.println("========================================");
}

void loop() {
    // Nothing to do here
}