#if defined(DEBUG_DISPLAY) && !defined(PIO_UNIT_TESTING)

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "config.h"
#include "display_manager.h"
#include "error_manager.h"
#include "wifi_manager.h"
#include "calendar_wrapper.h"
#include "calendar_display_adapter.h"
#include "weather_client.h"
#include "localization.h"
#include "version.h"
#include "littlefs_config.h"
#include <LittleFS.h>
#include <assets/fonts.h>
#include <assets/icons/icons_64x64.h>
#include <assets/icons/icons_48x48.h>
#include <assets/icons/icons_32x32.h>
#include <vector>
#include <Fonts/FreeMonoBold9pt7b.h>

// Display layout constants (for old layout compatibility)
#define CALENDAR_START_X 10
#define CONTENT_START_Y 60
#define EVENTS_START_X 400
#define OLD_EVENTS_WIDTH 390

// Global objects
DisplayManager display;
WiFiManager wifiManager;
CalendarManager* calendarManager = nullptr;
WeatherClient* weatherClient = nullptr;
LittleFSConfig configLoader;

// Function declarations
void showMenu();
void processCommand(String command);
void testFullScreenError();
void testWifiConnection();
void testIconDisplay();
void testBatteryDisplay();
void testCalendarFetch();
void testErrorLevels();
void testDithering();
void clearDisplay();
void testWeatherFetch();
void testLittleFSConfig();
void helloWorld();
void testDisplayCapabilities();
std::vector<CalendarEvent*> generateMockEvents();
WeatherData generateMockWeather();
int getBatteryPercentage(float voltage); // Battery percentage calculation

void onDeviceBusy(const void*)
{
    // Handle device busy state
    // Serial.println("Display is busy...");
}

    void setup()
{
    
    Serial.begin(115200);
    while (!Serial.isConnected()) {
        delay(10);
    }
    
    delay(1000); // Wait for serial connection

    esp_log_level_set("*", ESP_LOG_DEBUG); // Set ESP32 core log level to DEBUG

    Serial.println("\n\n=================================");
    Serial.println("ESP32 Calendar Display - DEBUG MODE");
    Serial.println("Version: " + getFullVersionString());
    Serial.println("=================================\n");

    // Initialize configuration from LittleFS for initial setup
    Serial.println("Initializing configuration...");
    if (!configLoader.begin()) {
        Serial.println("Failed to initialize LittleFS! Using built-in defaults.");
    } else {
        if (!configLoader.loadConfiguration()) {
            Serial.println("Failed to load configuration! Using built-in defaults.");
        } else {
            Serial.println("Configuration loaded successfully");
        }
    }

    pinMode(EPD_CS, OUTPUT);
    pinMode(EPD_RST, OUTPUT);
    pinMode(EPD_DC, OUTPUT);

    // Initialize display
    display.init();

    if (display.pages() > 1) {
        delay(100);
        Serial.print("pages = ");
        Serial.print(display.pages());
        Serial.print(" page height = ");
        Serial.println(display.pageHeight());
        delay(1000);
    } else {
        Serial.println("Single page display detected");
    }

    // display.display.epd2.setBusyCallback(onDeviceBusy); // Disable busy callback for debug
    display.clear();
    display.refresh(false);

    Serial.println("\nDEBUG_DISPLAY mode active - Normal operation bypassed");

    // Initialize WiFi client for calendar and weather testing
    WiFiClientSecure* wifiClient = new WiFiClientSecure();
    wifiClient->setInsecure();
    calendarManager = new CalendarManager();
    calendarManager->setDebug(true);
    weatherClient = new WeatherClient(wifiClient);

    showMenu();
}

void loop()
{
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        processCommand(command);
    }
    delay(100);
}

void showMenu()
{
    Serial.println("\n========== DEBUG MENU ==========");
    Serial.println("1  - Full Modern Calendar Demo");
    Serial.println("3  - Test Full Screen Errors");
    Serial.println("4  - Test WiFi Connection");
    Serial.println("5  - Test Icon Display (Inverted Bitmaps)");
    Serial.println("6  - Test Battery Status Display");
    Serial.println("7  - Test Calendar Fetch (Real)");
    Serial.println("8  - Test Error Levels");
    Serial.println("9  - Test Dithering Patterns");
    Serial.println("11 - Clear Display");
    Serial.println("13 - Test Weather Fetch (Real)");
    Serial.println("14 - Test LittleFS Configuration");
    Serial.println("15 - Test Display (hello world)");
    Serial.println("16 - Test Display Capabilities");
    Serial.println("h  - Show this menu");
    Serial.println("================================");
    Serial.print("\nEnter command: ");
}

void processCommand(String command)
{
    Serial.println(command);

    if (command == "1") {
        // Full demo with modern layout
        Serial.println("Running modern calendar demo...");

        // Get current time
        struct tm date;
        POPULATE_TM_DATE_TIME(date, 2025, 10, 16, 12, 30, 1, -1);
        mktime(&date);

        int currentDay = date.tm_mday;
        int currentMonth = date.tm_mon + 1;
        int currentYear = date.tm_year + 1900;

        Serial.println("Date set to: " + String(currentDay) + "/" + String(currentMonth) + "/" + String(currentYear));

        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%I:%M %p", &date);
        Serial.println("Time: " + String(timeStr));

        // Generate mock events
        Serial.println("Generating mock events...");
        std::vector<CalendarEvent*> mockEvents = generateMockEvents();
        Serial.println("Generated " + String(mockEvents.size()) + " mock events");

        // Generate mock weather data
        Serial.println("Generating mock weather...");
        WeatherData mockWeather = generateMockWeather();
        Serial.println("Weather generated: " + String(mockWeather.currentTemp) + "°C");

        // Display modern layout with weather
        Serial.println("Calling showModernCalendar...");
        display.setRotation(0); // Landscape
        display.showModernCalendar(mockEvents, currentDay, currentMonth, currentYear,
            String(timeStr), &mockWeather, true, -65, 4.2, 85);

        Serial.println("Modern demo displayed!");
    } else if (command == "3") {
        testFullScreenError();
    } else if (command == "4") {
        testWifiConnection();
    } else if (command == "5") {
        testIconDisplay();
    } else if (command == "6") {
        testBatteryDisplay();
    } else if (command == "7") {
        testCalendarFetch();
    } else if (command == "8") {
        testErrorLevels();
    } else if (command == "9") {
        testDithering();
    } else if (command == "11") {
        clearDisplay();
    } else if (command == "13") {
        testWeatherFetch();
    } else if (command == "14") {
        testLittleFSConfig();
    } else if (command == "15") {
        helloWorld();
    } else if (command == "16") {
        testDisplayCapabilities();
    } else if (command == "h") {
        showMenu();
    } else {
        Serial.println("Unknown command: " + command);
    }

    Serial.print("\nEnter command: ");
}

void testFullScreenError()
{
    Serial.println("\nTesting Full Screen Errors...");
    Serial.println("Select error type:");
    Serial.println("1 - WiFi Connection Error");
    Serial.println("2 - Calendar Fetch Error");
    Serial.println("3 - Battery Critical");
    Serial.println("4 - Memory Error");
    Serial.println("5 - NTP Sync Failed");

    while (!Serial.available())
        delay(10);
    String choice = Serial.readStringUntil('\n');
    choice.trim();

    ErrorInfo error;

    if (choice == "1") {
        error.code = ErrorCode::WIFI_CONNECTION_FAILED;
        error.message = LOC_ERROR_WIFI_CONNECTION_FAILED;
        error.level = ErrorLevel::ERROR;
        error.icon = ErrorIcon::WIFI;
        error.details = "SSID: TestNetwork";
    } else if (choice == "2") {
        error.code = ErrorCode::CALENDAR_FETCH_FAILED;
        error.message = LOC_ERROR_CALENDAR_FETCH_FAILED;
        error.level = ErrorLevel::WARNING;
        error.icon = ErrorIcon::CALENDAR;
        error.details = "HTTP Error 404";
    } else if (choice == "3") {
        error.code = ErrorCode::BATTERY_CRITICAL;
        error.message = LOC_ERROR_BATTERY_CRITICAL;
        error.level = ErrorLevel::CRITICAL;
        error.icon = ErrorIcon::BATTERY;
        error.details = "Battery: 5%";
    } else if (choice == "4") {
        error.code = ErrorCode::MEMORY_ALLOCATION_FAILED;
        error.message = LOC_ERROR_MEMORY_ALLOCATION_FAILED;
        error.level = ErrorLevel::CRITICAL;
        error.icon = ErrorIcon::MEMORY;
        error.details = "Free heap: 1024 bytes";
    } else if (choice == "5") {
        error.code = ErrorCode::NTP_SYNC_FAILED;
        error.message = LOC_ERROR_NTP_SYNC_FAILED;
        error.level = ErrorLevel::WARNING;
        error.icon = ErrorIcon::CLOCK;
        error.details = "pool.ntp.org";
    }

    display.showFullScreenError(error);
    Serial.println("Error screen displayed!");
}

void testWifiConnection()
{
    Serial.println("\n========================================");
    Serial.println("Testing WiFi Connection");
    Serial.println("========================================\n");

    // Create a test config loader and load configuration properly
    LittleFSConfig testConfigLoader;

    // Initialize LittleFS
    if (!testConfigLoader.begin()) {
        Serial.println("ERROR: Failed to initialize LittleFS!");
        return;
    }

    // Load configuration
    if (!testConfigLoader.loadConfiguration()) {
        Serial.println("ERROR: Failed to load configuration from LittleFS!");
        return;
    }

    // Display WiFi configuration from config.json
    Serial.println("--- WiFi Configuration from config.json ---");
    Serial.println("SSID: " + testConfigLoader.getWiFiSSID());
    Serial.println("Password: " + String(testConfigLoader.getWiFiPassword().isEmpty() ? "(no password)" : "********"));
    Serial.println("\nAttempting to connect...");

    bool connected = wifiManager.connect(testConfigLoader.getConfig());

    if (connected) {
        Serial.println("\n✓ WiFi connected successfully!");
        Serial.println("--- Connection Details ---");
        Serial.println("IP Address: " + WiFi.localIP().toString());
        Serial.println("Subnet Mask: " + WiFi.subnetMask().toString());
        Serial.println("Gateway: " + WiFi.gatewayIP().toString());
        Serial.println("DNS Server: " + WiFi.dnsIP().toString());
        Serial.println("MAC Address: " + WiFi.macAddress());
        Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
        Serial.println("Channel: " + String(WiFi.channel()));

        // Calculate signal quality percentage
        int rssi = WiFi.RSSI();
        int quality = 0;
        if (rssi >= -50)
            quality = 100;
        else if (rssi >= -60)
            quality = 90;
        else if (rssi >= -70)
            quality = 75;
        else if (rssi >= -80)
            quality = 60;
        else if (rssi >= -90)
            quality = 40;
        else
            quality = 20;

        Serial.println("Signal Quality: " + String(quality) + "%");

        // Display connection info on screen
        display.firstPage();
        do {
            display.clear();
            display.setFont(&Ubuntu_R_18pt8b);
            display.setCursor(50, 40);
            display.print("WiFi Connected");

            display.setFont(&Ubuntu_R_12pt8b);
            int y = 80;

            display.setCursor(50, y);
            display.print("SSID: " + testConfigLoader.getWiFiSSID());
            y += 25;

            display.setCursor(50, y);
            display.print("IP: " + WiFi.localIP().toString());
            y += 25;

            display.setCursor(50, y);
            display.print("Gateway: " + WiFi.gatewayIP().toString());
            y += 25;

            display.setCursor(50, y);
            display.print("DNS: " + WiFi.dnsIP().toString());
            y += 25;

            display.setCursor(50, y);
            display.print("RSSI: " + String(WiFi.RSSI()) + " dBm (" + String(quality) + "%)");
            y += 25;

            display.setCursor(50, y);
            display.print("Channel: " + String(WiFi.channel()));
            y += 25;

            display.setCursor(50, y);
            display.print("MAC: " + WiFi.macAddress());

        } while (display.nextPage());

        Serial.println("\n========================================");
        Serial.println("WiFi test complete - Connected!");
        Serial.println("========================================\n");

    } else {
        Serial.println("\n✗ WiFi connection failed!");
        Serial.println("Please check:");
        Serial.println("1. WiFi credentials in config.json are correct");
        Serial.println("2. WiFi network is available");
        Serial.println("3. Router is within range");

        ErrorInfo error;
        error.code = ErrorCode::WIFI_CONNECTION_FAILED;
        error.message = LOC_ERROR_WIFI_CONNECTION_FAILED;
        error.level = ErrorLevel::ERROR;
        error.icon = ErrorIcon::WIFI;
        error.details = "SSID: " + testConfigLoader.getWiFiSSID();
        display.showFullScreenError(error);

        Serial.println("\n========================================");
        Serial.println("WiFi test complete - Failed!");
        Serial.println("========================================\n");
    }
}

void testIconDisplay()
{
    Serial.println("\nTesting Icon Display with Inverted Bitmaps...");
    Serial.println("Drawing error and warning icons using actual bitmap files...");

    display.firstPage();
    do {
        display.clear();

        display.setFont(&Luna_ITC_Std_Bold12pt7b);
        display.setCursor(50, 30);
        display.print("Icon Test - Error & Warning Icons");

        // Draw 64x64 icons
        int x = 50;
        int y = 60;

        // Error icon (for WiFi errors)
        display.drawInvertedBitmap(x, y, error_icon_64x64, 64, 64, GxEPD_BLACK);
        display.setFont(&Ubuntu_R_9pt8b);
        display.setCursor(x, y + 75);
        display.print("Error 64");

        x += 100;

        // Warning icon (for other errors)
        display.drawInvertedBitmap(x, y, warning_icon_64x64, 64, 64, GxEPD_BLACK);
        display.setCursor(x, y + 75);
        display.print("Warning 64");

        // Draw weather icons
        x = 50;
        y = 160;

        display.setFont(&Luna_ITC_Std_Bold12pt7b);
        display.setCursor(x, y);
        display.print("Weather Icons (32x32):");

        y += 20;

        // Sunny day
        display.drawInvertedBitmap(x, y, wi_day_sunny_32x32, 32, 32, GxEPD_BLACK);
        display.setFont(&Ubuntu_R_9pt8b);
        display.setCursor(x, y + 40);
        display.print("Sun");

        x += 80;

        // Cloudy day
        display.drawInvertedBitmap(x, y, wi_day_cloudy_32x32, 32, 32, GxEPD_BLACK);
        display.setCursor(x, y + 40);
        display.print("Cloud");

        x += 80;

        // Rain
        display.drawInvertedBitmap(x, y, wi_rain_32x32, 32, 32, GxEPD_BLACK);
        display.setCursor(x, y + 40);
        display.print("Rain");

        x += 80;

        // Cloudy night
        display.drawInvertedBitmap(x, y, wi_night_cloudy_32x32, 32, 32, GxEPD_BLACK);
        display.setCursor(x, y + 40);
        display.print("Night");

        // Draw smaller icons
        x = 50;
        y = 250;

        display.setFont(&Luna_ITC_Std_Bold12pt7b);
        display.setCursor(x, y);
        display.print("48x48 Icons:");

        y += 20;

        // Error 48x48
        display.drawInvertedBitmap(x, y, error_icon_48x48, 48, 48, GxEPD_BLACK);
        display.setFont(&Ubuntu_R_9pt8b);
        display.setCursor(x, y + 55);
        display.print("Error");

        x += 80;

        // Warning 48x48
        display.drawInvertedBitmap(x, y, warning_icon_48x48, 48, 48, GxEPD_BLACK);
        display.setCursor(x, y + 55);
        display.print("Warning");

    } while (display.nextPage());

    Serial.println("Icon test with inverted bitmaps complete!");
}

// LiPo battery discharge curve lookup table (same as in main.cpp)
int getBatteryPercentageDebug(float voltage)
{
    const struct {
        float voltage;
        int percentage;
    } lipoTable[] = {
        { 4.20, 100 },
        { 4.15, 95 },
        { 4.11, 90 },
        { 4.08, 85 },
        { 4.02, 80 },
        { 3.98, 75 },
        { 3.95, 70 },
        { 3.91, 65 },
        { 3.87, 60 },
        { 3.85, 55 },
        { 3.84, 50 },
        { 3.82, 45 },
        { 3.80, 40 },
        { 3.79, 35 },
        { 3.77, 30 },
        { 3.75, 25 },
        { 3.73, 20 },
        { 3.71, 15 },
        { 3.69, 10 },
        { 3.60, 5 },
        { 3.50, 0 }
    };

    const int tableSize = sizeof(lipoTable) / sizeof(lipoTable[0]);

    if (voltage >= lipoTable[0].voltage)
        return lipoTable[0].percentage;
    if (voltage <= lipoTable[tableSize - 1].voltage)
        return lipoTable[tableSize - 1].percentage;

    for (int i = 0; i < tableSize - 1; i++) {
        if (voltage >= lipoTable[i + 1].voltage) {
            float v1 = lipoTable[i].voltage;
            float v2 = lipoTable[i + 1].voltage;
            int p1 = lipoTable[i].percentage;
            int p2 = lipoTable[i + 1].percentage;
            float ratio = (voltage - v2) / (v1 - v2);
            return p2 + (int)(ratio * (p1 - p2));
        }
    }
    return 0;
}

void testBatteryDisplay()
{
    Serial.println("\nTesting Battery Status - Live Monitor");
    Serial.println("========================================");
    Serial.println("Press any key to stop monitoring...\n");

    // Initialize ADC for battery monitoring
    pinMode(BATTERY_PIN, INPUT);
    analogReadResolution(12); // 12-bit resolution
    analogSetAttenuation(ADC_11db); // Full scale 0-3.3V

    // Display initial screen
    display.firstPage();
    do {
        display.clear();
        display.setFont(&Ubuntu_R_18pt8b);
        display.setCursor(50, 50);
        display.print("Battery Monitor Active");
        display.setFont(&Ubuntu_R_12pt8b);
        display.setCursor(50, 100);
        display.print("Check serial monitor for readings");
    } while (display.nextPage());

    // Monitor battery in a loop
    int loopCount = 0;
    while (!Serial.available()) {
        // Read ADC value
        int adcValue = analogRead(BATTERY_PIN);

        // Convert to voltage
        float measuredMillivolts = (adcValue / 4095.0) * 3300.0; // mV at ADC pin
        float batteryMillivolts = measuredMillivolts * BATTERY_VOLTAGE_DIVIDER; // mV at battery
        float batteryVoltage = batteryMillivolts / 1000.0; // Convert to volts

        // Calculate percentage using LiPo curve
        int batteryPercentage = getBatteryPercentageDebug(batteryVoltage);

        // Print detailed readings
        Serial.print("[");
        Serial.print(loopCount++);
        Serial.print("] ADC: ");
        Serial.print(adcValue);
        Serial.print(" | ADC mV: ");
        Serial.print(measuredMillivolts, 0);
        Serial.print(" | Battery mV: ");
        Serial.print(batteryMillivolts, 0);
        Serial.print(" | Battery V: ");
        Serial.print(batteryVoltage, 3);
        Serial.print(" | Percentage: ");
        Serial.print(batteryPercentage);
        Serial.print("%");

        // Add battery state description
        if (batteryPercentage >= 80) {
            Serial.print(" [Full]");
        } else if (batteryPercentage >= 60) {
            Serial.print(" [Good]");
        } else if (batteryPercentage >= 40) {
            Serial.print(" [Fair]");
        } else if (batteryPercentage >= 20) {
            Serial.print(" [Low]");
        } else if (batteryPercentage >= 10) {
            Serial.print(" [Critical]");
        } else {
            Serial.print(" [Empty]");
        }
        Serial.println();

        // Update display every 10 readings
        if (loopCount % 10 == 0) {
            display.firstPage();
            do {
                display.clear();

                // Title
                display.setFont(&Ubuntu_R_18pt8b);
                display.setCursor(50, 50);
                display.print("Battery Monitor");

                // Current readings
                display.setFont(&Ubuntu_R_24pt8b);
                display.setCursor(50, 120);
                display.print(String(batteryVoltage, 2) + "V");

                display.setCursor(250, 120);
                display.print(String(batteryPercentage) + "%");

                // Detailed info
                display.setFont(&Ubuntu_R_12pt8b);
                display.setCursor(50, 180);
                display.print("ADC: " + String(adcValue) + " (" + String(measuredMillivolts, 0) + " mV)");

                display.setCursor(50, 220);
                display.print("Battery: " + String(batteryMillivolts, 0) + " mV");

                // Draw battery status bar
                // Get current date and time for status bar
                time_t now;
                time(&now);
                struct tm* timeinfo = localtime(&now);
                int currentDay = timeinfo->tm_mday;
                int currentMonth = timeinfo->tm_mon + 1;
                int currentYear = timeinfo->tm_year + 1900;
                char timeStr[6];
                strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);

                display.drawStatusBar(true, -65, batteryVoltage, batteryPercentage,
                    currentDay, currentMonth, currentYear, String(timeStr));

            } while (display.nextPage());
        }

        delay(1000); // Update every second
    }

    // Clear serial buffer
    while (Serial.available()) {
        Serial.read();
    }

    Serial.println("\nBattery monitoring stopped.");
    Serial.println("========================================");
}

void testCalendarFetch()
{
    Serial.println("\nTesting Real Calendar Fetch...");
    Serial.println("========================================");

    // Show current system time BEFORE doing anything
    time_t nowBefore = time(nullptr);
    struct tm* tmBefore = localtime(&nowBefore);
    char timeStrBefore[64];
    strftime(timeStrBefore, sizeof(timeStrBefore), "%Y-%m-%d %H:%M:%S %Z", tmBefore);
    Serial.println("\n>>> SYSTEM TIME AT START <<<");
    Serial.println("  Current time: " + String(timeStrBefore));
    Serial.println("  Unix timestamp: " + String(nowBefore));
    Serial.println("  Year: " + String(tmBefore->tm_year + 1900));
    Serial.println("  Month: " + String(tmBefore->tm_mon + 1));
    Serial.println("  Day: " + String(tmBefore->tm_mday));

    // Create a test config loader and load configuration
    LittleFSConfig testConfigLoader;
    if (!testConfigLoader.begin()) {
        Serial.println("ERROR: Failed to initialize LittleFS!");
        return;
    }
    if (!testConfigLoader.loadConfiguration()) {
        Serial.println("ERROR: Failed to load configuration!");
        return;
    }

    // Connect to WiFi first if not connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to WiFi first...");
        if (!wifiManager.connect(testConfigLoader.getConfig())) {
            Serial.println("WiFi connection failed!");
            return;
        }
    }

    // Sync time with NTP for proper timezone conversion
    Serial.println("Syncing time with NTP server...");
    String timezone = testConfigLoader.getTimezone();
    setenv("TZ", timezone.c_str(), 1);
    tzset();

    // Try multiple NTP servers for better reliability
    configTime(0, 0, "pool.ntp.org", "0.europe.pool.ntp.org", "time.google.com");

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;  // Increased retry count

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
        Serial.print(".");
        delay(1000);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry < retry_count) {
        Serial.println("\nTime synchronized!");
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.println("Current time: " + String(timeStr));
    } else {
        Serial.println("\nError: Failed to sync time with NTP server!");
        Serial.println("Cannot continue without proper time sync.");

        // Parse compile date (__DATE__ format: "Oct 24 2024")
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char monthStr[4];
        int day, year, month = 0;
        sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);

        for (int i = 0; i < 12; i++) {
            if (strcmp(monthStr, months[i]) == 0) {
                month = i;
                break;
            }
        }

        // Set a reasonable default time based on compile date
        struct tm defaultTime = {0};
        defaultTime.tm_year = year - 1900;   // Years since 1900
        defaultTime.tm_mon = month;           // Month (0-based)
        defaultTime.tm_mday = day;            // Day of month
        defaultTime.tm_hour = 12;            // Noon
        defaultTime.tm_min = 0;
        defaultTime.tm_sec = 0;
        defaultTime.tm_isdst = -1;           // Let system determine DST

        time_t defaultTimestamp = mktime(&defaultTime);
        struct timeval tv = { .tv_sec = defaultTimestamp, .tv_usec = 0 };
        settimeofday(&tv, NULL);

        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &defaultTime);
        Serial.println("Using fallback date: " + String(timeStr) + " (compile date)");
        Serial.println("Note: Event today/tomorrow detection may be incorrect.");
    }

    // Load calendar configuration from LittleFS
    Serial.println("\nLoading calendar configuration...");
    RuntimeConfig config;
    if (!testConfigLoader.loadConfiguration()) {
        Serial.println("Failed to load configuration!");
        return;
    }
    config = testConfigLoader.getConfig();

    // Initialize calendar manager and load calendars
    calendarManager->loadFromConfig(config);
    calendarManager->printStatus();

    Serial.println("\nFetching calendar events from all configured calendars...");
    bool success = calendarManager->loadAll(false); // false = use cache if available

    // Get current time for date range
    time_t nowTime = time(nullptr);
    time_t endDate = nowTime + (31 * 86400); // Next 31 days

    Serial.println("\n=== CALENDAR FETCH RESULTS ===");

    // Print results per calendar
    size_t totalCalendars = calendarManager->getCalendarCount();
    Serial.println("Total calendars configured: " + String(totalCalendars));

    if (!success) {
        Serial.println("WARNING: Some calendars failed to load!");
    }

    // Collect events per calendar for display
    struct CalendarSummary {
        String name;
        String color;
        size_t eventCount;
        std::vector<CalendarEvent*> events;
    };
    std::vector<CalendarSummary> calendarSummaries;

    for (size_t i = 0; i < totalCalendars; i++) {
        CalendarWrapper* cal = calendarManager->getCalendar(i);
        if (cal && cal->isEnabled()) {
            CalendarSummary summary;
            summary.name = cal->getName();
            summary.color = cal->getColor();
            summary.events = cal->getEvents(nowTime, endDate);
            summary.eventCount = summary.events.size();

            Serial.println("\n--- Calendar: " + summary.name + " ---");
            Serial.println("  Color: " + summary.color);
            Serial.println("  Enabled: Yes");
            Serial.println("  Events in range: " + String(summary.eventCount));
            Serial.println("  Is Stale (cached): " + String(cal->isStale ? "Yes" : "No"));

            // Print first few events from this calendar
            for (size_t j = 0; j < summary.events.size() && j < 5; j++) {
                auto event = summary.events[j];
                Serial.println("    Event " + String(j+1) + ": " + event->summary);
                Serial.println("      Date: " + event->date + " " + event->startTimeStr);
            }
            if (summary.eventCount > 5) {
                Serial.println("    ... and " + String(summary.eventCount - 5) + " more events");
            }

            calendarSummaries.push_back(summary);
        }
    }

    // Calculate total events
    size_t totalEvents = 0;
    for (const auto& summary : calendarSummaries) {
        totalEvents += summary.eventCount;
    }
    Serial.println("\nTotal events across all calendars: " + String(totalEvents));

    // Display on screen - per-calendar with user prompts
    Serial.println("\n[DEBUG] Starting display rendering...");

    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);

    // Display each calendar separately
    for (size_t i = 0; i < calendarSummaries.size(); i++) {
        const CalendarSummary& summary = calendarSummaries[i];

        Serial.println("\n[DEBUG] Displaying calendar " + String(i+1) + "/" + String(calendarSummaries.size()) + ": " + summary.name);

        display.setFullWindow();
        display.firstPage();

        do {
            display.fillScreen(GxEPD_WHITE);

            // Title at top
            display.setFont(&Ubuntu_R_18pt8b);
            display.setCursor(20, 40);
            display.print("Calendar " + String(i+1) + "/" + String(calendarSummaries.size()));

            // Calendar name
            display.setFont(&Ubuntu_R_14pt8b);
            display.setCursor(20, 80);
            display.print(summary.name);

            // Color and event count
            display.setFont(&Ubuntu_R_12pt8b);
            display.setCursor(20, 110);
            display.print("Color: " + summary.color);

            display.setCursor(20, 135);
            display.print("Events: " + String(summary.eventCount));

            // List events
            int y = 170;
            int lineHeight = 25;
            display.setFont(&Ubuntu_R_9pt8b);

            for (size_t j = 0; j < summary.events.size() && j < 15; j++) {
                auto event = summary.events[j];

                // Event title
                display.setCursor(20, y);
                String eventTitle = event->summary;
                if (eventTitle.length() > 50) {
                    eventTitle = eventTitle.substring(0, 47) + "...";
                }
                display.print(String(j+1) + ". " + eventTitle);
                y += lineHeight;

                // Event date/time
                display.setCursor(40, y);
                display.print(event->date + " " + event->startTimeStr);
                y += lineHeight;

                // Check if we're running out of space
                if (y > display.height() - 50) {
                    display.setCursor(20, y);
                    display.print("... and " + String(summary.eventCount - j - 1) + " more events");
                    break;
                }
            }

            // Footer instruction
            display.setFont(&Ubuntu_R_9pt8b);
            display.setCursor(20, display.height() - 20);
            display.print("Press any key to continue...");

        } while (display.nextPage());

        Serial.println("[DEBUG] Display rendered for " + summary.name);

        // Wait for user input before continuing to next calendar
        if (i < calendarSummaries.size() - 1) {
            Serial.println("\nPress any key to continue to next calendar...");
            while (!Serial.available()) {
                delay(100);
            }
            // Clear serial buffer
            while (Serial.available()) {
                Serial.read();
            }
        }
    }

    Serial.println("\n[DEBUG] Display rendering complete!");
    Serial.println("\n========================================");
    Serial.println("Calendar fetch test complete!");
}

void testErrorLevels()
{
    Serial.println("\nTesting Error Levels...");

    display.firstPage();
    do {
        display.clear();

        display.setFont(&Ubuntu_R_18pt8b);
        display.setCursor(50, 50);
        display.print("Error Level Test");

        int y = 100;

        // Test each error level
        ErrorLevel levels[] = { ErrorLevel::INFO, ErrorLevel::WARNING,
            ErrorLevel::ERROR, ErrorLevel::CRITICAL };
        const char* levelNames[] = { "INFO", "WARNING", "ERROR", "CRITICAL" };

        for (int i = 0; i < 4; i++) {
            ErrorInfo error;
            error.level = levels[i];
            error.message = String("Test ") + levelNames[i] + " message";
            error.code = static_cast<ErrorCode>(100 + i);

            display.setFont(&Ubuntu_R_12pt8b);
            display.setCursor(50, y);
            display.print(levelNames[i]);

            // Draw appropriate icon for each level
            int iconX = 200;
            int iconSize = 48;

            switch (levels[i]) {
            case ErrorLevel::INFO:
                display.drawInfoIcon(iconX, y - 30, iconSize);
                break;
            case ErrorLevel::WARNING:
                display.drawWarningIcon(iconX, y - 30, iconSize);
                break;
            case ErrorLevel::ERROR:
            case ErrorLevel::CRITICAL:
                display.drawErrorIcon(iconX, y - 30, iconSize);
                break;
            }

            y += 80;
        }

    } while (display.nextPage());

    Serial.println("Error levels test complete!");
}

void testDithering()
{
    Serial.println("\nTesting Dithering Patterns...");

    display.firstPage();
    do {
        display.clear();

        display.setFont(&Ubuntu_R_18pt8b);
        display.setCursor(50, 50);
        display.print("Dithering Test");

        // Test different dithering levels
        float levels[] = { 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.75 };
        int x = 50;
        int y = 100;
        int boxSize = 80;

        for (int i = 0; i < 8; i++) {
            // Apply dithering to a box (white background, black foreground)
            display.applyDithering(x, y, boxSize, boxSize, GxEPD_WHITE, GxEPD_BLACK, levels[i]);

            // Draw label
            display.setFont(&Ubuntu_R_9pt8b);
            display.setCursor(x + 10, y + boxSize + 20);
            display.print(String(int(levels[i] * 100)) + "%");

            x += boxSize + 20;
            if (x > 650) {
                x = 50;
                y += boxSize + 40;
            }
        }

    } while (display.nextPage());

    Serial.println("Dithering test complete!");
}

void clearDisplay()
{
    Serial.println("\nClearing display...");
    display.clear();
    display.refresh(false);
    Serial.println("Display cleared!");
}

void testWeatherFetch()
{
    Serial.println("\nTesting Real Weather Fetch...");
    Serial.println("========================================");

    // Create a test config loader and load configuration
    LittleFSConfig testConfigLoader;
    if (!testConfigLoader.begin()) {
        Serial.println("ERROR: Failed to initialize LittleFS!");
        return;
    }
    if (!testConfigLoader.loadConfiguration()) {
        Serial.println("ERROR: Failed to load configuration!");
        return;
    }

    // Connect to WiFi first if not connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to WiFi first...");
        if (!wifiManager.connect(testConfigLoader.getConfig())) {
            Serial.println("WiFi connection failed!");
            return;
        }
    }

    // Set weather location from config
    weatherClient->setLocation(testConfigLoader.getLatitude(), testConfigLoader.getLongitude());

    // Fetch weather data
    Serial.println("Fetching weather data...");
    Serial.println("Location: Latitude " + String(testConfigLoader.getLatitude(), 6) + ", Longitude " + String(testConfigLoader.getLongitude(), 6));

    WeatherData weatherData;
    bool success = weatherClient->fetchWeather(weatherData);

    if (!success) {
        Serial.println("Failed to fetch weather data!");
        return;
    }

    // Display weather info in serial
    Serial.println("\n=== Current Weather ===");
    Serial.println("Temperature: " + String(weatherData.currentTemp) + "°C");
    Serial.println("Weather Code: " + String(weatherData.currentWeatherCode));
    Serial.println("Is Day: " + String(weatherData.isDay ? "Yes" : "No"));

    Serial.println("\n=== Daily Forecast ===");
    for (size_t i = 0; i < weatherData.dailyForecast.size(); i++) {
        const WeatherDay& day = weatherData.dailyForecast[i];
        Serial.print("Date: " + day.date + " - ");
        Serial.print("Code: " + String(day.weatherCode) + ", ");
        Serial.print("Max: " + String(day.tempMax, 1) + "°C, ");
        Serial.print("Min: " + String(day.tempMin, 1) + "°C, ");
        Serial.println("Sunrise: " + day.sunrise.substring(11) + ", Sunset: " + day.sunset.substring(11));
    }

    // Display on screen
    Serial.println("\nDisplaying weather on screen...");

    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    // Use mock date if time not synced
    int currentDay = timeinfo->tm_mday;
    int currentMonth = timeinfo->tm_mon + 1;
    int currentYear = timeinfo->tm_year + 1900;

    if (currentYear <= 1970) {
        currentDay = 16;
        currentMonth = 10;
        currentYear = 2025;
    }

    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%I:%M %p", timeinfo);

    // Generate mock events for display
    std::vector<CalendarEvent*> mockEvents = generateMockEvents();

    // Get real WiFi signal strength
    int rssi = WiFi.RSSI();
    bool wifiConnected = WiFi.status() == WL_CONNECTED;

    // Get real battery values
    int adcValue = analogRead(BATTERY_PIN);
    float measuredVoltage = (adcValue / 4095.0) * 3.3;
    float batteryVoltage = measuredVoltage * BATTERY_VOLTAGE_DIVIDER;
    int batteryPercentage = getBatteryPercentage(batteryVoltage);

    // Display with weather data using real values
    display.showModernCalendar(mockEvents, currentDay, currentMonth, currentYear,
        String(timeStr), &weatherData, wifiConnected, rssi, batteryVoltage, batteryPercentage);

    Serial.println("Weather test complete!");
    Serial.println("========================================");
}

std::vector<CalendarEvent*> generateMockEvents()
{
    std::vector<CalendarEvent*> events;

    // Today's event (October 16)
    CalendarEvent* event1 = new CalendarEvent();
    event1->title = "DS Call Swisscom";
    event1->location = "Office";
    event1->startTimeStr = "11:00";
    event1->endTimeStr = "12:00";
    event1->date = "2025-10-16";
    event1->allDay = false;
    event1->isToday = true;
    event1->isTomorrow = false;
    event1->dayOfMonth = 16;
    events.push_back(event1);

    // Tomorrow's events (October 17)
    CalendarEvent* event2 = new CalendarEvent();
    event2->title = "Ritirare la moto";
    event2->location = "Officina";
    event2->startTimeStr = "10:30";
    event2->endTimeStr = "11:00";
    event2->date = "2025-10-17";
    event2->allDay = false;
    event2->isToday = false;
    event2->isTomorrow = true;
    event2->dayOfMonth = 17;
    events.push_back(event2);

    CalendarEvent* event3 = new CalendarEvent();
    event3->title = "Chiamare meccanico";
    event3->location = "";
    event3->startTimeStr = "12:30";
    event3->endTimeStr = "13:00";
    event3->date = "2025-10-17";
    event3->allDay = false;
    event3->isToday = false;
    event3->isTomorrow = true;
    event3->dayOfMonth = 17;
    events.push_back(event3);

    // Event on Sunday 19th (same month)
    CalendarEvent* event5 = new CalendarEvent();
    event5->title = "Buttare il secco";
    event5->location = "";
    event5->startTimeStr = "09:30";
    event5->endTimeStr = "10:00";
    event5->date = "2025-10-19";
    event5->allDay = false;
    event5->isToday = false;
    event5->isTomorrow = false;
    event5->dayOfMonth = 19;
    events.push_back(event5);

    // Event on Wednesday 22nd (same month)
    CalendarEvent* event6 = new CalendarEvent();
    event6->title = "Cena di Natale con i colleghi di Swisscom";
    event6->location = "Ristorante";
    event6->startTimeStr = "17:30";
    event6->endTimeStr = "23:00";
    event6->date = "2025-10-22";
    event6->allDay = false;
    event6->isToday = false;
    event6->isTomorrow = false;
    event6->dayOfMonth = 22;
    events.push_back(event6);

    // Event on Friday 31st (same month)
    CalendarEvent* event7 = new CalendarEvent();
    event7->title = "Partita";
    event7->location = "Stadium";
    event7->startTimeStr = "21:00";
    event7->endTimeStr = "23:00";
    event7->date = "2025-10-31";
    event7->allDay = false;
    event7->isToday = false;
    event7->isTomorrow = false;
    event7->dayOfMonth = 31;
    events.push_back(event7);

    // Event on Saturday, November 4th (next month)
    CalendarEvent* event8 = new CalendarEvent();
    event8->title = "Partita";
    event8->location = "Stadium";
    event8->startTimeStr = "09:00";
    event8->endTimeStr = "11:00";
    event8->date = "2025-11-04";
    event8->allDay = false;
    event8->isToday = false;
    event8->isTomorrow = false;
    event8->dayOfMonth = 4;
    events.push_back(event8);

    // Event on Christmas Eve (December)
    CalendarEvent* event9 = new CalendarEvent();
    event9->title = "Vigilia di Natale";
    event9->location = "Casa";
    event9->startTimeStr = "19:00";
    event9->endTimeStr = "23:59";
    event9->date = "2025-12-24";
    event9->allDay = false;
    event9->isToday = false;
    event9->isTomorrow = false;
    event9->dayOfMonth = 24;
    events.push_back(event9);

    // Event on New Year's Day 2026 (next year)
    CalendarEvent* event10 = new CalendarEvent();
    event10->title = "Nuovo Anno";
    event10->location = "Casa";
    event10->startTimeStr = "12:00";
    event10->endTimeStr = "13:00";
    event10->date = "2026-01-01";
    event10->allDay = false;
    event10->isToday = false;
    event10->isTomorrow = false;
    event10->dayOfMonth = 1;
    events.push_back(event10);

    return events;
}

WeatherData generateMockWeather()
{
    WeatherData weatherData;

    // Current weather
    weatherData.currentTemp = 15.5;
    weatherData.currentWeatherCode = 2; // Partly cloudy
    weatherData.isDay = true;

    // Daily forecast (today and tomorrow)
    WeatherDay today;
    today.date = "2025-10-16";
    today.weatherCode = 2;
    today.tempMax = 18.5;
    today.tempMin = 8.0;
    today.sunrise = "2025-10-16T06:42";
    today.sunset = "2025-10-16T18:15";
    today.precipitationProbability = 15; // 15% chance of rain
    weatherData.dailyForecast.push_back(today);

    WeatherDay tomorrow;
    tomorrow.date = "2025-10-17";
    tomorrow.weatherCode = 3;
    tomorrow.tempMax = 16.2;
    tomorrow.tempMin = 9.5;
    tomorrow.sunrise = "2025-10-17T06:44";
    tomorrow.sunset = "2025-10-17T18:13";
    tomorrow.precipitationProbability = 45; // 45% chance of rain
    weatherData.dailyForecast.push_back(tomorrow);

    return weatherData;
}

// LiPo battery discharge curve lookup table
// Based on typical LiPo discharge characteristics
int getBatteryPercentage(float voltage)
{
    // Voltage to percentage lookup table for single LiPo cell
    const struct {
        float voltage;
        int percentage;
    } lipoTable[] = {
        { 4.20, 100 },
        { 4.15, 95 },
        { 4.11, 90 },
        { 4.08, 85 },
        { 4.02, 80 },
        { 3.98, 75 },
        { 3.95, 70 },
        { 3.91, 65 },
        { 3.87, 60 },
        { 3.85, 55 },
        { 3.84, 50 },
        { 3.82, 45 },
        { 3.80, 40 },
        { 3.79, 35 },
        { 3.77, 30 },
        { 3.75, 25 },
        { 3.73, 20 },
        { 3.71, 15 },
        { 3.69, 10 },
        { 3.60, 5 },
        { 3.50, 0 }
    };

    const int tableSize = sizeof(lipoTable) / sizeof(lipoTable[0]);

    // Handle edge cases
    if (voltage >= lipoTable[0].voltage)
        return lipoTable[0].percentage;
    if (voltage <= lipoTable[tableSize - 1].voltage)
        return lipoTable[tableSize - 1].percentage;

    // Find the two points to interpolate between
    for (int i = 0; i < tableSize - 1; i++) {
        if (voltage >= lipoTable[i + 1].voltage) {
            // Linear interpolation between two points
            float v1 = lipoTable[i].voltage;
            float v2 = lipoTable[i + 1].voltage;
            int p1 = lipoTable[i].percentage;
            int p2 = lipoTable[i + 1].percentage;

            float ratio = (voltage - v2) / (v1 - v2);
            return p2 + (int)(ratio * (p1 - p2));
        }
    }

    return 0;
}

// Helper function to list files recursively
void listDirectory(File dir, int numTabs = 1)
{
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            // No more files
            break;
        }

        // Print indentation
        for (int i = 0; i < numTabs; i++) {
            Serial.print("  ");
        }

        // Print file/directory info
        if (entry.isDirectory()) {
            Serial.print("[DIR] ");
            Serial.println(entry.name());
            // Recurse into directory
            listDirectory(entry, numTabs + 1);
        } else {
            Serial.print(entry.name());
            Serial.print(" (");
            Serial.print(entry.size());
            Serial.println(" bytes)");
        }
        entry.close();
    }
}

void testLittleFSConfig()
{
    Serial.println("\n========================================");
    Serial.println("Testing LittleFS Configuration");
    Serial.println("========================================\n");

    // Create a new config loader instance for testing
    LittleFSConfig testConfigLoader;

    // Use the same initialization method as main.cpp
    Serial.println("--- Initializing LittleFS via configLoader.begin() ---");

    if (!testConfigLoader.begin()) {
        Serial.println("ERROR: Failed to initialize LittleFS via configLoader.begin()!");
        return;
    }

    Serial.println("LittleFS initialized successfully via configLoader.begin()");

    // Show filesystem info after initialization
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    Serial.println("\n--- LittleFS Filesystem Status ---");
    Serial.println("Total space: " + String(totalBytes / 1024.0, 2) + " KB");
    Serial.println("Used space: " + String(usedBytes / 1024.0, 2) + " KB");
    Serial.println("Free space: " + String((totalBytes - usedBytes) / 1024.0, 2) + " KB");

    // List all files recursively
    Serial.println("\n--- Complete File System Listing ---");
    File root = LittleFS.open("/");
    if (root) {
        if (!root.isDirectory()) {
            Serial.println("ERROR: Root is not a directory!");
        } else {
            listDirectory(root);
        }
        root.close();
    } else {
        Serial.println("ERROR: Failed to open root directory!");
    }

    // Try to read the raw config.json file
    Serial.println("\n--- Raw config.json Content ---");

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("ERROR: config.json file not found in LittleFS!");
        Serial.println("\nTo fix this:");
        Serial.println("1. Create a 'data' folder in your project root");
        Serial.println("2. Add your config.json file to the data folder");
        Serial.println("3. Upload filesystem: pio run -t uploadfs -e esp32-s3-devkitc-1");
        return;
    }

    Serial.println("File found! Size: " + String(configFile.size()) + " bytes\n");
    Serial.println("--- Begin config.json ---");

    // Read and print line by line for better readability
    int lineNum = 1;
    while (configFile.available()) {
        String line = configFile.readStringUntil('\n');
        Serial.printf("%3d: %s\n", lineNum++, line.c_str());
    }
    configFile.close();

    Serial.println("--- End config.json ---\n");

    // Now load and parse the configuration using the same method as main.cpp
    Serial.println("--- Loading configuration via configLoader.loadConfiguration() ---");
    if (!testConfigLoader.loadConfiguration()) {
        Serial.println("ERROR: Failed to load configuration via configLoader.loadConfiguration()!");
        Serial.println("The config.json file exists but may have JSON syntax errors or parsing issues.");
        Serial.println("Check the debug output above for specific parsing errors.");
        return;
    }

    Serial.println("\nConfiguration loaded successfully!\n");

    // Display all configuration values
    Serial.println("--- WiFi Configuration ---");
    Serial.println("SSID: " + testConfigLoader.getWiFiSSID());
    Serial.println("Password: " + String(testConfigLoader.getWiFiPassword().isEmpty() ? "(no password)" : "********"));

    Serial.println("\n--- Time & Location ---");
    Serial.println("Timezone: " + testConfigLoader.getTimezone());
    Serial.println("Latitude: " + String(testConfigLoader.getLatitude(), 6));
    Serial.println("Longitude: " + String(testConfigLoader.getLongitude(), 6));
    Serial.println("Update Hour: " + String(testConfigLoader.getUpdateHour()) + ":00");

    Serial.println("\n--- Display Settings ---");
    Serial.println("First Day of Week: " + String(FIRST_DAY_OF_WEEK) + (FIRST_DAY_OF_WEEK == 0 ? " (Sunday)" : " (Monday)"));
    Serial.println("Time Format: " + String(TIME_FORMAT_24H ? "24-hour" : "12-hour"));
    Serial.println("Display Language: " + String(DISPLAY_LANGUAGE));

    Serial.println("\n--- Calendar Configuration ---");
    const auto& calendars = testConfigLoader.getCalendars();

    if (!calendars.empty()) {
        Serial.println("Multiple calendars configured: " + String(calendars.size()));
        for (size_t i = 0; i < calendars.size(); i++) {
            Serial.println("\nCalendar #" + String(i + 1) + ":");
            Serial.println("  Name: " + calendars[i].name);
            Serial.println("  Color: " + calendars[i].color);
            Serial.println("  Days to fetch: " + String(calendars[i].days_to_fetch));
            Serial.println("  Enabled: " + String(calendars[i].enabled ? "Yes" : "No"));
            Serial.println("  URL: " + calendars[i].url.substring(0, 50) + (calendars[i].url.length() > 50 ? "..." : ""));
        }
    } else {
        // Fallback to single calendar
        String calendarUrl = testConfigLoader.getCalendarUrl();
        Serial.println("Single calendar mode");
        Serial.println("Calendar URL: " + (calendarUrl.isEmpty() ? "(not configured)" : calendarUrl.substring(0, 50) + (calendarUrl.length() > 50 ? "..." : "")));
    }

    // Display on screen as well
    Serial.println("\n--- Displaying on Screen ---");

    display.setRotation(0);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&Ubuntu_R_18pt8b);
        display.setCursor(50, 40);
        display.print("LittleFS Configuration");

        display.setFont(&Ubuntu_R_12pt8b);
        int y = 80;

        // LittleFS space information
        display.setCursor(50, y);
        display.print("FS Total: " + String(totalBytes / 1024.0, 1) + " KB");
        y += 25;

        display.setCursor(50, y);
        display.print("FS Used: " + String(usedBytes / 1024.0, 1) + " KB");
        y += 25;

        display.setCursor(50, y);
        display.print("FS Free: " + String((totalBytes - usedBytes) / 1024.0, 1) + " KB");
        y += 25;

        // Add separator
        y += 10;

        display.setCursor(50, y);
        display.print("WiFi: " + testConfigLoader.getWiFiSSID());
        y += 25;

        display.setCursor(50, y);
        display.print("Timezone: " + testConfigLoader.getTimezone());
        y += 25;

        display.setCursor(50, y);
        display.print("Location: " + String(testConfigLoader.getLatitude(), 2) + ", " + String(testConfigLoader.getLongitude(), 2));
        y += 25;

        display.setCursor(50, y);
        display.print("Calendars: " + String(calendars.empty() ? 1 : calendars.size()));
        y += 25;

        display.setCursor(50, y);
        display.print("Language: " + String(DISPLAY_LANGUAGE));
        y += 25;

        display.setCursor(50, y);
        display.print("Update Hour: " + String(testConfigLoader.getUpdateHour()) + ":00");
        y += 25;

        display.setCursor(50, y);
        display.print("First Day: " + String(FIRST_DAY_OF_WEEK == 0 ? "Sunday" : "Monday"));

    } while (display.nextPage());

    Serial.println("Configuration display complete!");
    Serial.println("========================================\n");
}

const char HelloWorld[] = "Hello World!";
const char HelloArduino[] = "Hello Arduino!";
const char HelloEpaper[] = "Hello E-Paper!";

void helloWorld()
{
    Serial.println("helloWorld");
    display.setRotation(1);
    display.setFont(&FreeMonoBold9pt7b);
    if (display.width() < 104)
        display.setFont(0);
    display.setTextColor(GxEPD_BLACK);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
    // center bounding box by transposition of origin:
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(HelloWorld);
    } while (display.nextPage());
    Serial.println("helloWorld done");
}

void testDisplayCapabilities()
{
    Serial.println("\n========================================");
    Serial.println("Testing Display Capabilities");
    Serial.println("========================================\n");

    // Get display info
    int width = display.width();
    int height = display.height();
    int pages = display.pages();
    int pageHeight = display.pageHeight();
    bool hasColor = display.hasColor();
    bool hasPartialUpdate = display.hasPartialUpdate();
    bool hasFastPartialUpdate = display.hasFastPartialUpdate();

    // Print to serial
    Serial.println("--- Display Information ---");
    Serial.println("Width: " + String(width) + " pixels");
    Serial.println("Height: " + String(height) + " pixels");
    Serial.println("Pages: " + String(pages));
    if (pages > 1) {
        Serial.println("Page Height: " + String(pageHeight) + " pixels");
    }
    Serial.println("Has Color: " + String(hasColor ? "Yes" : "No"));
    Serial.println("Supports Partial Update: " + String(hasPartialUpdate ? "Yes" : "No"));
    Serial.println("Supports Fast Partial Update: " + String(hasFastPartialUpdate ? "Yes" : "No"));

#ifdef DISP_TYPE_BW
    Serial.println("Display Type: Black & White (BW)");
#elif defined(DISP_TYPE_6C)
    Serial.println("Display Type: 6-Color (7C)");
#endif

    // Display on screen - First page: Info
    display.setFullWindow();
    display.firstPage();
    do {
        display.clear();

        display.setFont(&Ubuntu_R_18pt8b);
        display.setCursor(50, 50);
        display.print("Display Capabilities");

        display.setFont(&Ubuntu_R_12pt8b);
        int y = 100;
        int lineHeight = 30;

        display.setCursor(50, y);
        display.print("Resolution: " + String(width) + " x " + String(height) + " pixels");
        y += lineHeight;

        display.setCursor(50, y);
        display.print("Pages: " + String(pages));
        if (pages > 1) {
            display.print(" (Page Height: " + String(pageHeight) + ")");
        }
        y += lineHeight;

        display.setCursor(50, y);
        display.print("Has Color: " + String(hasColor ? "Yes" : "No"));
        y += lineHeight;

        display.setCursor(50, y);
        display.print("Partial Update: " + String(hasPartialUpdate ? "Yes" : "No"));
        y += lineHeight;

        display.setCursor(50, y);
        display.print("Fast Partial Update: " + String(hasFastPartialUpdate ? "Yes" : "No"));
        y += lineHeight;

        display.setCursor(50, y);
    #ifdef DISP_TYPE_BW
        display.print("Type: Black & White");
    #elif defined(DISP_TYPE_6C)
        display.print("Type: 6-Color (Red/Orange/Yellow)");
    #endif
        y += lineHeight + 20;

        display.setFont(&Ubuntu_R_9pt8b);
        display.setCursor(50, y);
        display.print("Press any key to see color/dithering test...");

    } while (display.nextPage());

    Serial.println("\nDisplaying info screen. Press any key to continue...");

    // Wait for user input
    while (!Serial.available()) {
        delay(100);
    }
    // Clear serial buffer
    while (Serial.available()) {
        Serial.read();
    }

    Serial.println("\n--- Displaying Color/Dithering Test ---\n");

    // Second page: Color/Dithering test
    display.firstPage();
    do {
        display.clear();

        display.setFont(&Ubuntu_R_18pt8b);
        display.setCursor(50, 50);

    #ifdef DISP_TYPE_BW
        display.print("B&W Display - Dithering");
    #elif defined(DISP_TYPE_6C)
        display.print("Color Display Test");
    #endif

        int startY = 80;
        int barWidth = 70;
        int barHeight = 300;
        int spacing = 20;
        int x = 50;

    #ifdef DISP_TYPE_BW
        // For B&W displays: show dithering levels + special colors

        // Dithering levels
        DitherLevel ditherLevels[] = {
            DitherLevel::DITHER_10,
            DitherLevel::DITHER_25,
            DitherLevel::DITHER_50,
            DitherLevel::DITHER_75
        };

        const char* ditherLabels[] = {"10%", "25%", "50%", "75%"};

        for (int i = 0; i < 4; i++) {
            display.applyDithering(x, startY, barWidth, barHeight,
                                  GxEPD_WHITE, GxEPD_BLACK,
                                  static_cast<int>(ditherLevels[i]) / 100.0f);

            display.setFont(&Ubuntu_R_9pt8b);
            display.setCursor(x + 10, startY + barHeight + 20);
            display.print(ditherLabels[i]);

            x += barWidth + spacing;
        }

        // Add special colors: Black, White, Dark Grey, Light Grey
        uint16_t specialColors[] = {GxEPD_BLACK, GxEPD_WHITE, GxEPD_DARKGREY, GxEPD_LIGHTGREY};
        const char* colorLabels[] = {"Black", "White", "Dark", "Light"};

        for (int i = 0; i < 4; i++) {
            display.fillRect(x, startY, barWidth, barHeight, specialColors[i]);

            display.setFont(&Ubuntu_R_9pt8b);
            // Use contrasting color for text
            display.setTextColor(i == 0 ? GxEPD_WHITE : GxEPD_BLACK);
            display.setCursor(x + 5, startY + barHeight + 20);
            display.print(colorLabels[i]);
            display.setTextColor(GxEPD_BLACK); // Reset

            x += barWidth + spacing;
        }

    #elif defined(DISP_TYPE_6C)
        // For color displays: show all supported colors
        uint16_t colors[] = {
            GxEPD_BLACK,
            GxEPD_WHITE,
            GxEPD_RED,
            GxEPD_YELLOW,
            GxEPD_ORANGE,
            GxEPD_DARKGREY,
            GxEPD_LIGHTGREY
        };

        const char* colorLabels[] = {
            "Black",
            "White",
            "Red",
            "Yellow",
            "Orange",
            "Dark",
            "Light"
        };

        for (int i = 0; i < 7; i++) {
            display.fillRect(x, startY, barWidth, barHeight, colors[i]);

            display.setFont(&Ubuntu_R_9pt8b);
            // Use contrasting color for text on white/yellow/orange
            if (i == 1 || i == 3 || i == 4) {
                display.setTextColor(GxEPD_BLACK);
            } else {
                display.setTextColor(GxEPD_WHITE);
            }
            display.setCursor(x + 5, startY + barHeight + 20);
            display.print(colorLabels[i]);
            display.setTextColor(GxEPD_BLACK); // Reset

            x += barWidth + spacing;

            // Move to second row if needed
            if (i == 3 && x > 600) {
                x = 50;
                startY += barHeight + 50;
            }
        }
    #endif

    } while (display.nextPage());

    Serial.println("Color/Dithering test displayed!");
    Serial.println("========================================\n");
}

#endif // DEBUG_DISPLAY