// Remote test for configuration loading on ESP32
// This test runs on the actual device with real LittleFS

#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "littlefs_config.h"

// Test JSON content
const char* TEST_CONFIG_JSON = R"({
  "wifi": {
    "ssid": "TestNetwork",
    "password": "TestPassword123"
  },
  "location": {
    "latitude": 47.3769,
    "longitude": 8.5417,
    "name": "Zurich"
  },
  "calendars": [
    {
      "name": "Work Calendar",
      "color": "blue",
      "type": "ics",
      "url": "https://calendar.google.com/calendar/ical/work/basic.ics",
      "days_to_fetch": 14,
      "enabled": true
    },
    {
      "name": "Personal Calendar",
      "color": "green",
      "type": "ics",
      "url": "https://calendar.google.com/calendar/ical/personal/basic.ics",
      "days_to_fetch": 30,
      "enabled": true
    },
    {
      "name": "Birthdays",
      "type": "ics",
      "color": "red",
      "url": "local:///calendars/birthdays.ics",
      "days_to_fetch": 365,
      "enabled": false
    }
  ],
  "display": {
    "timezone": "CET-1CEST,M3.5.0,M10.5.0/3",
    "update_hour": 6
  }
})";

// setUp and tearDown are now defined in test_main.cpp

void test_littlefs_initialization() {
    Serial.println("\n=== Testing LittleFS Initialization ===");

    LittleFSConfig config;
    bool result = config.begin();

    TEST_ASSERT_TRUE_MESSAGE(result, "LittleFS should initialize successfully");
    Serial.println("✓ LittleFS initialized");
}

void test_default_configuration() {
    Serial.println("\n=== Testing Default Configuration ===");

    LittleFSConfig config;
    config.begin();

    // Check default values
    TEST_ASSERT_EQUAL_FLOAT(LOC_LATITUDE, config.getLatitude());
    TEST_ASSERT_EQUAL_FLOAT(LOC_LONGITUDE, config.getLongitude());
    TEST_ASSERT_EQUAL_STRING(DEFAULT_TIMEZONE, config.getTimezone().c_str());
    TEST_ASSERT_EQUAL_INT(DEFAULT_UPDATE_HOUR, config.getUpdateHour());

    // Should have one default calendar
    TEST_ASSERT_EQUAL_INT(1, config.getCalendars().size());
    TEST_ASSERT_TRUE(config.getCalendars()[0].enabled);

    Serial.println("✓ Default configuration verified");
}

void test_save_and_load_configuration() {
    Serial.println("\n=== Testing Save and Load Configuration ===");

    // First, write the test config file
    if (!LittleFS.begin(true)) {
        TEST_FAIL_MESSAGE("Failed to mount LittleFS");
        return;
    }

    // Write test configuration
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        TEST_FAIL_MESSAGE("Failed to create config file");
        return;
    }

    configFile.print(TEST_CONFIG_JSON);
    configFile.close();
    Serial.println("✓ Test config file written");

    // Now test loading it
    LittleFSConfig config;
    config.begin();

    bool loadResult = config.loadConfiguration();
    TEST_ASSERT_TRUE_MESSAGE(loadResult, "Configuration should load successfully");
    TEST_ASSERT_TRUE_MESSAGE(config.isValid(), "Configuration should be valid");

    // Verify WiFi settings
    TEST_ASSERT_EQUAL_STRING("TestNetwork", config.getWiFiSSID().c_str());
    TEST_ASSERT_EQUAL_STRING("TestPassword123", config.getWiFiPassword().c_str());

    // Verify location settings
    TEST_ASSERT_EQUAL_FLOAT(47.3769f, config.getLatitude());
    TEST_ASSERT_EQUAL_FLOAT(8.5417f, config.getLongitude());
    TEST_ASSERT_EQUAL_STRING("Zurich", config.getLocationName().c_str());

    // Verify display settings
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", config.getTimezone().c_str());
    TEST_ASSERT_EQUAL_INT(6, config.getUpdateHour());

    Serial.println("✓ Configuration loaded and verified");
}

void test_multiple_calendars() {
    Serial.println("\n=== Testing Multiple Calendars ===");

    // Load the test configuration (from previous test)
    LittleFSConfig config;
    config.begin();
    config.loadConfiguration();

    const auto& calendars = config.getCalendars();
    TEST_ASSERT_EQUAL_INT(3, calendars.size());

    // First calendar - Work
    TEST_ASSERT_EQUAL_STRING("Work Calendar", calendars[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("blue", calendars[0].color.c_str());
    TEST_ASSERT_EQUAL_INT(14, calendars[0].days_to_fetch);
    TEST_ASSERT_TRUE(calendars[0].enabled);

    // Second calendar - Personal
    TEST_ASSERT_EQUAL_STRING("Personal Calendar", calendars[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("green", calendars[1].color.c_str());
    TEST_ASSERT_EQUAL_INT(30, calendars[1].days_to_fetch);
    TEST_ASSERT_TRUE(calendars[1].enabled);

    // Third calendar - Birthdays (disabled)
    TEST_ASSERT_EQUAL_STRING("Birthdays", calendars[2].name.c_str());
    TEST_ASSERT_EQUAL_STRING("red", calendars[2].color.c_str());
    TEST_ASSERT_EQUAL_INT(365, calendars[2].days_to_fetch);
    TEST_ASSERT_FALSE(calendars[2].enabled);

    Serial.println("✓ All 3 calendars verified");
    Serial.println("  - Work: 14 days, enabled");
    Serial.println("  - Personal: 30 days, enabled");
    Serial.println("  - Birthdays: 365 days, disabled");
}

void test_local_calendar_url() {
    Serial.println("\n=== Testing Local Calendar URL ===");

    LittleFSConfig config;
    config.begin();
    config.loadConfiguration();

    const auto& calendars = config.getCalendars();

    // Find the birthday calendar (should be the third one)
    bool foundLocal = false;
    for (const auto& cal : calendars) {
        if (cal.url.indexOf("local://") == 0) {
            foundLocal = true;
            TEST_ASSERT_EQUAL_STRING("local:///calendars/birthdays.ics", cal.url.c_str());
            TEST_ASSERT_EQUAL_INT(365, cal.days_to_fetch);
            Serial.println("✓ Local calendar URL preserved correctly");
            break;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(foundLocal, "Should find local calendar URL");
}

void test_calendar_management() {
    Serial.println("\n=== Testing Calendar Management ===");

    LittleFSConfig config;
    config.begin();

    // Clear existing calendars
    config.clearCalendars();
    TEST_ASSERT_EQUAL_INT(0, config.getCalendars().size());

    // Add new calendars
    CalendarConfig cal1;
    cal1.name = "Test Calendar 1";
    cal1.url = "https://example.com/cal1.ics";
    cal1.color = "purple";
    cal1.days_to_fetch = 7;
    cal1.enabled = true;
    config.addCalendar(cal1);

    CalendarConfig cal2;
    cal2.name = "Test Calendar 2";
    cal2.url = "https://example.com/cal2.ics";
    cal2.color = "orange";
    cal2.days_to_fetch = 60;
    cal2.enabled = false;
    config.addCalendar(cal2);

    TEST_ASSERT_EQUAL_INT(2, config.getCalendars().size());
    TEST_ASSERT_EQUAL_STRING("Test Calendar 1", config.getCalendars()[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Test Calendar 2", config.getCalendars()[1].name.c_str());

    // Remove first calendar
    config.removeCalendar(0);
    TEST_ASSERT_EQUAL_INT(1, config.getCalendars().size());
    TEST_ASSERT_EQUAL_STRING("Test Calendar 2", config.getCalendars()[0].name.c_str());

    Serial.println("✓ Calendar management functions work correctly");
}

void test_config_save() {
    Serial.println("\n=== Testing Configuration Save ===");

    LittleFSConfig config;
    config.begin();

    // Set custom values
    config.setWiFiCredentials("NewSSID", "NewPassword");
    config.setLocation(52.5200f, 13.4050f, "Berlin");

    // Save configuration
    bool saveResult = config.saveConfiguration();
    TEST_ASSERT_TRUE_MESSAGE(saveResult, "Configuration should save successfully");

    // Load it back in a new instance
    LittleFSConfig config2;
    config2.begin();
    config2.loadConfiguration();

    TEST_ASSERT_EQUAL_STRING("NewSSID", config2.getWiFiSSID().c_str());
    TEST_ASSERT_EQUAL_STRING("NewPassword", config2.getWiFiPassword().c_str());
    TEST_ASSERT_EQUAL_FLOAT(52.5200f, config2.getLatitude());
    TEST_ASSERT_EQUAL_FLOAT(13.4050f, config2.getLongitude());
    TEST_ASSERT_EQUAL_STRING("Berlin", config2.getLocationName().c_str());

    Serial.println("✓ Configuration saved and reloaded successfully");
}

void test_backward_compatibility() {
    Serial.println("\n=== Testing Backward Compatibility ===");

    // Test old single calendar format
    const char* oldConfig = R"({
  "wifi": {
    "ssid": "OldSSID",
    "password": "OldPassword"
  },
  "calendar": {
    "url": "https://old.calendar.com/cal.ics",
    "days_to_fetch": 45
  },
  "display": {
    "timezone": "UTC",
    "update_hour": 3
  }
})";

    // Write old format config
    File configFile = LittleFS.open("/config.json", "w");
    configFile.print(oldConfig);
    configFile.close();

    LittleFSConfig config;
    config.begin();
    bool loadResult = config.loadConfiguration();

    TEST_ASSERT_TRUE(loadResult);
    TEST_ASSERT_EQUAL_INT(1, config.getCalendars().size());
    TEST_ASSERT_EQUAL_STRING("https://old.calendar.com/cal.ics", config.getCalendarUrl().c_str());
    TEST_ASSERT_EQUAL_INT(45, config.getCalendars()[0].days_to_fetch);

    Serial.println("✓ Backward compatibility with old format maintained");
}

// Setup function for configuration tests (called from test_main.cpp)
void setup_config_tests() {
    // Any specific setup needed for config tests
    Serial.println("Initializing configuration tests...");
}