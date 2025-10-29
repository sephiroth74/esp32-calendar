# Changelog

All notable changes to the ESP32 E-Paper Calendar project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.7.6] - 2025-10-29

### Added
- **Display Capabilities Test** - New debug menu option (16) to test display
  - Shows comprehensive display information: resolution, pages, color support
  - Displays partial update and fast partial update capabilities
  - Shows display type (Black & White or 6-Color)
  - Color test screen with vertical bars showing all supported colors
  - For B&W displays: Shows 4 dithering levels (10%, 25%, 50%, 75%) plus Black/White/Dark Grey/Light Grey
  - For color displays: Shows all 7 colors (Black, White, Red, Yellow, Orange, Dark Grey, Light Grey)
  - Two-stage test: info screen first, then color/dithering test after keypress

### Fixed
- **Native Tests** - Fixed all compiler errors and infinite loops in native test environment
  - Added missing methods to mock Arduino classes (printf, startsWith, remove, mkdir, etc.)
  - Fixed infinite loop in `replaceAll()` when handling empty "from" string
  - Fixed `truncate()` edge cases when maxLength is smaller than suffix length
  - Fixed `toTitleCase()` to treat all non-alphabetic characters as word delimiters
  - Fixed `millis()` mock to return incrementing values instead of always 0
  - Fixed UTF-8 character handling in tests (changed from "…" to "...")
  - All 29 native test cases now pass successfully (127 assertions)
  - Tests can now be run with standard `pio test -e native` command

### Changed
- **TeeStream Implementation** - Added virtual destructor and readString() method
- **Mock String Class** - Enhanced with bounds checking and safer character access
- **Test Infrastructure** - Disabled outdated `test_calendar_stream_parser.cpp` (tested old API)

## [1.7.5] - 2025-10-25

### Changed
- **Weather Layout Refinements** - Fine-tuned weather section positioning
  - Weather forecast hours section moved down 2 pixels for better spacing
  - Improved visual alignment with main weather display
  - Better separation between weather elements

## [1.7.4] - 2025-10-25

### Changed
- **Sunrise/Sunset Icons** - Replaced text arrows with proper weather icons
  - Now uses wi_sunrise_24x24 bitmap for sunrise indicator
  - Now uses wi_sunset_24x24 bitmap for sunset indicator
  - Icons positioned at Y=8, text at Y=22 for better alignment
  - Improved visual consistency with other weather elements
- **Weather Icon Positioning** - Adjusted main weather icon
  - Today's weather icon moved down 6 pixels (Y position from -20 to -14)
  - Better visual balance with temperature display

## [1.7.3] - 2025-10-25

### Fixed
- **Sunrise/Sunset Visibility** - Fixed display issues in header
  - Added explicit black text color for sunrise/sunset times
  - Adjusted position to Y=25 for better visibility
  - Added arrow indicators (↑ for sunrise, ↓ for sunset)
  - Increased horizontal padding to 15px from edges

### Changed
- **Header Layout** - Fine-tuned positioning
  - Moved day number up 6 pixels (Y position from 10 to 4)
  - Better vertical balance with month text
- **Weather Section** - Adjusted positioning
  - Moved weather row down 6 pixels for better spacing
  - Adjusted separator line position to maintain visual balance

## [1.7.2] - 2025-10-25

### Added
- **Sunrise/Sunset Display in Header** - Shows sun times in header corners
  - Sunrise time displayed at top-left corner of header
  - Sunset time displayed at top-right corner of header
  - New FONT_SUNRISE_SUNSET configuration for customizable font
  - Automatically extracts times from weather data when available
- **Events/Weather Separator** - Visual separator between sections
  - Horizontal line between events list and weather forecast
  - 15px spacing above weather section for cleaner layout

### Changed
- Updated drawModernHeader to accept WeatherData parameter
- Improved header layout with sun times integration

### Fixed
- Removed duplicate FONT_SUNRISE_SUNSET definition
- Fixed compilation warnings

## [1.7.1] - 2025-10-25

### Changed
- **Weather Section Layout** - Simplified weather display
  - Removed sunrise/sunset row for cleaner appearance
  - Decreased weather icon size from 64x64 to 48x48
  - Moved main temperature display down and closer to icon
  - Reduced spacing between weather icon and hourly forecast
- **Header Layout Adjustments** - Improved header spacing
  - Moved day number text up 20px for better balance
  - Moved month/year text up 40px to be closer to day number
  - Adjusted separator line position using font metrics

### Fixed
- Fixed compilation errors with nullptr font references
- Removed unused variable warnings

## [1.7.0] - 2025-10-25

### Added
- **Configurable Font System** - Fonts can now be configured for each display element
  - Each calendar element can use a different font (header, calendar, events, weather, status, etc.)
  - Comprehensive font configuration in config.h with 20+ font defines
  - Improved layout flexibility with font-based calculations
- **Font Metrics System** - Dynamic positioning based on actual font dimensions
  - `getFontHeight()` - Calculate font height from font data
  - `getFontBaseline()` - Calculate baseline position for proper alignment
  - `getTextWidth()` - Calculate actual text width for centering
  - `calculateYPosition()` - Calculate next Y position based on font metrics
  - `drawTextWithMetrics()` - Draw text with automatic centering and metrics
- **StringUtils Class** - Dedicated string manipulation utilities
  - `convertAccents()` - Convert Unicode accented characters to ASCII
  - `removeAccents()` - Alias for convertAccents
  - `truncate()` - Truncate strings with ellipsis
  - `trim()` - Remove leading/trailing whitespace
  - `replaceAll()` - Replace all occurrences of substring
  - `startsWith()` / `endsWith()` - String boundary checks
  - `toTitleCase()` - Convert to title case
  - Comprehensive unit tests for all methods

### Changed
- Migrated from ICSParser to CalendarStreamParser for all calendar operations
- Improved RRULE expansion for all recurring event types (YEARLY, MONTHLY, WEEKLY, DAILY)
- Event filtering now happens during parsing for better memory efficiency
- Removed debug logging added for troubleshooting
- **Display Manager Refactoring** - All hardcoded fonts replaced with configurable defines
  - Replaced Luna_ITC_Std_Bold32pt7b with FONT_HEADER_DAY_NUMBER
  - Replaced Ubuntu_R_12pt8b with FONT_EVENT_DATE_HEADER
  - Replaced all inline font references with configuration macros
  - Updated drawModernHeader to use font metrics for dynamic positioning
  - Updated drawEventsSection to use font metrics for proper text centering

### Fixed
- Personal Calendar events not displaying (non-recurring events were being filtered incorrectly)
- Birthday events not showing (FREQ=YEARLY events now properly expand to current year)
- Holiday events with recurring rules now display correctly
- "Nessun Evento" text now properly centered both horizontally and vertically

### Removed
- ICSParser completely removed in favor of CalendarStreamParser
- Removed temporary debug logging used for issue diagnosis

## [1.6.1] - 2025-10-22

### Added
- **Streaming ICS Parser** - Implemented streaming approach for parsing large ICS files
  - `parseInChunks()` method for processing large String data in chunks
  - `parseStreamLarge()` for direct stream parsing with state machine
  - `fetchStream()` method in CalendarFetcher for HTTP and file streaming
  - Prevents memory overflow when parsing files larger than 30KB
  - Successfully handles Google Calendar files up to 165KB+
- **Unified Test Framework** - Fixed embedded test infrastructure
  - Created `test_main.cpp` as single entry point for all embedded tests
  - Added `run_tests.sh` script for automated test execution
  - Created comprehensive `TEST_INSTRUCTIONS.md` documentation
  - Resolved multiple definition conflicts in Unity test framework

### Changed
- CalendarWrapper now automatically uses streaming for remote URLs
- CalendarFetcher supports both traditional fetch and streaming modes
- ICSParser `parseRRule` method made public for test accessibility
- Test configuration updated to properly handle source file inclusion

### Fixed
- **Critical Bug**: ICS parser skipping thousands of events in large files
  - Parser was jumping from line 27 to line 4113 in 165KB files
  - Root cause: Arduino String memory limitations with large data
  - Solution: Event-by-event parsing without loading entire file into memory
- **Test Framework**: Multiple definition errors for setup() and loop()
  - Conflicting definitions between test files and source files
  - Solution: Unified test runner with automatic file management
- Compiler warnings in PSRAM test file (unused variable)
- Calendar count validation to prevent exceeding MAX_CALENDARS limit
- Infinite restart loop on configuration errors (now uses deep sleep)

### Performance
- Memory usage significantly reduced when parsing large ICS files
- Events parsed individually with 8KB buffer limit per event
- Periodic yielding (delay) to prevent watchdog resets
- Streaming approach eliminates String memory exhaustion

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