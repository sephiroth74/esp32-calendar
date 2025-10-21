# Changelog

All notable changes to the ESP32 E-Paper Calendar project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.6.0] - 2025-10-21

### Added
- **New Calendar Architecture** - Complete rewrite of calendar parsing system
  - `CalendarFetcher` class for fetching ICS data from remote URLs or local files
  - `ICSParser` class with RFC 5545 compliant parsing
  - `CalendarWrapper` class combining parser with per-calendar configuration
  - `CalendarManager` class for managing multiple calendars
  - `CalendarDisplayAdapter` for compatibility between new CalendarEvent and DisplayManager
- **BatteryMonitor Class** - Extracted battery monitoring into dedicated class
  - Encapsulated ADC reading and voltage calculation
  - LiPo discharge curve for accurate percentage calculation
  - Status checking methods (isLow, isCritical, isCharging)
  - Debug output and formatted status strings
- **Enhanced WiFiManager** - Moved NTP time sync functionality to WiFiManager
  - `syncTimeFromNTP()` method with configurable timezone and NTP servers
  - Time configuration state tracking
  - Better separation of network-related concerns
- **Comprehensive Testing** - Added 63+ unit tests using doctest framework
  - Mock Arduino infrastructure for native testing
  - Tests for all ICS parsing scenarios
  - RFC 5545 date-time format compliance tests
  - Memory management and PSRAM allocation tests

### Changed
- Calendar configuration now supports per-calendar `days_to_fetch` setting
- Improved PSRAM allocation with simplified macro approach
- Display manager now uses forward declaration for CalendarEvent
- Main.cpp significantly cleaned up with better separation of concerns
- All compiler warnings fixed with proper buffer sizing

### Fixed
- Memory management issues with proper delete instead of free
- Buffer truncation warnings in date/time formatting functions
- ICS line folding/unfolding per RFC 5545 specification
- Recurring event expansion within date ranges
- All-day event detection and handling
- Timezone support for DTSTART with TZID parameter

### Removed
- Old `calendar_client.cpp/h` implementation (moved to backup)
- Battery-related functions from main.cpp (moved to BatteryMonitor)
- NTP sync function from main.cpp (moved to WiFiManager)
- Direct WiFi calls from main.cpp (now uses WiFiManager)

### Technical Details
- New modular architecture with clear separation of concerns
- Support for both HTTP and local file fetching with automatic detection
- Caching functionality for remote calendars
- PSRAM support with automatic fallback to heap
- Clean build with no warnings (except third-party libraries)

## [1.5.0] - 2025-10-20

### Added
- **RRULE Support for Recurring Events** - Full support for yearly recurring events (birthdays, anniversaries)
- ICS parser now recognizes and processes RRULE:FREQ=YEARLY patterns
- Recurring events automatically generate occurrences for current year
- Support for multi-year recurring events when date range extends beyond 365 days
- Birthday calendars now work correctly with events repeating every year

### Fixed
- Recurring events (like birthdays from past years) now show on their anniversary dates
- Events with RRULE:FREQ=YEARLY now properly display in current year
- October 26 "l'ora legale termina" recurring event now appears correctly

### Technical Details
- RRULE parsing extracts frequency patterns from ICS events
- Yearly recurring events have their year adjusted to current year
- When fetching >365 days, next year's occurrences are also included

## [1.4.2] - 2025-10-20

### Added
- **Local ICS File Support** - Calendars can now be loaded from ICS files stored in LittleFS
- Support for `local://` prefix and `/` paths to indicate local files
- Enhanced debug logging for event date filtering to diagnose missing events
- Documentation for local ICS file setup and configuration

### Fixed
- Added detailed timestamp debugging to help identify date filtering issues
- Improved event parsing debug output

## [1.4.1] - 2025-10-20

### Added
- **Colored Event Indicators** on 6-color displays - Calendar events now show colored circles based on calendar source
- Support for up to 3 different calendar colors per day on the month view
- Maximum calendar limit set to 3 for memory and display optimization

### Changed
- "Nessun Evento" (No Events) text now uses Montserrat 16pt font for better visibility
- Event indicators on calendar cells now use calendar-specific colors on 6-color displays
- Calendar loading enforces 3 calendar maximum limit

### Fixed
- Improved event indicator positioning when multiple calendars have events on the same day

## [1.4.0] - 2025-10-20

### Added
- **Multiple Calendar Support** - Fetch and merge events from multiple ICS calendar sources
- Calendar configuration now supports array of calendars with individual settings
- Each calendar can have a name, URL, color code, and enabled/disabled status
- Events are automatically merged and sorted chronologically from all enabled calendars
- Calendar source metadata (name, color) added to each event for future display features
- Example configuration files for both single and multiple calendar setups
- Backward compatibility maintained for existing single calendar configurations

### Changed
- Updated configuration structure to support multiple calendars while maintaining backward compatibility
- CalendarClient now provides `fetchMultipleCalendars()` method for aggregating events
- Configuration loader handles both legacy single calendar and new multiple calendar formats

## [1.3.1] - 2025-10-20

### Fixed
- Fixed "Today" (Oggi) label not showing for today's events on 6-color displays
- Fixed "Tomorrow" (Domani) label not visible for tomorrow's events
- Fixed future event date headers (e.g., "November 14") not visible due to orange color on 6-color displays
- Changed future event headers from ORANGE to BLACK for better visibility on 6-color e-paper displays

### Changed
- Moved month/year header down 10 pixels for better visual spacing
- Moved calendar separator line down 10 pixels to align with header changes
- Adjusted calendar table position down 10 pixels (CALENDAR_START_Y from HEADER_HEIGHT+10 to HEADER_HEIGHT+20)
- Changed weekend cell backgrounds from 25% to 50% yellow using checkerboard dithering pattern for better visibility
- Improved color management with explicit color resets after printing headers

### Removed
- Removed config.local system in favor of LittleFS-based configuration
- Removed hardcoded configuration constants (DAY_START_HOUR, TIMEZONE, CALENDAR_TYPE, ICS_CALENDAR_URL, etc.)
- All configuration now managed through config.json in LittleFS filesystem

## [1.3.0] - 2025-10-14

### Added
- LittleFS filesystem support for configuration storage
- JSON-based configuration file (config.json) for all settings
- Runtime configuration loading from JSON file
- Default configuration values for missing config.json entries
- Support for 6-color e-paper displays (DISP_TYPE_6C)
- Configurable color scheme for different display elements
- Weather forecast display with 7-day hourly predictions
- Battery monitoring with visual indicators
- WiFi signal strength display with RSSI values

### Changed
- Migrated from compile-time constants to runtime configuration
- Improved event parsing for ICS calendar format
- Enhanced display layout with split-screen design
- Updated error handling with comprehensive error codes
- Improved localization support (Italian and English)

### Fixed
- ICS date/time parsing with proper timezone handling
- Memory management using PSRAM for large calendar data
- Display refresh optimization to reduce ghosting

## [1.2.0] - 2025-09-10

### Added
- Deep sleep mode with configurable wake intervals
- Button wake-up for manual refresh
- Floyd-Steinberg dithering for grayscale effects
- Event indicators on calendar cells
- Full-screen error display with icons
- Localized error messages

### Changed
- Optimized power consumption with smart sleep modes
- Improved calendar grid layout
- Enhanced event filtering logic
- Updated display refresh timing

## [1.1.0] - 2025-08-05

### Added
- Google Calendar API support
- CalDAV protocol support (partial implementation)
- Multiple calendar source types
- Configurable timezone support
- Event location display

### Changed
- Refactored calendar client architecture
- Improved date/time formatting
- Enhanced event sorting algorithm

## [1.0.0] - 2025-07-01

### Added
- Initial release
- ICS calendar feed support
- 7.5" e-paper display support (800x480)
- ESP32-S3 platform support
- WiFi connectivity
- NTP time synchronization
- Monthly calendar view with events
- Split-screen layout (calendar + events)
- Configurable first day of week
- Basic error handling
- Serial debug output