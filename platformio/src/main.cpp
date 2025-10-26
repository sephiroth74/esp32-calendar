#if !defined(DEBUG_DISPLAY) && !defined(PIO_UNIT_TESTING)

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <LittleFS.h>
#include <time.h>
#include "display_manager.h"
#include "calendar_wrapper.h"
#include "calendar_display_adapter.h"
#include "weather_client.h"
#include "littlefs_config.h"
#include "error_manager.h"
#include "wifi_manager.h"
#include "battery_monitor.h"
#include "config.h"
#include "debug_config.h"
#include "version.h"

// Global objects
DisplayManager displayMgr;
LittleFSConfig configLoader;
ErrorManager errorMgr;
WiFiManager wifiManager;
BatteryMonitor batteryMonitor;
WeatherClient* weatherClient = nullptr;
CalendarManager* calendarManager = nullptr;

// Variables to track last error for retry logic
ErrorCode lastError = ErrorCode::SUCCESS;

// Button monitoring for configuration reset
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool configResetPending = false;

// Forward declarations
void performUpdate();
void enterDeepSleep(int retryMinutes = -1); // -1 means calculate next update hour
void printWakeupReason();

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n" + String(PROJECT_NAME) + " v" + VERSION);
    Serial.println("Build: " + String(__DATE__) + " " + String(__TIME__));
    Serial.println("=====================================");

    bool enableDeepSleep = !DISABLE_DEEP_SLEEP;
    unsigned long delayBeforeDeepSleepMs = 5000;

    Serial.println("Deep Sleep: " + String(enableDeepSleep ? "ENABLED" : "DISABLED"));
    Serial.println("=====================================");

    // Print wake-up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    printWakeupReason();

    // Check wake-up reason and add appropriate delay
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        DEBUG_INFO_PRINTLN("Button wake-up - waiting 15 seconds for viewing...");
        delayBeforeDeepSleepMs = 5000;
    } else if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
        DEBUG_INFO_PRINTLN("Undefined wakeup - waiting 10 seconds...");
        delayBeforeDeepSleepMs = 1000;
    } else {
        DEBUG_INFO_PRINTLN("Timer wake-up - minimal delay");
        delayBeforeDeepSleepMs = 1000;
    }

    // Configure button if enabled
    if (BUTTON_WAKEUP_ENABLED) {
        pinMode(BUTTON_PIN, INPUT_PULLDOWN);

        // Apply stronger pull-down
        pinMode(BUTTON_PIN, OUTPUT);
        digitalWrite(BUTTON_PIN, LOW);
        delay(1);
        pinMode(BUTTON_PIN, INPUT_PULLDOWN);

        DEBUG_VERBOSE_PRINTLN("Button configured on pin " + String(BUTTON_PIN));

        // If the wakeup button is pressed for more than CONFIG_RESET_HOLD_TIME during boot, disable the deep sleep
        if (enableDeepSleep && digitalRead(BUTTON_PIN) == HIGH) {
            DEBUG_WARN_PRINTLN("Button held during boot... ");
            delay(CONFIG_RESET_HOLD_TIME);
            if (digitalRead(BUTTON_PIN) == HIGH) {
                DEBUG_WARN_PRINTLN("Configuration reset triggered during boot! Disabling deep sleep.");
                enableDeepSleep = false;
            }
        }
    }

    // Update battery status
    DEBUG_INFO_PRINTLN("\n--- Battery Status ---");
    batteryMonitor.update();

#if DEBUG_LEVEL >= DEBUG_INFO
    batteryMonitor.printStatus();
#endif

    delay(1000);

    if (batteryMonitor.isCritical()) {
        DEBUG_ERROR_PRINTLN("Battery critical: " + String(batteryMonitor.getPercentage()) + "%");
        lastError = ErrorCode::BATTERY_LOW;
        errorMgr.setError(lastError);
        delay(10000); // Give user time to read the message

        if (enableDeepSleep) {
            enterDeepSleep(0); // 0 means no wake-up timer - sleep indefinitely
        } else {
            DEBUG_WARN_PRINTLN("Deep sleep disabled - staying awake for testing.");
        }
        return;
    }

    // Initialize display
    DEBUG_INFO_PRINTLN("Initializing display...");
    displayMgr.init();

    if (batteryMonitor.isLow()) {
        DEBUG_WARN_PRINTLN("Battery low: " + String(batteryMonitor.getPercentage()) + "%");
        lastError = ErrorCode::BATTERY_LOW;
        errorMgr.setError(lastError);
        displayMgr.showFullScreenError(errorMgr.getCurrentError());
        DEBUG_WARN_PRINTLN("Going to indefinite deep sleep due to low battery...");

        if (enableDeepSleep) {
            delay(5000); // Give user time to read the message
            enterDeepSleep(0); // 0 means no wake-up timer - sleep indefinitely
        } else {
            DEBUG_WARN_PRINTLN("Deep sleep disabled - staying awake for testing.");
        }
        return;
    }

    // Initialize LittleFS and load configuration
    DEBUG_INFO_PRINTLN("Initializing LittleFS...");
    if (!configLoader.begin()) {
        DEBUG_ERROR_PRINTLN("Failed to initialize LittleFS!");
        displayMgr.showMessage("Configuration Error",
            "Failed to mount filesystem\n\n"
            "Device will sleep indefinitely.\n"
            "Fix filesystem and reset device.");
        delay(10000); // Give user time to read the message

        if (enableDeepSleep) {
            DEBUG_WARN_PRINTLN("Going to indefinite deep sleep due to filesystem error...");
            enterDeepSleep(0); // 0 means no wake-up timer - sleep indefinitely
        } else {
            DEBUG_WARN_PRINTLN("Deep sleep disabled - staying awake for testing.");
        }
        return;
    }

    // Load configuration from LittleFS
    if (!configLoader.loadConfiguration()) {
        DEBUG_ERROR_PRINTLN("No valid configuration found in LittleFS!");
        displayMgr.showMessage("Configuration Missing",
            "Please upload config.json:\n\n"
            "1. Edit data/config.json\n"
            "2. Run: pio run -t uploadfs\n\n"
            "Device will sleep indefinitely.");

        if (enableDeepSleep) {
            delay(15000); // Give user more time to read instructions
            DEBUG_WARN_PRINTLN("Going to indefinite deep sleep due to missing configuration...");
            delay(10000);
            enterDeepSleep(0); // 0 means no wake-up timer - sleep indefinitely
        } else {
            DEBUG_WARN_PRINTLN("Deep sleep disabled - staying awake for testing.");
        }
        return;
    }

    // Perform calendar update
    performUpdate();

    DEBUG_INFO_PRINTLN("\n--- Setup Complete ---");

    // Enter deep sleep or stay awake for testing
    if (enableDeepSleep) {
        if (delayBeforeDeepSleepMs > 0) {
            DEBUG_INFO_PRINTLN("Waiting " + String(delayBeforeDeepSleepMs) + "ms before deep sleep...");
            delay(delayBeforeDeepSleepMs);
        }

        DEBUG_INFO_PRINTLN("Entering deep sleep...");

        // Normal operation - enter deep sleep
        if (lastError == ErrorCode::BATTERY_LOW || lastError == ErrorCode::BATTERY_CRITICAL) {
            // Battery too low - sleep without setting wake-up timer
            enterDeepSleep(0); // 0 means no wake-up timer
        } else if (lastError == ErrorCode::WIFI_CONNECTION_FAILED) {
            // WiFi error - retry after 1 hour
            enterDeepSleep(WIFI_ERROR_RETRY_MINUTES);
        } else if (lastError == ErrorCode::CALENDAR_FETCH_FAILED) {
            // Calendar error - retry after 2 hours
            enterDeepSleep(CALENDAR_ERROR_RETRY_MINUTES);
        } else {
            // Success - wake up at next update hour
            enterDeepSleep(); // Default: next day at update hour
        }
    } else {
        DEBUG_INFO_PRINTLN("\n=== DEEP SLEEP DISABLED - Device staying awake for testing ===");
        DEBUG_INFO_PRINTLN("Hold button for 3 seconds to reset configuration and restart");
    }
}

void loop()
{
    // Only runs when DISABLE_DEEP_SLEEP is true
    if (DISABLE_DEEP_SLEEP) {
        // Monitor button for configuration reset
        int buttonState = digitalRead(BUTTON_PIN);

        if (buttonState == HIGH && !buttonPressed) {
            // Button just pressed
            buttonPressed = true;
            buttonPressStart = millis();
            DEBUG_VERBOSE_PRINTLN("Button pressed - hold for 3 seconds to reset config...");
        } else if (buttonState == LOW && buttonPressed) {
            // Button released
            buttonPressed = false;
            unsigned long pressDuration = millis() - buttonPressStart;

            if (pressDuration >= CONFIG_RESET_HOLD_TIME) {
                DEBUG_VERBOSE_PRINTLN("Configuration reset triggered!");

                // Show message on display
                displayMgr.showMessage("Configuration Reset",
                    "Deleting saved configuration...\n\n"
                    "Device will restart.\n\n"
                    "Please upload new config.json");

                // Delete configuration
                configLoader.resetConfiguration();

                // Wait a moment for user to see message
                delay(3000);

                // Restart device
                ESP.restart();
            } else {
                DEBUG_VERBOSE_PRINTLN("Button released after " + String(pressDuration) + "ms (not long enough)");
            }
        } else if (buttonPressed) {
            // Button still held - check if held long enough
            unsigned long pressDuration = millis() - buttonPressStart;
            if (pressDuration >= CONFIG_RESET_HOLD_TIME && !configResetPending) {
                configResetPending = true;
                DEBUG_VERBOSE_PRINTLN("Config reset ready - release button to execute");
            }
        }

        // Small delay to prevent excessive CPU usage
        delay(50);
    }
}

void performUpdate()
{
    // Get configuration
    const RuntimeConfig& config = configLoader.getConfig();

    // Connect to WiFi using WiFiManager
    DEBUG_INFO_PRINTLN("\n--- WiFi Connection ---");
    DEBUG_INFO_PRINTLN("Connecting to: " + config.wifi_ssid);

    if (!wifiManager.connect(config)) {
        DEBUG_INFO_PRINTLN("WiFi connection failed!");
        lastError = ErrorCode::WIFI_CONNECTION_FAILED;
        errorMgr.setError(lastError);
        displayMgr.showFullScreenError(errorMgr.getCurrentError());
        return;
    }

    DEBUG_INFO_PRINTLN("WiFi connected!");
    DEBUG_INFO_PRINTLN("IP Address: " + wifiManager.getIPAddress());
    DEBUG_INFO_PRINTLN("RSSI: " + String(wifiManager.getRSSI()) + " dBm");

    // Successful WiFi connection
    lastError = ErrorCode::SUCCESS;

    // Initialize WiFiClientSecure
    WiFiClientSecure* client = new WiFiClientSecure();
    if (!client) {
        DEBUG_ERROR_PRINTLN("Failed to create WiFiClientSecure");
        return;
    }
    client->setInsecure(); // Skip certificate validation

    // Initialize clients with configuration from LittleFS
    weatherClient = new WeatherClient(client);
    weatherClient->setLocation(config.latitude, config.longitude);

    // Initialize calendar manager
    calendarManager = new CalendarManager();
    calendarManager->setDebug(true); // Enable debug output

    // Fetch weather data
    DEBUG_INFO_PRINTLN("\n--- Weather Update ---");
    WeatherData weatherData;
    bool weatherSuccess = false;
    if (weatherClient) {
        weatherSuccess = weatherClient->fetchWeather(weatherData);
        if (weatherSuccess) {
            DEBUG_INFO_PRINTLN("Weather fetched successfully");
        } else {
            DEBUG_WARN_PRINTLN("Weather fetch failed (non-critical)");
        }
    }

    // Sync time from NTP
    DEBUG_INFO_PRINTLN("\n--- Time Sync ---");
    if (!wifiManager.syncTimeFromNTP(config.timezone, NTP_SERVER_1, NTP_SERVER_2)) {
        DEBUG_WARN_PRINTLN("Warning: NTP sync failed");
    }

    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    // Format date and time properly with zero padding
    char dateStr[32]; // Increased buffer size to avoid truncation warning
    char timeStr[6];
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d",
        timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
        timeinfo->tm_hour, timeinfo->tm_min);

    String currentDate = String(dateStr);
    String currentTime = String(timeStr);

    // Debug: Show timezone info
    DEBUG_VERBOSE_PRINTLN("Current local time: " + currentDate + " " + currentTime);
    DEBUG_VERBOSE_PRINTLN("Hour: " + String(timeinfo->tm_hour) + ", DST: " + String(timeinfo->tm_isdst));

    // Fetch calendar events
    DEBUG_INFO_PRINTLN("\n--- Calendar Update ---");
    std::vector<CalendarEvent*> events;

    // Load calendar configuration
    calendarManager->loadFromConfig(config);

    // Load all calendars
    bool allCalendarsSuccess = calendarManager->loadAll(false); // false = use cache if available
    DEBUG_INFO_PRINTLN("Calendar loadAll returned: " + String(allCalendarsSuccess ? "all success" : "some failures"));

    // Get current time
    now = time(nullptr);
    time_t endDate = now + (365 * 86400); // Get events for next year

    // Get merged events from all calendars (even if some failed to load)
    events = calendarManager->getAllEvents(now, endDate);
    DEBUG_INFO_PRINTLN("Fetched " + String(events.size()) + " events from " + String(calendarManager->getCalendarCount()) + " calendars");

    if (!events.empty()) {
        lastError = ErrorCode::SUCCESS;

        // Only limit total events if we have too many
        if (events.size() > MAX_EVENTS_TO_SHOW) {
            events.resize(MAX_EVENTS_TO_SHOW);
            DEBUG_VERBOSE_PRINTLN("Limited to " + String(MAX_EVENTS_TO_SHOW) + " events");
        }
    } else if (!allCalendarsSuccess) {
        // Only set error if no events AND some calendars failed
        DEBUG_WARN_PRINTLN("Some calendars failed to load and no events found");
        lastError = ErrorCode::CALENDAR_FETCH_FAILED;
        errorMgr.setError(lastError);
    } else {
        // All calendars loaded but no events found
        DEBUG_INFO_PRINTLN("All calendars loaded successfully but no events found");
        lastError = ErrorCode::SUCCESS;
    }

    // Print calendar status
    calendarManager->printStatus();

    // Prepare events for display (add compatibility fields)
    if (!events.empty()) {
        CalendarDisplayAdapter::prepareEventsForDisplay(events);
        DEBUG_INFO_PRINTLN("Events prepared for display");
    }

    // Update display
    DEBUG_INFO_PRINTLN("\n--- Display Update ---");
    displayMgr.showCalendar(events, currentDate, currentTime,
        weatherSuccess ? &weatherData : nullptr,
        wifiManager.isConnected(),
        wifiManager.getRSSI(),
        batteryMonitor.getVoltage(),
        batteryMonitor.getPercentage());

    DEBUG_INFO_PRINTLN("Display update complete");

    // Note: Events are managed by CalendarManager and CalendarStreamParser, no need to free them manually

    // Cleanup clients
    delete weatherClient;
    weatherClient = nullptr;
    delete calendarManager;
    calendarManager = nullptr;
    delete client;
}

void enterDeepSleep(int retryMinutes)
{
    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    int sleepSeconds;

    if (retryMinutes == 0) {
        // Indefinite sleep - don't set wake-up timer
        // Used for: battery too low, configuration errors, filesystem errors
        DEBUG_INFO_PRINTLN("Sleeping indefinitely without wake-up timer");
        // Don't call esp_sleep_enable_timer_wakeup
    } else if (retryMinutes > 0) {
        // Error retry - sleep for specified minutes
        sleepSeconds = retryMinutes * 60;
        DEBUG_WARN_PRINTLN("Error retry - sleeping for " + String(retryMinutes) + " minutes");
        esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    } else {
        // Normal operation - wake up at next update hour
        struct tm targetTime = *timeinfo;

        // Get update hour from configuration
        const RuntimeConfig& config = configLoader.getConfig();
        int updateHour = config.update_hour;

        // Always move to next day's update hour for once-daily update
        targetTime.tm_mday += 1;
        targetTime.tm_hour = updateHour;
        targetTime.tm_min = 0;
        targetTime.tm_sec = 0;

        time_t targetWakeup = mktime(&targetTime);
        sleepSeconds = targetWakeup - now;

        // If somehow negative, sleep for 24 hours
        if (sleepSeconds <= 0) {
            sleepSeconds = 24 * 60 * 60;
        }

        DEBUG_INFO_PRINTLN("Next update at " + String(updateHour) + ":00 tomorrow");
        DEBUG_INFO_PRINTLN("Sleeping for " + String(sleepSeconds / 3600) + " hours");
        esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    }

    // Configure button wake-up if enabled
    if (BUTTON_WAKEUP_ENABLED) {
        int buttonState = digitalRead(BUTTON_PIN);

        if (buttonState == HIGH) {
            DEBUG_INFO_PRINTLN("Waiting for button release...");
            int timeout = 50;
            while (digitalRead(BUTTON_PIN) == HIGH && timeout > 0) {
                delay(100);
                timeout--;
            }
        }

        if (digitalRead(BUTTON_PIN) == LOW) {
            uint64_t button_mask = 1ULL << BUTTON_PIN;
            esp_sleep_enable_ext1_wakeup(button_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
            DEBUG_INFO_PRINTLN("Button wake-up configured");
        }
    }

    // Disconnect WiFi
    wifiManager.disconnect();

    DEBUG_INFO_PRINTLN("Going to sleep...");
    Serial.flush();

    // Enter deep sleep
    esp_deep_sleep_start();
}

void printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    DEBUG_INFO_PRINTLN("\n=== WAKE-UP REASON ===");
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
        DEBUG_INFO_PRINTLN("Wake-up: External signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1: {
        uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        DEBUG_VERBOSE_PRINT("Wake-up: Button press on GPIO ");
#if DEBUG_LEVEL >= DEBUG_VERBOSE
        if (wakeup_pin_mask != 0) {
            int pin = __builtin_ctzll(wakeup_pin_mask);
            DEBUG_VERBOSE_PRINTLN(pin);
        }
#else
        (void)wakeup_pin_mask; // Suppress unused variable warning
#endif
        break;
    }
    case ESP_SLEEP_WAKEUP_TIMER:
        DEBUG_INFO_PRINTLN("Wake-up: Timer (scheduled update)");
        break;
    default:
        DEBUG_INFO_PRINTLN("Wake-up: Power on / Reset");
        break;
    }
    DEBUG_INFO_PRINTLN("=======================\n");
}

#endif // DEBUG_DISPLAY