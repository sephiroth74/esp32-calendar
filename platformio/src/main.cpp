#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <time.h>
#include "display_manager.h"
#include "calendar_client.h"
#include "weather_client.h"
#include "wifi_manager.h"
#include "error_manager.h"
#include "config.h"
#include "version.h"

// Global objects
DisplayManager displayMgr;
WiFiManager wifiMgr;
ErrorManager errorMgr;
WeatherClient* weatherClient = nullptr;
CalendarClient* calendarClient = nullptr;

// Variables to track last error for retry logic
ErrorCode lastError = ErrorCode::SUCCESS;

// Battery monitoring
float batteryVoltage = 0.0;
int batteryPercentage = 0;

// Forward declarations
void performUpdate();
bool syncTimeFromNTP();
void updateBatteryStatus();
int getBatteryPercentage(float voltage);
void enterDeepSleep(int retryMinutes = -1);  // -1 means calculate next DAY_START_HOUR
void printWakeupReason();

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n" + String(PROJECT_NAME) + " v" + VERSION);
    Serial.println("Build: " + String(__DATE__) + " " + String(__TIME__));
    Serial.println("=====================================");

    // Print wake-up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    printWakeupReason();

    // Configure button if enabled
    if (BUTTON_WAKEUP_ENABLED) {
        pinMode(BUTTON_PIN, INPUT_PULLDOWN);

        // Apply stronger pull-down
        pinMode(BUTTON_PIN, OUTPUT);
        digitalWrite(BUTTON_PIN, LOW);
        delay(1);
        pinMode(BUTTON_PIN, INPUT_PULLDOWN);

        Serial.println("Button configured on pin " + String(BUTTON_PIN));
    }

    // Initialize display
    Serial.println("Initializing display...");
    displayMgr.init();
    displayMgr.clear();

    // Initialize SPI
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    // Perform calendar update
    performUpdate();

    Serial.println("Setup complete!");

    // Check wake-up reason and add appropriate delay
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Button wake-up - waiting 15 seconds for viewing...");
        delay(15000);
    } else if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("Undefined wakeup - waiting 10 seconds...");
        delay(10000);
    } else {
        Serial.println("Timer wake-up - minimal delay");
        delay(1000);
    }

    // Enter deep sleep with appropriate retry interval based on last error
    if (lastError == ErrorCode::BATTERY_LOW || lastError == ErrorCode::BATTERY_CRITICAL) {
        // Battery too low - sleep without setting wake-up timer
        enterDeepSleep(0);  // 0 means no wake-up timer
    } else if (lastError == ErrorCode::WIFI_CONNECTION_FAILED) {
        // WiFi error - retry after 1 hour
        enterDeepSleep(WIFI_ERROR_RETRY_MINUTES);
    } else if (lastError == ErrorCode::CALENDAR_FETCH_FAILED) {
        // Calendar error - retry after 2 hours
        enterDeepSleep(CALENDAR_ERROR_RETRY_MINUTES);
    } else {
        // Success - wake up at next DAY_START_HOUR
        enterDeepSleep();  // Default: next day at DAY_START_HOUR
    }
}

void loop() {
    // Never reached - we go to deep sleep at end of setup
}

void performUpdate() {
    // Connect to WiFi
    Serial.println("\n--- WiFi Connection ---");
    if (!wifiMgr.connect()) {
        lastError = ErrorCode::WIFI_CONNECTION_FAILED;
        errorMgr.setError(lastError);
        displayMgr.showFullScreenError(errorMgr.getCurrentError());
        return;
    }

    // Successful WiFi connection
    lastError = ErrorCode::SUCCESS;

    // Initialize WiFiClientSecure
    WiFiClientSecure* client = new WiFiClientSecure();
    if (!client) {
        Serial.println("Failed to create WiFiClientSecure");
        return;
    }
    client->setInsecure();  // Skip certificate validation

    // Initialize clients
    weatherClient = new WeatherClient(client);
    calendarClient = new CalendarClient(client);

    // Update battery status
    Serial.println("\n--- Battery Status ---");
    updateBatteryStatus();

    // Check battery level
    if (batteryPercentage > 0 && batteryPercentage < 10) {
        Serial.println("Battery critical: " + String(batteryPercentage) + "%");
        lastError = ErrorCode::BATTERY_LOW;
        errorMgr.setError(lastError);
        displayMgr.showFullScreenError(errorMgr.getCurrentError());
        delay(3000);
        // Will sleep without wake-up timer
    }

    // Fetch weather data
    Serial.println("\n--- Weather Update ---");
    WeatherData weatherData;
    bool weatherSuccess = false;
    if (weatherClient) {
        weatherSuccess = weatherClient->fetchWeather(weatherData);
        if (weatherSuccess) {
            Serial.println("Weather fetched successfully");
        } else {
            Serial.println("Weather fetch failed (non-critical)");
        }
    }

    // Sync time from NTP
    Serial.println("\n--- Time Sync ---");
    if (!syncTimeFromNTP()) {
        Serial.println("Warning: NTP sync failed");
    }

    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    String currentDate = String(timeinfo->tm_mday) + "/" + String(timeinfo->tm_mon + 1) + "/" + String(timeinfo->tm_year + 1900);
    String currentTime = String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min);

    // Fetch calendar events
    Serial.println("\n--- Calendar Update ---");
    std::vector<CalendarEvent> events;

    String calendarUrl = ICS_CALENDAR_URL;
    events = calendarClient->fetchICSEvents(calendarUrl, DAYS_TO_FETCH);

    if (!events.empty()) {
        Serial.println("Fetched " + String(events.size()) + " events");
        lastError = ErrorCode::SUCCESS;

        // Filter past events
        calendarClient->filterPastEvents(events);

        // Limit events
        calendarClient->limitEvents(events, MAX_EVENTS_TO_SHOW);
    } else {
        Serial.println("Calendar fetch failed or no events");
        lastError = ErrorCode::CALENDAR_FETCH_FAILED;
        errorMgr.setError(lastError);
    }

    // Update display
    Serial.println("\n--- Display Update ---");
    displayMgr.showCalendar(events, currentDate, currentTime,
                           weatherSuccess ? &weatherData : nullptr,
                           wifiMgr.isConnected(),
                           wifiMgr.getRSSI(),
                           batteryVoltage,
                           batteryPercentage);

    Serial.println("Display update complete");

    // Cleanup
    delete weatherClient;
    weatherClient = nullptr;
    delete calendarClient;
    calendarClient = nullptr;
    delete client;
}

bool syncTimeFromNTP() {
    Serial.println("Configuring time with NTP...");

    // Set timezone
    setenv("TZ", TIMEZONE, 1);
    tzset();

    // Configure NTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Wait for time to be set
    int retry = 0;
    const int retry_count = 20;
    time_t now = time(nullptr);

    while (now < 24 * 3600 && retry < retry_count) {
        delay(500);
        now = time(nullptr);
        retry++;
        Serial.print(".");
    }
    Serial.println();

    if (now < 24 * 3600) {
        Serial.println("Failed to sync time from NTP");
        return false;
    }

    struct tm* timeinfo = localtime(&now);
    Serial.println("Time synchronized: " + String(asctime(timeinfo)));
    return true;
}

// LiPo discharge curve lookup table
struct BatteryPoint {
    float voltage;
    int percentage;
};

const BatteryPoint lipoTable[] = {
    {4.20, 100}, {4.15, 95}, {4.10, 90}, {4.05, 85}, {4.00, 80},
    {3.95, 75}, {3.90, 70}, {3.85, 65}, {3.80, 60}, {3.75, 55},
    {3.70, 50}, {3.65, 45}, {3.60, 40}, {3.55, 35}, {3.50, 30},
    {3.45, 25}, {3.40, 20}, {3.35, 15}, {3.30, 10}, {3.25, 5}, {3.00, 0}
};

int getBatteryPercentage(float voltage) {
    const int tableSize = sizeof(lipoTable) / sizeof(lipoTable[0]);

    if (voltage >= lipoTable[0].voltage) return lipoTable[0].percentage;
    if (voltage <= lipoTable[tableSize - 1].voltage) return lipoTable[tableSize - 1].percentage;

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

void updateBatteryStatus() {
    int adcValue = analogRead(BATTERY_PIN);
    float measuredVoltage = (adcValue / 4095.0) * 3.3;
    batteryVoltage = measuredVoltage * BATTERY_VOLTAGE_DIVIDER;
    batteryPercentage = getBatteryPercentage(batteryVoltage);

    Serial.print("Battery: ");
    Serial.print(batteryVoltage, 2);
    Serial.print("V (");
    Serial.print(batteryPercentage);
    Serial.println("%)");
}

void enterDeepSleep(int retryMinutes) {
    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    int sleepSeconds;

    if (retryMinutes == 0) {
        // Battery too low - don't set wake-up timer
        Serial.println("Battery too low - sleeping without wake-up timer");
        // Don't call esp_sleep_enable_timer_wakeup
    } else if (retryMinutes > 0) {
        // Error retry - sleep for specified minutes
        sleepSeconds = retryMinutes * 60;
        Serial.println("Error retry - sleeping for " + String(retryMinutes) + " minutes");
        esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    } else {
        // Normal operation - wake up at next DAY_START_HOUR
        struct tm targetTime = *timeinfo;

        // Always move to next day's DAY_START_HOUR for once-daily update
        targetTime.tm_mday += 1;
        targetTime.tm_hour = DAY_START_HOUR;
        targetTime.tm_min = 0;
        targetTime.tm_sec = 0;

        time_t targetWakeup = mktime(&targetTime);
        sleepSeconds = targetWakeup - now;

        // If somehow negative, sleep for 24 hours
        if (sleepSeconds <= 0) {
            sleepSeconds = 24 * 60 * 60;
        }

        Serial.println("Next update at " + String(DAY_START_HOUR) + ":00 tomorrow");
        Serial.println("Sleeping for " + String(sleepSeconds / 3600) + " hours");
        esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    }

    // Configure button wake-up if enabled
    if (BUTTON_WAKEUP_ENABLED) {
        int buttonState = digitalRead(BUTTON_PIN);

        if (buttonState == HIGH) {
            Serial.println("Waiting for button release...");
            int timeout = 50;
            while (digitalRead(BUTTON_PIN) == HIGH && timeout > 0) {
                delay(100);
                timeout--;
            }
        }

        if (digitalRead(BUTTON_PIN) == LOW) {
            uint64_t button_mask = 1ULL << BUTTON_PIN;
            esp_sleep_enable_ext1_wakeup(button_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
            Serial.println("Button wake-up configured");
        }
    }

    // Disconnect WiFi
    wifiMgr.disconnect();

    Serial.println("Going to sleep...");
    Serial.flush();

    // Enter deep sleep
    esp_deep_sleep_start();
}

void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    Serial.println("\n=== WAKE-UP REASON ===");
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wake-up: External signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1: {
            uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
            Serial.print("Wake-up: Button press on GPIO ");
            if (wakeup_pin_mask != 0) {
                int pin = __builtin_ctzll(wakeup_pin_mask);
                Serial.println(pin);
            }
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wake-up: Timer (scheduled update)");
            break;
        default:
            Serial.println("Wake-up: Power on / Reset");
            break;
    }
    Serial.println("=======================\n");
}