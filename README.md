# ESP32 E-Paper Calendar Display

A feature-rich calendar display using ESP32 (S3/C6) and Waveshare/Good Display 7.5" e-paper displays (B/W or 6-color) with split-screen layout, comprehensive error handling, and multiple language support.

**Version 1.7.8** - Improved code organization and compilation fixes!

## What's New in v1.7.8

- 🔧 **Fixed Critical Compilation Error**: Resolved ESP32 assembler jump range error
- 📁 **Improved Code Organization**: Split display_manager.cpp into specialized modules for better maintainability
- 🎨 **Icon Library Optimization**: Reduced icon library size by 95% (15,673 lines → 734 lines)
- 🧹 **Cleaner Build Configuration**: Removed conflicting compiler flags
- 📚 **Better Documentation**: Added ICONS_TO_KEEP.md for future maintenance

See [CHANGELOG.md](platformio/CHANGELOG.md) for complete details.

## Features

### Display Features
- **Multi-Display Support**: Works with both B/W and 6-color e-paper displays
- **Split-Screen Layout**: Month calendar on left, event list on right
- **6-Color Display Enhancements**: Configurable color scheme for visual hierarchy
- **Floyd-Steinberg Dithering**: Weekend cells (25% yellow on color, 10% gray on B/W)
- **Event Indicators**: Small dots in calendar cells with events
- **Smart Event Grouping**: Today, Tomorrow, and upcoming events sections with date headers
- **Large Weather Icon**: 96x96 pixel weather display with current conditions
- **Embedded Icons**: High-quality bitmap icons for errors, warnings, and status indicators

### Calendar Features
- **Multiple Calendar Sources**: ICS URLs (Google Calendar, iCloud, Outlook), Google Calendar API, CalDAV
- **Smart Event Filtering**: Shows ALL of today's events (even if passed), plus future events
- **Timezone Support**: Automatic timezone conversion with DST support
- **First Day of Week**: Configurable (Sunday/Monday)
- **24-Hour Time Format**: Optional 24-hour time display for events

### System Features
- **Comprehensive Error Handling**: 40+ error codes with visual feedback and localization
- **Multi-Language Support**: English and Italian (easily extensible)
- **Enhanced Power Management**: Immediate deep sleep after display update
- **Battery Monitoring**: Real-time battery status with accurate LiPo curve
- **WiFi Status**: Connection indicator with RSSI display
- **PSRAM Support**: Utilizes ESP32-S3's PSRAM for large calendar data
- **Debug Mode**: Interactive serial menu for testing all features without WiFi
- **Semantic Versioning**: Built-in version management for OTA updates
- **Button Wake-Up**: Manual refresh via button press (GPIO 2)
- **Weather Integration**: Real-time weather with Open-Meteo API

## Hardware Requirements

- **ESP32 Board**:
  - ESP32-S3 (recommended - includes PSRAM for large calendar data)
  - ESP32-C6 (supported)
  - Minimum 4MB flash, 2MB PSRAM recommended
- **Display Options**:
  - **B/W Display**: Waveshare 7.5" (800x480) - Models: V2, V3, GDEY075T7
  - **6-Color Display**: Good Display 7.5" GDEP073E01 (800x480) - Black, White, Red, Yellow, Orange, Green
- **Optional Components**:
  - Wake-up button (connected to GPIO 2)
  - LiPo battery for portable operation
  - Voltage divider for battery monitoring (GPIO 1)

## Wiring

### ESP32-S3 Connections
| E-Paper | ESP32-S3 | Description |
|---------|----------|-------------|
| VCC     | 3.3V     | Power       |
| GND     | GND      | Ground      |
| DIN     | GPIO 11  | SPI MOSI    |
| CLK     | GPIO 12  | SPI Clock   |
| CS      | GPIO 7   | Chip Select |
| DC      | GPIO 5   | Data/Command|
| RST     | GPIO 6   | Reset       |
| BUSY    | GPIO 4   | Busy Signal |

### ESP32-C6 Connections
| E-Paper | ESP32-C6 | Description |
|---------|----------|-------------|
| VCC     | 3.3V     | Power       |
| GND     | GND      | Ground      |
| DIN     | GPIO 10  | SPI MOSI    |
| CLK     | GPIO 8   | SPI Clock   |
| CS      | GPIO 7   | Chip Select |
| DC      | GPIO 5   | Data/Command|
| RST     | GPIO 6   | Reset       |
| BUSY    | GPIO 4   | Busy Signal |

## Configuration

### 🔐 Initial Setup - IMPORTANT!

Before compiling, you must create your local configuration file:

```bash
cd include/
cp config.local.example config.local
```

Edit `include/config.local` with your WiFi credentials and location:

```cpp
// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_NETWORK"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// Location for weather (find at https://www.latlong.net/)
#define LOC_LATITUDE 40.7128    // Example: New York
#define LOC_LONGITUDE -74.0060   // Example: New York
```

**Note:** `config.local` is git-ignored, so your credentials stay private!

### 2. Calendar Source

Edit `include/config.h` to configure your calendar source:

Choose one of three options:

#### Option A: ICS URL (Simplest)
Most calendar services provide an ICS URL for read-only access:

```cpp
#define CALENDAR_TYPE "ICS"
#define ICS_CALENDAR_URL "YOUR_CALENDAR_ICS_URL"
```

**How to get ICS URLs:**
- **Google Calendar**: Settings → Calendar → Integrate → Secret address in iCal format
- **iCloud**: Calendar app → Share Calendar → Public Calendar → Copy link
- **Outlook**: Settings → View all Outlook settings → Calendar → Shared calendars → Publish a calendar → Copy ICS link

#### Option B: Google Calendar API
```cpp
#define CALENDAR_TYPE "GOOGLE"
#define GOOGLE_CALENDAR_ID "primary"
#define GOOGLE_API_KEY "YOUR_API_KEY"
// Additional OAuth tokens required
```

#### Option C: CalDAV
```cpp
#define CALENDAR_TYPE "CALDAV"
#define CALDAV_SERVER "https://caldav.icloud.com"
#define CALDAV_USERNAME "YOUR_USERNAME"
#define CALDAV_PASSWORD "YOUR_APP_PASSWORD"
```

### 3. Display Settings

Edit `include/config.h` for display and timing preferences:

```cpp
// Display Type Selection
// #define DISP_TYPE_BW   // Uncomment for B/W display
#define DISP_TYPE_6C      // Uncomment for 6-color display

// Update Settings
#define DAY_START_HOUR 5              // Daily update hour (5 = 5:00 AM)
#define WIFI_ERROR_RETRY_MINUTES 60   // Retry WiFi errors after 1 hour
#define CALENDAR_ERROR_RETRY_MINUTES 120 // Retry calendar errors after 2 hours

// Time & Locale
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0"  // Timezone string with DST
#define TIME_FORMAT_24H true          // true for 24-hour, false for 12-hour
#define FIRST_DAY_OF_WEEK 1           // 0=Sunday, 1=Monday
#define DISPLAY_LANGUAGE 0            // 0=English, 4=Italian

// Event Display
#define DAYS_TO_FETCH 30              // Event range in days
#define MAX_EVENTS_TO_SHOW 10         // Maximum events on display

// Color Customization (6-color displays only)
#ifdef DISP_TYPE_6C
    #define COLOR_CALENDAR_TODAY_TEXT      GxEPD_RED      // Today's date number
    #define COLOR_CALENDAR_TODAY_BORDER    GxEPD_RED      // Today's border
    #define COLOR_CALENDAR_OUTSIDE_MONTH   GxEPD_GREEN    // Previous/next month days
    #define COLOR_CALENDAR_WEEKEND_BG      GxEPD_YELLOW   // Weekend background (25% dithered)
    #define COLOR_CALENDAR_DAY_LABELS      GxEPD_RED      // Day labels (M, T, W...)
    #define COLOR_EVENT_TODAY_HEADER       GxEPD_RED      // "Today" header
    #define COLOR_EVENT_TOMORROW_HEADER    GxEPD_ORANGE   // "Tomorrow" header
    #define COLOR_EVENT_OTHER_HEADER       GxEPD_GREEN    // Other date headers
    #define COLOR_WEATHER_ICON            GxEPD_RED       // Weather icon
    #define COLOR_HEADER_DAY_NUMBER       GxEPD_RED       // Large day number
#endif
```

## Building and Uploading

### Prerequisites
1. Install PlatformIO (VS Code extension or CLI)
2. **Create your config.local file** (see Configuration section above)
3. Connect your ESP32 board via USB

### Standard Build
1. Open the project folder
2. Select your board environment in `platformio.ini`:
   - `esp32-s3-devkitc-1` for ESP32-S3 boards
   - Add your own environment for other boards
3. Build the project:
   ```bash
   pio run -e esp32-s3-devkitc-1
   ```
4. Upload to your board:
   ```bash
   pio run -t upload -e esp32-s3-devkitc-1
   ```
5. Monitor serial output:
   ```bash
   pio device monitor
   ```

### Debug Mode
Enable debug mode to test all features without WiFi or calendar configuration:

1. Edit `platformio.ini` and add/ensure this build flag:
   ```ini
   build_flags =
       -DDEBUG_DISPLAY=1
   ```

2. Build and upload:
   ```bash
   pio run -t upload -e esp32-s3-devkitc-1
   ```

3. Open serial monitor (115200 baud) and use the interactive menu:
   - `1` - Test Monthly Calendar View
   - `2` - Test Event List
   - `3` - Test Full Screen Errors
   - `4` - Test WiFi Connection
   - `5` - Test Icon Display
   - `6` - Test Battery Status
   - `7` - Test Calendar Fetch
   - `8` - Test Error Levels
   - `9` - Test Dithering Patterns
   - `10` - Test Time Display
   - `11` - Clear Display
   - `12` - Full Calendar Demo
   - `h` - Show menu

## Power Consumption

With optimized once-daily updates (v1.3.0):
- **Active**: ~80mA (during WiFi/display update, ~20-30 seconds)
- **Deep Sleep**: ~10µA (23+ hours per day)
- **Average**: ~0.2mA (with once-daily updates)
- **Battery Life**: 8-12 months on 2000mAh battery

Power-saving features:
- **Once-daily update** at configured hour (DAY_START_HOUR)
- **Smart error recovery**: Shorter retry intervals only on errors
- **Battery protection**: No wake-up when battery critical
- **Immediate deep sleep** after display update
- **Manual refresh** available via button press

## Troubleshooting

### Compilation error: config.local not found
- **Solution**: You need to create the local configuration file:
  ```bash
  cd include/
  cp config.local.example config.local
  # Edit config.local with your WiFi credentials
  ```

### Display not updating
- Check wiring connections
- Verify SPI pins in config.h match your wiring
- Try the display test function in setup()

### WiFi connection fails
- Verify SSID and password in `include/config.local` (not config.h)
- Check if 2.4GHz WiFi is enabled (ESP32 doesn't support 5GHz)
- Increase connection timeout if needed
- Ensure `config.local` exists (copy from `config.local.example`)

### No calendar events showing
- Verify your calendar URL is accessible
- Check if the URL requires authentication
- For ICS URLs, make sure they're publicly accessible
- Check serial monitor for error messages

### Time is incorrect
- Verify TIMEZONE string in config.h (e.g., "CET-1CEST,M3.5.0,M10.5.0" for Central Europe)
- Check that NTP servers are accessible
- System automatically handles DST based on timezone string

## Display Layout

The 800x480 display is divided into two main sections:

### Left Side - Month Calendar (400px)
- **Header**: Large day number (32pt) and month/year (26pt), centered
- **Calendar Grid**:
  - Day of week labels (localized, red on color displays)
  - 6-week calendar grid with smart visual hierarchy
  - Current day with colored border (red on color displays)
  - Event indicators (dots) in calendar cells
  - Weekend cells with 25% yellow (color) or 10% gray (B/W) background
  - Previous/next month days in green (color) or 20% gray (B/W)

### Right Side - Event List (400px)
- **Event Sections**:
  - "Oggi/Today" section with ALL of today's events (even if passed)
  - "Domani/Tomorrow" section
  - Smart date grouping for future events
  - Event details: time (12/24h format), title with word-wrap
  - Dynamic event limit with "+N more events" indicator
- **Weather Display** (bottom):
  - Large 96x96 weather icon (red on color displays)
  - Current temperature range
  - Sunrise/sunset times with icons
  - 7-day hourly forecast

### Status Bar (bottom)
- Current date/time (center)
- Version info (center)
- WiFi status with RSSI (right)
- Battery percentage (left)

## Error Handling

The system includes comprehensive error handling with visual feedback:

### Error Levels
- **INFO**: Informational messages
- **WARNING**: Non-critical issues (system continues)
- **ERROR**: Significant problems (may retry)
- **CRITICAL**: System failure (requires intervention)

### Error Categories
- **WiFi Errors** (100-199): Connection failures, authentication issues
- **Calendar Errors** (200-299): Fetch failures, parsing errors
- **Display Errors** (300-399): Hardware issues, refresh failures
- **System Errors** (400-499): Memory, filesystem problems
- **Battery Errors** (500-599): Low battery, charging issues
- **Time Errors** (600-699): NTP sync failures
- **Configuration Errors** (700-799): Invalid settings
- **Network Errors** (800-899): HTTP/HTTPS issues
- **Parsing Errors** (900-999): Data format problems

## Localization

Add new languages by creating a header file in `include/lang/`:

```cpp
// include/lang/es.h (Spanish example)
#define LOC_TODAY "Hoy"
#define LOC_TOMORROW "Mañana"
#define LOC_NO_EVENTS "Sin eventos"
// ... add all string translations
```

Then update `include/localization.h` to include your language file.

## Documentation

- **[Architecture Guide](platformio/docs/ARCHITECTURE.md)** - Comprehensive system architecture, design decisions, and data flow
- **[Changelog](CHANGELOG.md)** - Version history and release notes
- **[Migration Guide](platformio/MIGRATION_GUIDE.md)** - Upgrading from previous versions

## Project Structure

```
platformio/
├── include/
│   ├── config.h              # User configuration
│   ├── display_manager.h      # Display control
│   ├── calendar_client.h      # Calendar fetching
│   ├── error_manager.h        # Error handling
│   ├── localization.h         # Language selection
│   ├── version.h              # Version management
│   └── lang/                  # Language files
│       ├── en.h               # English
│       └── it.h               # Italian
├── src/
│   ├── main.cpp                      # Main program
│   ├── debug.cpp                     # Debug mode
│   ├── display_manager.cpp           # Core display functions
│   ├── display_calendar.cpp          # Calendar drawing functions
│   ├── display_weather.cpp           # Weather & header drawing
│   ├── display_events_status.cpp     # Events, status bar, errors
│   ├── calendar_client.cpp           # Calendar parsing
│   ├── error_manager.cpp             # Error handling
│   └── wifi_manager.cpp              # WiFi management
├── lib/
│   └── esp32-calendar-assets/ # Icons and fonts
├── docs/
│   ├── ARCHITECTURE.md        # System architecture and design
│   └── CALENDAR_SPECS.md      # ICS format specification
└── platformio.ini             # Build configuration
```

## Memory Usage

Typical resource usage (ESP32-S3 with 4MB flash):

- **Flash**: ~1.2MB (40% of 3.0MB available)
- **RAM**: ~90KB (27% of 320KB)
- **PSRAM**: Used for large calendar data when available

*Note: v1.7.8 reduced flash usage by ~600KB through code refactoring and icon optimization*

## Version Information

Current version: **1.7.8**

The project uses semantic versioning (MAJOR.MINOR.PATCH):
- **MAJOR**: Incompatible API changes
- **MINOR**: Backwards-compatible functionality additions
- **PATCH**: Backwards-compatible bug fixes

Version details are in `platformio/include/version.h` and displayed on serial startup.

## Known Issues

1. **ICS Timezone Parsing**: Events with UTC timestamps need manual timezone adjustment
2. **Event Limits**: Display limited to MAX_EVENTS_TO_SHOW (default: 10)
3. **Deep Sleep**: RTC time drift may occur over extended periods

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please submit pull requests or open issues for bugs and feature requests.

## Acknowledgments

- GxEPD2 library for e-paper display support
- Adafruit GFX library for graphics primitives
- ArduinoJson for JSON parsing
- ESP32 community for platform support