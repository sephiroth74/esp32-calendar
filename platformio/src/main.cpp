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
void enterDeepSleep(int retryMinutes = -1);  // -1 means calculate next update hour
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

    // Initialize LittleFS and load configuration
    Serial.println("Initializing LittleFS...");
    if (!configLoader.begin()) {
        Serial.println("Failed to initialize LittleFS!");
        displayMgr.showMessage("Configuration Error",
            "Failed to mount filesystem\n\n"
            "Device will sleep indefinitely.\n"
            "Fix filesystem and reset device.");
        delay(10000);  // Give user time to read the message
        Serial.println("Going to indefinite deep sleep due to filesystem error...");
        enterDeepSleep(0);  // 0 means no wake-up timer - sleep indefinitely
    }

    // Load configuration from LittleFS
    if (!configLoader.loadConfiguration()) {
        Serial.println("No valid configuration found in LittleFS!");
        displayMgr.showMessage("Configuration Missing",
            "Please upload config.json:\n\n"
            "1. Edit data/config.json\n"
            "2. Run: pio run -t uploadfs\n\n"
            "Device will sleep indefinitely.");
        delay(15000);  // Give user more time to read instructions
        Serial.println("Going to indefinite deep sleep due to missing configuration...");
        enterDeepSleep(0);  // 0 means no wake-up timer - sleep indefinitely
    }

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

    // Enter deep sleep or stay awake for testing
    if (!DISABLE_DEEP_SLEEP) {
        // Normal operation - enter deep sleep
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
            // Success - wake up at next update hour
            enterDeepSleep();  // Default: next day at update hour
        }
    } else {
        Serial.println("\n=== DEEP SLEEP DISABLED - Device staying awake for testing ===");
        Serial.println("Hold button for 3 seconds to reset configuration and restart");
    }
}

void loop() {
    // Only runs when DISABLE_DEEP_SLEEP is true
    if (DISABLE_DEEP_SLEEP) {
        // Monitor button for configuration reset
        int buttonState = digitalRead(BUTTON_PIN);

        if (buttonState == HIGH && !buttonPressed) {
            // Button just pressed
            buttonPressed = true;
            buttonPressStart = millis();
            Serial.println("Button pressed - hold for 3 seconds to reset config...");
        } else if (buttonState == LOW && buttonPressed) {
            // Button released
            buttonPressed = false;
            unsigned long pressDuration = millis() - buttonPressStart;

            if (pressDuration >= CONFIG_RESET_HOLD_TIME) {
                Serial.println("Configuration reset triggered!");

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
                Serial.println("Button released after " + String(pressDuration) + "ms (not long enough)");
            }
        } else if (buttonPressed) {
            // Button still held - check if held long enough
            unsigned long pressDuration = millis() - buttonPressStart;
            if (pressDuration >= CONFIG_RESET_HOLD_TIME && !configResetPending) {
                configResetPending = true;
                Serial.println("Config reset ready - release button to execute");
            }
        }

        // Small delay to prevent excessive CPU usage
        delay(50);
    }
}

void performUpdate() {
    // Get configuration
    const RuntimeConfig& config = configLoader.getConfig();

    // Connect to WiFi using WiFiManager
    Serial.println("\n--- WiFi Connection ---");
    Serial.println("Connecting to: " + config.wifi_ssid);

    if (!wifiManager.connect(configLoader)) {
        Serial.println("WiFi connection failed!");
        lastError = ErrorCode::WIFI_CONNECTION_FAILED;
        errorMgr.setError(lastError);
        displayMgr.showFullScreenError(errorMgr.getCurrentError());
        return;
    }

    Serial.println("WiFi connected!");
    Serial.println("IP Address: " + wifiManager.getIPAddress());
    Serial.println("RSSI: " + String(wifiManager.getRSSI()) + " dBm");

    // Successful WiFi connection
    lastError = ErrorCode::SUCCESS;

    // Initialize WiFiClientSecure
    WiFiClientSecure* client = new WiFiClientSecure();
    if (!client) {
        Serial.println("Failed to create WiFiClientSecure");
        return;
    }
    client->setInsecure();  // Skip certificate validation

    // Initialize clients with configuration from LittleFS
    weatherClient = new WeatherClient(client);
    weatherClient->setLocation(config.latitude, config.longitude);

    // Initialize calendar manager
    calendarManager = new CalendarManager();
    calendarManager->setDebug(true); // Enable debug output

    // Update battery status
    Serial.println("\n--- Battery Status ---");
    batteryMonitor.update();
    batteryMonitor.printStatus();

    // Check battery level
    if (batteryMonitor.isCritical()) {
        Serial.println("Battery critical: " + String(batteryMonitor.getPercentage()) + "%");
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
    if (!wifiManager.syncTimeFromNTP(config.timezone)) {
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
    std::vector<CalendarEvent*> events;

    // Load calendar configuration
    calendarManager->loadFromConfig(config);

    // Load all calendars
    bool calendarSuccess = calendarManager->loadAll(false); // false = use cache if available

    if (calendarSuccess) {
        // Get current time
        time_t now = time(nullptr);
        time_t endDate = now + (365 * 86400); // Get events for next year

        // Get merged events from all calendars
        events = calendarManager->getAllEvents(now, endDate);

        if (!events.empty()) {
            Serial.println("Fetched " + String(events.size()) + " events from " +
                         String(calendarManager->getCalendarCount()) + " calendars");
            lastError = ErrorCode::SUCCESS;

            // Only limit total events if we have too many
            if (events.size() > MAX_EVENTS_TO_SHOW) {
                events.resize(MAX_EVENTS_TO_SHOW);
                Serial.println("Limited to " + String(MAX_EVENTS_TO_SHOW) + " events");
            }
        } else {
            Serial.println("No events found in calendars");
            lastError = ErrorCode::CALENDAR_FETCH_FAILED;
            errorMgr.setError(lastError);
        }
    } else {
        Serial.println("Calendar fetch failed");
        lastError = ErrorCode::CALENDAR_FETCH_FAILED;
        errorMgr.setError(lastError);
    }

    // Print calendar status
    calendarManager->printStatus();

    // Prepare events for display (add compatibility fields)
    if (!events.empty()) {
        CalendarDisplayAdapter::prepareEventsForDisplay(events);
        Serial.println("Events prepared for display");
    }

    // Update display
    Serial.println("\n--- Display Update ---");
    displayMgr.showCalendar(events, currentDate, currentTime,
                           weatherSuccess ? &weatherData : nullptr,
                           wifiManager.isConnected(),
                           wifiManager.getRSSI(),
                           batteryMonitor.getVoltage(),
                           batteryMonitor.getPercentage());

    Serial.println("Display update complete");

    // Note: Events are managed by CalendarManager and ICSParser, no need to free them manually

    // Cleanup clients
    delete weatherClient;
    weatherClient = nullptr;
    delete calendarManager;
    calendarManager = nullptr;
    delete client;
}

void enterDeepSleep(int retryMinutes) {
    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    int sleepSeconds;

    if (retryMinutes == 0) {
        // Indefinite sleep - don't set wake-up timer
        // Used for: battery too low, configuration errors, filesystem errors
        Serial.println("Sleeping indefinitely without wake-up timer");
        // Don't call esp_sleep_enable_timer_wakeup
    } else if (retryMinutes > 0) {
        // Error retry - sleep for specified minutes
        sleepSeconds = retryMinutes * 60;
        Serial.println("Error retry - sleeping for " + String(retryMinutes) + " minutes");
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

        Serial.println("Next update at " + String(updateHour) + ":00 tomorrow");
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
    wifiManager.disconnect();

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

#endif // DEBUG_DISPLAY