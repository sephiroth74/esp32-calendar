# Changelog
All notable changes to the ESP32 E-Paper Calendar project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Planned
- OTA (Over-The-Air) update support
- CalDAV full implementation
- Multiple calendar source aggregation
- QR code for WiFi configuration

## [1.3.0] - 2024-12-19
### Added
- **Once-Daily Update System**
  - Calendar now updates only once per day at configurable hour (DAY_START_HOUR)
  - Significant battery life improvement with reduced wake cycles
  - Removed multiple daily updates and quiet hours complexity

- **Intelligent Error Recovery**
  - WiFi errors: Automatic retry after 1 hour (WIFI_ERROR_RETRY_MINUTES)
  - Calendar fetch errors: Retry after 2 hours (CALENDAR_ERROR_RETRY_MINUTES)
  - Battery critical: Deep sleep without wake timer (manual intervention required)
  - Error-specific retry intervals for optimal recovery

- **Secure Configuration System**
  - Sensitive data moved to separate `config.local` file
  - WiFi credentials and location coordinates protected from git
  - Template file (`config.local.example`) for easy setup
  - Automatic .gitignore configuration for security

- **Enhanced Button Noise Filtering**
  - Hardware debouncing recommendations with circuit diagrams
  - Software noise validation after wake-up
  - Option to disable button wake-up if noise persists
  - Stronger pull-down configuration for stability

### Changed
- **Power Management Overhaul**
  - Removed UPDATE_INTERVAL_MINUTES constant (now always 24 hours)
  - Removed DAY_END_HOUR constant (no more quiet hours)
  - Simplified sleep duration calculation
  - More predictable wake-up schedule

- **Configuration Structure**
  - config.h now includes config.local for sensitive data
  - Reorganized config.h with clear sections and documentation
  - Removed unused constants (NTP servers, battery thresholds)
  - Added comprehensive comments for all settings

### Fixed
- **Button Wake-Up Issues**
  - Fixed continuous reboot caused by electrical noise on button pin
  - Added noise detection and immediate re-sleep if spurious wake
  - Improved button state validation before sleep
  - Hardware recommendations for proper debouncing

- **Build System**
  - Removed missing header file references (pinout.h, debug.h)
  - Fixed function signatures and forward declarations
  - Corrected CalendarClient API usage
  - Resolved compilation errors with refactored code

### Security
- WiFi passwords never committed to version control
- Location data kept private and local
- Safe to share code publicly without exposing credentials
- Clear documentation for secure setup

### Documentation
- Added CONFIG_SETUP.md with detailed configuration instructions
- Added BUTTON_NOISE_FIX.md with hardware solutions
- Added CONFIG_REFACTORING.md documenting cleanup
- Updated all inline documentation for clarity

### Technical Improvements
- Cleaner code organization with removed debug conditionals
- Simplified main.cpp with better error handling
- Reduced code complexity by removing interval-based updates
- Better separation of concerns with local configuration

## [1.2.0] - 2024-10-18
### Added
- **6-Color E-Paper Display Support**
  - Full support for Good Display 7.5" 6-color displays (GDEP073E01)
  - Conditional color rendering for enhanced visual hierarchy
  - Automatic fallback to B/W rendering for monochrome displays

- **Configurable Color Scheme**
  - All colors now defined as constants in config.h for easy customization
  - COLOR_CALENDAR_TODAY_TEXT - Today's date number color
  - COLOR_CALENDAR_TODAY_BORDER - Today's border color
  - COLOR_CALENDAR_OUTSIDE_MONTH - Previous/next month days
  - COLOR_CALENDAR_WEEKEND_BG - Weekend cell background
  - COLOR_CALENDAR_DAY_LABELS - Day of week labels
  - COLOR_EVENT_TODAY_HEADER - "Oggi/Today" header
  - COLOR_EVENT_TOMORROW_HEADER - "Domani/Tomorrow" header
  - COLOR_EVENT_OTHER_HEADER - Other date headers
  - COLOR_WEATHER_ICON - Weather icon color
  - COLOR_HEADER_DAY_NUMBER - Large day number in header

- **Enhanced Event Display**
  - Event date formatting with intelligent grouping (Today/Tomorrow/Day Date)
  - Shows actual event dates when no events today/tomorrow
  - Support for events spanning different months/years
  - Improved date header formatting for better readability
  - Dynamic event limit based on available space
  - Overflow indicator showing count of hidden events
  - Word-boundary text wrapping for cleaner event titles

- **Improved Power Management**
  - ESP32 now goes to deep sleep immediately after setup
  - Loop function emptied - never reached during normal operation
  - 10-second viewing delay only for undefined wakeup (first boot/reset)
  - No delay for timer or button wakeups (immediate sleep)
  - Significant power consumption reduction

### Changed
- **Display Improvements**
  - Weather icon increased to 96x96 pixels for better visibility
  - Weather icon positioned higher with better centering
  - Sunrise/sunset row moved up for optimal spacing
  - Header spacing increased (day number and month/year)
  - Month view moved down 10px to prevent header overlap
  - HEADER_HEIGHT increased from 110 to 120 pixels
  - Event list fonts: Luna Bold 12pt for dates, Ubuntu R 9pt for events
  - Left header now centered within panel
  - Month/year header uses Luna Regular 26pt font
  - Day number uses Luna Bold 32pt font
  - Status bar now shows current date/time alongside version

- **Color Display Enhancements** (6-color displays only)
  - Weekend cells: 25% yellow dithered pattern (subtle background)
  - Current day: Red text with red border
  - Outside month days: Green text (was yellow)
  - Day labels (M,T,W...): Red text
  - Today's events header: Red
  - Tomorrow's events header: Orange
  - Other date headers: Green
  - Weather icon: Red
  - Large header day number: Red

- **24-Hour Time Format Support**
  - Events display in 24-hour format when TIME_FORMAT_24H is true
  - Automatic formatting with leading zeros (e.g., "09:30", "15:45")
  - Seamless switching between 12/24 hour formats

### Fixed
- **Event Display Issues**
  - Fixed filterPastEvents removing today's events that already occurred
  - Now keeps ALL of today's events regardless of time
  - Events from past days still filtered out correctly
  - "Ritirare la moto" and other past events now display properly
  - Event parsing improved with better debug logging

- **Timezone Handling**
  - Fixed UTC to local time conversion
  - System now uses proper timezone string with automatic DST
  - Removed manual TIMEZONE_OFFSET and DAYLIGHT_SAVING calculations
  - More reliable event time display

- **Display Layout Issues**
  - Fixed header text overlap with separator line
  - Corrected month view positioning
  - Improved spacing between date groups in events list
  - Better visual hierarchy throughout interface

### Technical Improvements
- Extensive debug logging for event parsing and display
- Improved UTC to local time conversion logging
- Better error handling for ICS parsing
- Code optimization with color constants
- Version bumped to 1.2.0

## [1.1.0] - 2024-10-16
### Added
- **Weather Display Feature**
  - Open-Meteo API integration for real-time weather data
  - 7-day forecast with 3-hour intervals (6am to midnight)
  - Current conditions with temperature and weather icon
  - Sunrise/sunset times with dedicated icons
  - Weather icons for all conditions (sunny, cloudy, rain, snow, etc.)
  - Automatic weather updates with calendar refresh
  - Weather placeholder when data unavailable

- **Button Wake-Up Support**
  - Manual refresh button on GPIO pin 2
  - EXT1 wake-up source for instant display update
  - Pull-down resistor configuration for stable operation
  - Wake reason detection and logging

- **Enhanced Display Layout**
  - Modern split-screen design with improved spacing
  - Larger fonts for better readability (Luna ITC, Montserrat, Ubuntu families)
  - Today's date outlined instead of filled for better visibility
  - Improved event text wrapping with proper multi-line support
  - Weekend cells with 10% gray dithering
  - Previous/next month days with 20% gray dithering

- **Battery Monitoring Improvements**
  - ADC-based battery voltage reading (GPIO pin 1)
  - LiPo discharge curve for accurate percentage calculation
  - Low battery warnings and critical battery shutdown
  - Real battery values in debug tests

### Changed
- Month header now shows correct month (fixed array indexing issue)
- All-day events now display "--" instead of "All Day" to save space
- Calendar day labels now use localized strings
- Removed intermediate status messages during startup (cleaner boot)
- Weather forecast moved up 10px for better visual balance
- Status bar moved down 5px to bottom edge
- Sunrise/sunset row repositioned for optimal spacing
- Updated mock data to use October 16th as current date

### Fixed
- Today's cell outline now properly highlights current day
- Month display showing wrong month (September vs October)
- Event text second line alignment issues
- Weather icon bitmap function optimized with macro (56% code reduction)
- Italian localization missing error message constants
- Main.cpp updated to use showModernCalendar() method
- Debug mode test functions using real WiFi/battery values

### Technical Improvements
- Platform upgraded to ESP32-S3 with 4MB flash, 2MB PSRAM
- Display pins updated for ESP32-S3 compatibility
- Weather icons in multiple sizes (16x16, 24x24, 32x32, 48x48, 64x64)
- Improved memory usage with weather data caching
- Code size optimization through macro usage
- Enhanced error handling for weather API failures

### Known Issues
- CalDAV implementation incomplete
- No WiFi provisioning UI

## [1.0.0] - 2024-10-14
### Added
- Initial release of ESP32 E-Paper Calendar
- Support for Waveshare 7.5" black and white e-paper display
- Split-screen layout with month calendar and event list
- Month calendar view with:
  - Current month grid display
  - Event indicators (dots) on days with events
  - Today's date highlighting
  - Day of week labels
- Event list panel showing:
  - Today's events
  - Tomorrow's events
  - Day after tomorrow's events
  - Event times and locations
  - "All Day" event support
- Multiple calendar source support:
  - ICS URL (Google Calendar, iCloud, Outlook)
  - Google Calendar API (with OAuth2)
  - CalDAV (partial implementation)
- Localization system:
  - Compile-time language selection
  - English and Italian languages included
  - Easy framework for adding new languages
- WiFi connectivity:
  - Automatic connection management
  - Reconnection on failure
  - Signal strength indicator
- Power management:
  - Deep sleep between updates
  - Configurable update interval (default 24 hours)
  - Battery voltage monitoring (MAX17048)
  - Low power consumption design
- Time synchronization:
  - NTP time sync
  - Timezone support with DST
  - Configurable timezone offset
- Configuration system:
  - Centralized configuration file (config.h)
  - Easy WiFi credential setup
  - Calendar source selection
  - Display customization options
- Status indicators:
  - WiFi connection status
  - Battery level display
  - Last update timestamp
- Snake_case file naming convention
- Clean modular architecture:
  - WiFi Manager
  - Display Manager
  - Calendar Client
  - Localization support

### Technical Details
- Platform: ESP32-C6 (Seeed XIAO ESP32C6)
- Framework: Arduino with PlatformIO
- Display: GxEPD2 library for e-paper control
- JSON: ArduinoJson for API parsing
- HTTP: ESP32 HTTPClient for web requests
- Memory usage: ~82% Flash, ~13% RAM

### Configuration
- Default update interval: 24 hours
- Default timezone: UTC+2
- Maximum events displayed: 10
- Days to fetch ahead: 7

### Known Issues
- CalDAV implementation incomplete
- No touch/button input support yet
- No WiFi provisioning UI

## Version History

### Versioning Scheme
- MAJOR version: Incompatible API/hardware changes
- MINOR version: New backwards-compatible features
- PATCH version: Backwards-compatible bug fixes

---
*For more information, visit the [project repository](https://github.com/sephiroth74/esp32-calendar)*