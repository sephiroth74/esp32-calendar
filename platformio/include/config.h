#ifndef CONFIG_H
#define CONFIG_H

/**
 * ESP32 E-Paper Calendar Configuration
 * Version 1.2.0
 *
 * This file contains all user-configurable settings for the calendar display.
 * Modify these values according to your hardware setup and preferences.
 */

// =============================================================================
// LOCAL CONFIGURATION (Sensitive Data)
// =============================================================================
/**
 * IMPORTANT: Sensitive configuration is stored in a separate file
 *
 * Before compiling, you must:
 * 1. Copy 'config.local.example' to 'config.local'
 * 2. Edit 'config.local' with your actual WiFi credentials and location
 *
 * The config.local file contains:
 * - WIFI_SSID: Your WiFi network name
 * - WIFI_PASSWORD: Your WiFi password
 * - LOC_LATITUDE: Your location latitude
 * - LOC_LONGITUDE: Your location longitude
 *
 * Note: config.local is excluded from git to protect your sensitive data
 */
#include "config.local"

// =============================================================================
// DISPLAY HARDWARE CONFIGURATION
// =============================================================================

// Display resolution in pixels
// Standard resolution for 7.5" e-paper displays
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480

// Display type selection - uncomment ONE option:
// Option 1: Black & White display
// #define DISP_TYPE_BW
// Option 2: 6-color display (Black, White, Red, Yellow, Orange, Green)
#define DISP_TYPE_6C

// =============================================================================
// COLOR SCHEME CONFIGURATION (6-COLOR DISPLAYS ONLY)
// =============================================================================

#ifdef DISP_TYPE_6C
    // Calendar month view colors
    // Today's date number color
    #define COLOR_CALENDAR_TODAY_TEXT      GxEPD_RED

    // Border color for today's cell
    #define COLOR_CALENDAR_TODAY_BORDER    GxEPD_RED

    // Days from previous/next month
    #define COLOR_CALENDAR_OUTSIDE_MONTH   GxEPD_GREEN

    // Weekend cell background (will be 25% dithered)
    #define COLOR_CALENDAR_WEEKEND_BG      GxEPD_YELLOW

    // Day of week labels (M, T, W, etc.)
    #define COLOR_CALENDAR_DAY_LABELS      GxEPD_RED

    // Events section colors
    // "Today" section header
    #define COLOR_EVENT_TODAY_HEADER       GxEPD_RED

    // "Tomorrow" section header
    #define COLOR_EVENT_TOMORROW_HEADER    GxEPD_ORANGE

    // Other date headers
    #define COLOR_EVENT_OTHER_HEADER       GxEPD_GREEN

    // Weather section colors
    // Weather icon color
    #define COLOR_WEATHER_ICON            GxEPD_RED

    // Header section colors
    // Large day number in header
    #define COLOR_HEADER_DAY_NUMBER       GxEPD_RED
#endif

// =============================================================================
// ESP32 PIN CONFIGURATION
// =============================================================================

// E-Paper display SPI connection pins
// Modify these based on your ESP32 board and wiring

// Display busy signal
#define EPD_BUSY  4

// Display reset pin
#define EPD_RST   6

// Data/Command selection pin
#define EPD_DC    5

// SPI Chip select
#define EPD_CS    7

// SPI Clock pin
#define EPD_SCK   12

// SPI MOSI (Master Out Slave In) data pin
#define EPD_MOSI  11

// =============================================================================
// CALENDAR DATA SOURCE CONFIGURATION
// =============================================================================

// Calendar source type
// Options: "ICS", "GOOGLE"
// ICS is recommended for simplicity
// Note: CalDAV support is not implemented in this version
#define CALENDAR_TYPE "ICS"

// -----------------------------------------------------------------------------
// ICS Calendar Configuration (for CALENDAR_TYPE = "ICS")
// -----------------------------------------------------------------------------

// ICS calendar URL
// How to get your ICS URL:
// - Google Calendar: Settings → Calendar → Integrate → Secret address in iCal format
// - iCloud: Calendar app → Share Calendar → Public Calendar → Copy link
// - Outlook: Settings → Calendar → Shared calendars → Publish → Copy ICS link
#define ICS_CALENDAR_URL "https://calendar.google.com/calendar/ical/alessandro.crugnola%40gmail.com/private-4ce5196fe2a4720cdeb66f798713d2a2/basic.ics"

// -----------------------------------------------------------------------------
// Google Calendar API Configuration (for CALENDAR_TYPE = "GOOGLE")
// -----------------------------------------------------------------------------

// Google Calendar ID
// Use "primary" for your main calendar
#define GOOGLE_CALENDAR_ID "primary"

// Google API credentials (requires OAuth2 setup)
#define GOOGLE_API_KEY "YOUR_GOOGLE_API_KEY"
#define GOOGLE_REFRESH_TOKEN "YOUR_REFRESH_TOKEN"
#define GOOGLE_CLIENT_ID "YOUR_CLIENT_ID"
#define GOOGLE_CLIENT_SECRET "YOUR_CLIENT_SECRET"

// -----------------------------------------------------------------------------
// CalDAV Configuration (for CALENDAR_TYPE = "CALDAV")
// -----------------------------------------------------------------------------
// Note: CalDAV support is not implemented in this version

// =============================================================================
// UPDATE & REFRESH CONFIGURATION
// =============================================================================

// Daily update time
// The calendar will update once per day at this hour (24-hour format)
// Example: 5 = 5:00 AM
#define DAY_START_HOUR 5

// Error retry intervals (in minutes)
// How long to wait before retrying after specific errors
#define WIFI_ERROR_RETRY_MINUTES 60        // Retry after 1 hour if WiFi fails
#define CALENDAR_ERROR_RETRY_MINUTES 120   // Retry after 2 hours if calendar fetch fails
// Note: Battery low error will not set a wake-up timer (sleep indefinitely)

// Timezone configuration with DST rules
// Format: "STD offset DST [offset],start[/time],end[/time]"
// Examples:
// - US Eastern: "EST5EDT,M3.2.0,M11.1.0"
// - US Pacific: "PST8PDT,M3.2.0,M11.1.0"
// - Central Europe: "CET-1CEST,M3.5.0,M10.5.0"
// - UK: "GMT0BST,M3.5.0,M10.5.0"
// - Japan: "JST-9"
// - Australia Sydney: "AEST-10AEDT,M10.1.0,M4.1.0"
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0"

// =============================================================================
// DISPLAY CONTENT CONFIGURATION
// =============================================================================

// Maximum number of events to show on the display
#define MAX_EVENTS_TO_SHOW 10

// Number of days ahead to fetch events
// Lower values reduce data usage
#define DAYS_TO_FETCH 30

// Show all-day events
#define SHOW_ALL_DAY_EVENTS true

// Show events that have already occurred today
#define SHOW_PAST_EVENTS false

// First day of the week
// 0 = Sunday, 1 = Monday, 2 = Tuesday, etc.
#define FIRST_DAY_OF_WEEK 1

// Time format
// true = 24-hour format (14:30)
// false = 12-hour format with AM/PM (2:30 PM)
#define TIME_FORMAT_24H true

// =============================================================================
// LOCALIZATION CONFIGURATION
// =============================================================================

// Display language
// Available options:
// 0 = English (EN)
// 1 = Spanish (ES) - not implemented
// 2 = French (FR) - not implemented
// 3 = German (DE) - not implemented
// 4 = Italian (IT)
// 5 = Portuguese (PT) - not implemented
// 6 = Dutch (NL) - not implemented
#define DISPLAY_LANGUAGE 4

// =============================================================================
// NTP TIME SYNCHRONIZATION
// =============================================================================
// NTP servers are hardcoded as "pool.ntp.org" and "time.nist.gov"


// =============================================================================
// BATTERY MONITORING CONFIGURATION
// =============================================================================

// ADC GPIO pin for battery voltage measurement
#define BATTERY_PIN 1

// Voltage divider ratio
// Calculate as: (R1 + R2) / R2
// Where R1 is connected to battery+ and R2 to GND
#define BATTERY_VOLTAGE_DIVIDER 2.1557377049

// =============================================================================
// BUTTON CONFIGURATION
// =============================================================================

// Manual refresh button GPIO pin
// Hardware setup for maximum reliability:
//
//      3.3V ──[Button]──┬──[1kΩ]──┬──── GPIO 2
//                       │          │
//                   [10kΩ]     [100nF]
//                       │          │
//                      GND        GND
//
// Components:
// - 1kΩ series resistor: Current limiting and GPIO protection
// - 10kΩ pull-down: Strong pull-down to prevent floating
// - 100nF capacitor: Hardware debouncing (1ms RC time constant)

// GPIO pin for wake-up button
#define BUTTON_PIN 2

// Enable/disable button wake-up feature
// Set to false if experiencing noise issues
#define BUTTON_WAKEUP_ENABLED true

// Software debounce time in milliseconds
// Can be low when using hardware debouncing
#define BUTTON_DEBOUNCE_MS 10

// Enable additional noise validation
// Not needed with proper hardware debouncing
#define BUTTON_NOISE_CHECK false

// =============================================================================
// WEATHER CONFIGURATION
// =============================================================================

// Location coordinates are defined in config.local
// The weather API URL is defined internally in weather_client.cpp
// It uses the Open-Meteo free weather API with your coordinates

#endif // CONFIG_H