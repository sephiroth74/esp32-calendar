# Changelog

All notable changes to the ESP32 E-Paper Calendar project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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