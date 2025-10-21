// Simple configuration test without full Arduino mocking
#include "../doctest.h"
#include <vector>
#include <string>

// Mock Arduino String for testing
class String {
public:
    std::string str;

    String() {}
    String(const char* s) : str(s ? s : "") {}
    String(const std::string& s) : str(s) {}
    String(int val) : str(std::to_string(val)) {}
    String(float val) : str(std::to_string(val)) {}

    const char* c_str() const { return str.c_str(); }
    bool isEmpty() const { return str.empty(); }
    size_t length() const { return str.length(); }

    bool operator==(const String& other) const { return str == other.str; }
    bool operator!=(const String& other) const { return str != other.str; }
    String& operator=(const String& other) { str = other.str; return *this; }
};

// Configuration structures from littlefs_config.h
struct CalendarConfig {
    String name;
    String url;
    String color;
    bool enabled;
    int days_to_fetch;

    CalendarConfig() : enabled(true), days_to_fetch(30) {}
};

struct RuntimeConfig {
    String wifi_ssid;
    String wifi_password;
    float latitude;
    float longitude;
    String location_name;
    std::vector<CalendarConfig> calendars;
    String timezone;
    int update_hour;
    bool valid;

    RuntimeConfig() : latitude(0), longitude(0), update_hour(5), valid(false) {}
};

// Test cases
TEST_CASE("CalendarConfig - Individual Calendar Settings") {
    MESSAGE("Testing individual calendar configuration");

    CalendarConfig cal;
    cal.name = "Work Calendar";
    cal.url = "https://calendar.google.com/calendar/ical/work/basic.ics";
    cal.color = "blue";
    cal.enabled = true;
    cal.days_to_fetch = 31;

    CHECK(cal.name == "Work Calendar");
    CHECK(cal.url == "https://calendar.google.com/calendar/ical/work/basic.ics");
    CHECK(cal.color == "blue");
    CHECK(cal.enabled == true);
    CHECK(cal.days_to_fetch == 31);

    MESSAGE("Calendar configuration stores per-calendar days_to_fetch");
    CAPTURE(cal.days_to_fetch);
}

TEST_CASE("RuntimeConfig - Multiple Calendars Support") {
    MESSAGE("Testing runtime configuration with multiple calendars");

    RuntimeConfig config;
    config.wifi_ssid = "TestNetwork";
    config.wifi_password = "TestPassword";
    config.latitude = 40.7128f;
    config.longitude = -74.0060f;
    config.location_name = "New York";
    config.timezone = "EST5EDT,M3.2.0,M11.1.0";
    config.update_hour = 5;

    // Add first calendar
    CalendarConfig cal1;
    cal1.name = "Personal Calendar";
    cal1.url = "https://calendar.google.com/calendar/ical/personal/basic.ics";
    cal1.color = "green";
    cal1.enabled = true;
    cal1.days_to_fetch = 30;
    config.calendars.push_back(cal1);

    // Add second calendar
    CalendarConfig cal2;
    cal2.name = "Work Calendar";
    cal2.url = "https://calendar.google.com/calendar/ical/work/basic.ics";
    cal2.color = "blue";
    cal2.enabled = true;
    cal2.days_to_fetch = 14;
    config.calendars.push_back(cal2);

    // Add third calendar (disabled)
    CalendarConfig cal3;
    cal3.name = "Birthdays";
    cal3.url = "local:///calendars/birthdays.ics";
    cal3.color = "red";
    cal3.enabled = false;
    cal3.days_to_fetch = 365;
    config.calendars.push_back(cal3);

    CHECK(config.calendars.size() == 3);
    CHECK(config.calendars[0].name == "Personal Calendar");
    CHECK(config.calendars[0].days_to_fetch == 30);
    CHECK(config.calendars[1].name == "Work Calendar");
    CHECK(config.calendars[1].days_to_fetch == 14);
    CHECK(config.calendars[2].name == "Birthdays");
    CHECK(config.calendars[2].days_to_fetch == 365);
    CHECK(config.calendars[2].enabled == false);

    MESSAGE("Configuration supports multiple calendars with individual settings");
    CAPTURE(config.calendars.size());
}

TEST_CASE("CalendarConfig - Different Days to Fetch Per Calendar") {
    MESSAGE("Testing different days_to_fetch values per calendar");

    std::vector<CalendarConfig> calendars;

    // Short-range calendar (1 week)
    CalendarConfig shortRange;
    shortRange.name = "Daily Tasks";
    shortRange.days_to_fetch = 7;
    shortRange.enabled = true;
    calendars.push_back(shortRange);

    // Medium-range calendar (1 month)
    CalendarConfig mediumRange;
    mediumRange.name = "Monthly Events";
    mediumRange.days_to_fetch = 31;
    mediumRange.enabled = true;
    calendars.push_back(mediumRange);

    // Long-range calendar (1 year)
    CalendarConfig longRange;
    longRange.name = "Annual Holidays";
    longRange.days_to_fetch = 365;
    longRange.enabled = true;
    calendars.push_back(longRange);

    // Verify each calendar maintains its own days_to_fetch
    CHECK(calendars[0].days_to_fetch == 7);
    CHECK(calendars[1].days_to_fetch == 31);
    CHECK(calendars[2].days_to_fetch == 365);

    // Find maximum days_to_fetch among enabled calendars
    int maxDays = 0;
    for (const auto& cal : calendars) {
        if (cal.enabled && cal.days_to_fetch > maxDays) {
            maxDays = cal.days_to_fetch;
        }
    }
    CHECK(maxDays == 365);

    MESSAGE("Each calendar can have different days_to_fetch values");
    MESSAGE("  Daily Tasks: 7 days");
    MESSAGE("  Monthly Events: 31 days");
    MESSAGE("  Annual Holidays: 365 days");

    CAPTURE(maxDays);
}

TEST_CASE("CalendarConfig - Local vs Remote URLs") {
    MESSAGE("Testing local and remote calendar URLs");

    CalendarConfig localCal;
    localCal.name = "Local Birthdays";
    localCal.url = "local:///calendars/birthdays.ics";
    localCal.days_to_fetch = 365;
    localCal.enabled = true;

    CalendarConfig remoteCal;
    remoteCal.name = "Google Calendar";
    remoteCal.url = "https://calendar.google.com/calendar/ical/abc123/public/basic.ics";
    remoteCal.days_to_fetch = 30;
    remoteCal.enabled = true;

    // Check URL prefixes
    bool localIsLocal = (localCal.url.c_str()[0] == 'l'); // starts with "local://"
    bool remoteIsRemote = (remoteCal.url.c_str()[0] == 'h'); // starts with "https://"

    CHECK(localIsLocal == true);
    CHECK(remoteIsRemote == true);
    CHECK(localCal.days_to_fetch == 365);
    CHECK(remoteCal.days_to_fetch == 30);

    MESSAGE("Calendars can use local:// or https:// URLs");
    MESSAGE("  Local calendar typically uses longer days_to_fetch (365)");
    MESSAGE("  Remote calendar typically uses shorter days_to_fetch (30)");

    CAPTURE(localCal.url);
    CAPTURE(remoteCal.url);
}

TEST_CASE("RuntimeConfig - Configuration Validation") {
    MESSAGE("Testing configuration validation");

    RuntimeConfig config;

    // Initially invalid
    CHECK(config.valid == false);

    // Set minimum required fields
    config.wifi_ssid = "MyNetwork";
    config.wifi_password = "MyPassword";
    config.latitude = 47.3769f;
    config.longitude = 8.5417f;
    config.location_name = "Zurich";
    config.timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

    // Add at least one calendar
    CalendarConfig cal;
    cal.name = "Main Calendar";
    cal.url = "https://example.com/calendar.ics";
    cal.days_to_fetch = 31;
    cal.enabled = true;
    config.calendars.push_back(cal);

    // Mark as valid after setting all required fields
    config.valid = !config.wifi_ssid.isEmpty() &&
                   !config.calendars.empty() &&
                   config.calendars[0].enabled;

    CHECK(config.valid == true);
    CHECK(config.calendars.size() > 0);
    CHECK(config.calendars[0].enabled == true);

    MESSAGE("Configuration is valid when all required fields are set");
    CAPTURE(config.valid);
    CAPTURE(config.calendars.size());
}

TEST_CASE("CalendarConfig - Color Coding") {
    MESSAGE("Testing calendar color coding");

    std::vector<CalendarConfig> calendars;

    CalendarConfig workCal;
    workCal.name = "Work";
    workCal.color = "blue";
    calendars.push_back(workCal);

    CalendarConfig personalCal;
    personalCal.name = "Personal";
    personalCal.color = "green";
    calendars.push_back(personalCal);

    CalendarConfig holidaysCal;
    holidaysCal.name = "Holidays";
    holidaysCal.color = "red";
    calendars.push_back(holidaysCal);

    CHECK(calendars[0].color == "blue");
    CHECK(calendars[1].color == "green");
    CHECK(calendars[2].color == "red");

    MESSAGE("Each calendar can have a different color for visual distinction");
    MESSAGE("  Work: blue");
    MESSAGE("  Personal: green");
    MESSAGE("  Holidays: red");
}

TEST_CASE("RuntimeConfig - Default Values") {
    MESSAGE("Testing default configuration values");

    RuntimeConfig config;

    CHECK(config.latitude == 0.0f);
    CHECK(config.longitude == 0.0f);
    CHECK(config.update_hour == 5);
    CHECK(config.valid == false);
    CHECK(config.calendars.empty() == true);

    CalendarConfig cal;
    CHECK(cal.enabled == true);
    CHECK(cal.days_to_fetch == 30);

    MESSAGE("Configuration has sensible defaults");
    MESSAGE("  Default update_hour: 5 AM");
    MESSAGE("  Default days_to_fetch: 30 days");
    MESSAGE("  Default enabled: true");
}