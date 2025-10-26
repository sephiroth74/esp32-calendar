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
// CONFIGURATION SYSTEM
// =============================================================================
/**
 * IMPORTANT: Configuration is now stored in LittleFS filesystem
 *
 * Before deploying, you must:
 * 1. Edit 'data/config.json' with your settings
 * 2. Upload filesystem: pio run -t uploadfs
 * 3. Upload firmware: pio run -t upload
 *
 * The config.json file contains:
 * - WiFi credentials (SSID and password)
 * - Location coordinates for weather
 * - Calendar URL and settings
 * - Display timezone and update schedule
 *
 * Note: config.json is excluded from git to protect your sensitive data
 */

// Default values used when LittleFS config is missing
#define LOC_LATITUDE 40.7128
#define LOC_LONGITUDE -74.0060

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

    // Weekend cell background (will be dithered)
    #define COLOR_CALENDAR_WEEKEND_BG      GxEPD_YELLOW
    #define DITHER_CALENDAR_WEEKEND        25  // 25% dithering for weekend cells

    // Previous/next month days dithering
    #define DITHER_CALENDAR_OUTSIDE_MONTH  20  // 20% dithering for prev/next month days

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
// FONT CONFIGURATION
// =============================================================================
// Configure fonts for each display element
// Available font families: Ubuntu_R, Luna_ITC_Regular, Luna_ITC_Std_Bold,
//                         Montserrat_Regular, Lato_Regular, Roboto_Regular, etc.
// Available sizes: 4pt, 5pt, 6pt, 7pt, 8pt, 9pt, 10pt, 11pt, 12pt, 14pt, 16pt,
//                 18pt, 20pt, 22pt, 24pt, 26pt, 32pt (not all sizes for all fonts)

// Calendar Header Fonts
#define FONT_HEADER_DAY_NUMBER Luna_ITC_Std_Bold32pt7b   // Large day number in header
#define FONT_HEADER_MONTH_YEAR Luna_ITC_Regular26pt7b    // Month and year text
#define FONT_SUNRISE_SUNSET Ubuntu_R_7pt8b               // Sunrise/sunset times in header

// Calendar Grid Fonts
#define FONT_CALENDAR_DAY_LABELS Luna_ITC_Std_Bold9pt7b  // Day of week labels (M, T, W, etc.)
#define FONT_CALENDAR_DAY_NUMBERS Luna_ITC_Std_Bold12pt7b // Day numbers in calendar grid
#define FONT_CALENDAR_OUTSIDE_MONTH Luna_ITC_Regular12pt7b // Previous/next month days

// Events Section Fonts
#define FONT_EVENT_DATE_HEADER Luna_ITC_Std_Bold12pt7b // Date headers (Today, Tomorrow, etc.)
#define FONT_EVENT_TIME Ubuntu_R_9pt8b                    // Event time display
#define FONT_EVENT_TITLE Ubuntu_R_9pt8b                   // Event title text
#define FONT_EVENT_LOCATION Ubuntu_R_7pt8b                // Event location (if shown)
#define FONT_EVENT_DETAILS Ubuntu_R_9pt8b                 // Event details and more events text
#define FONT_NO_EVENTS Luna_ITC_Regular14pt7b             // "No Events" message

// Weather Section Fonts
#define FONT_WEATHER_TEMP_MAIN Montserrat_Regular_14pt8b  // Main temperature display
#define FONT_WEATHER_TEMP_HOURLY Montserrat_Regular_9pt8b // Hourly forecast temps
#define FONT_WEATHER_TIME Ubuntu_R_9pt8b                  // Sunrise/sunset times
#define FONT_WEATHER_MESSAGE Luna_ITC_Regular9pt7b        // Weather status messages

// Error Display Fonts
#define FONT_ERROR_TITLE Luna_ITC_Std_Bold18pt7b         // Error title
#define FONT_ERROR_MESSAGE Luna_ITC_Regular12pt7b        // Error message text
#define FONT_ERROR_DETAILS Ubuntu_R_9pt8b                // Error details/codes

// Legacy compatibility (deprecated - use new font defines above)
#define EVENT_BODY_FONT FONT_EVENT_TITLE
#define EVENT_HEADER_FONT FONT_EVENT_DATE_HEADER

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
// DEFAULT CALENDAR CONFIGURATION
// =============================================================================
// These defaults are used only when config.json is missing or incomplete

// Default calendar URL (used as fallback)
#define DEFAULT_CALENDAR_URL "YOUR_CALENDAR_ICS_URL"

// Default number of days to fetch events
#define DEFAULT_DAYS_TO_FETCH 30

// =============================================================================
// UPDATE & REFRESH CONFIGURATION
// =============================================================================

// Daily update time and timezone are now configured in data/config.json
// Default values (used only if config.json is missing):
#define DEFAULT_UPDATE_HOUR 5                        // 5:00 AM
#define DEFAULT_TIMEZONE "EST5EDT,M3.2.0,M11.1.0"   // US Eastern

// Error retry intervals (in minutes)
// How long to wait before retrying after specific errors
#define WIFI_ERROR_RETRY_MINUTES 60        // Retry after 1 hour if WiFi fails
#define CALENDAR_ERROR_RETRY_MINUTES 120   // Retry after 2 hours if calendar fetch fails
// Note: Battery low error will not set a wake-up timer (sleep indefinitely)

// =============================================================================
// DISPLAY CONTENT CONFIGURATION
// =============================================================================

// Maximum number of events to show on the display
#define MAX_EVENTS_TO_SHOW 10

// Maximum number of calendars allowed (hardware/memory limitation)
#define MAX_CALENDARS 3

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
#define NTP_SERVER_1 "0.europe.pool.ntp.org"
#define NTP_SERVER_2 "pool.ntp.org"


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

// =============================================================================
// DEBUG & TESTING CONFIGURATION
// =============================================================================

// Disable deep sleep for testing
// When true, device stays awake to test button presses and configuration
// Set to false for production use to enable battery-saving deep sleep
#define DISABLE_DEEP_SLEEP false

// Configuration reset button hold time (milliseconds)
// How long to hold the button to reset WiFi configuration during runtime
#define CONFIG_RESET_HOLD_TIME 3000

#endif // CONFIG_H