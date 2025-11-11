# ESP32 E-Paper Calendar Display

![Preview](3d%20models/PXL_20251109_163847280.jpg)

A feature-rich calendar display using ESP32 (S3/C6) and Waveshare/Good Display 7.5" e-paper displays (B/W or 6-color) with dual orientation support, comprehensive error handling, and multiple language support.

**Version 1.9.0** - Timezone fix for recurring events and API improvements!

## What's New in v1.9.0

- 🐛 **WEEKLY Recurring Event Fix**: Fixed 2-day offset in WEEKLY recurring events with COUNT limit
  - Root cause: Mixed use of local timezone and UTC caused date calculation errors
  - Solution: Use GMT/UTC consistently with new `portable_timegm()` helper
  - Test coverage: **240/240 tests passing (100%)**
- 🔧 **Display Manager API Refactoring**: Simplified API by using `time_t` instead of 5 separate date/time parameters
  - `showModernCalendar(events, now, weatherData, ...)` - cleaner signature
  - Date/time components now extracted internally from single `time_t` parameter
  - All orientation-specific methods updated for consistency

See [CHANGELOG.md](CHANGELOG.md) for complete details.

## 3D Model

A .3mf 3d model for the enclosure is available here [3d models/esp32-calendar.3mf](3d%20models/esp32-calendar.3mf).


## Features

### Display Features
- **Multi-Display Support**: Works with both B/W and 6-color e-paper displays
- **Dual Orientation Support**: Choose between landscape (800x480) or portrait (480x800) layout
  - **Landscape Mode**: Split-screen with calendar on left, events and weather on right
  - **Portrait Mode**: Calendar on top, weather on left side, events on right
- **Compile-Time Optimization**: Only the selected orientation code is compiled, saving ~6KB flash
- **6-Color Display Enhancements**: Configurable color scheme for visual hierarchy
- **Floyd-Steinberg Dithering**: Weekend cells (25% yellow on color, 10% gray on B/W)
- **Event Indicators**: Small dots in calendar cells with events
- **Smart Event Grouping**: Today, Tomorrow, and upcoming events sections with date headers
- **Weather Display**: Today and tomorrow forecast with icons, temperature, and precipitation
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
  - ESP32-S3 (recommended - includes PSRAM for large calendar data): [Waveshare ESP32-S3 Zero](https://www.waveshare.com/wiki/ESP32-S3-Zero)
  - Minimum 4MB flash, 2MB PSRAM recommended
- **Display Options**:
  - **B/W Display**: Waveshare 7.5" (800x480) - Models: V2, V3, GDEY075T7
  - **6-Color Display**: Good Display 7.5" GDEP073E01 (800x480) - Black, White, Red, Yellow, Orange, Green
- **Display Adapter**:
  - **Despi-C73**: [GoodDisplay Despi-C73](https://www.good-display.com/product/522.html) adapter board for colored display (or the Despi-C02 for the BW variant)
- **Breadboard PCB**:
  - Solderable Breadboard PCB Board [EPLZON PCB](https://www.amazon.com/EPLZON-Solder-able-Breadboard-Electronics-Compatible/dp/B09WZXHMDG?th=1)
- **Battery**:
  - [3.7v 3000mAh Li-ion Battery](https://www.amazon.it/3000mAh-103665-Lithium-Replacement-Bluetooth/dp/B091Y3TW9F/ref=sr_1_2?__mk_it_IT=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=E88BGRPKNWEK&dib=eyJ2IjoiMSJ9.F0RxMiAURIkn7hAt_Jou9v9Li-AjYtN4lWXsjuEmg7NXk1TMiMo87Lap1QnMBunFieEITLfbmaXek9lSuug0Vm8Dc_veUR0ZJn37n3BPv4k-s6oYhvO07xp5PvUWYZ9coVX0wG0fBHWwi5504xg-9VPaR92qWqsTyNmDRa2msyqXlthFpqGFUK-oClXu_jN46sN_mQbVscpZ2a039npaUenDt2olPGOxkBEEk5DouDpLYK3kAuZefonQfc80HSeMYgi5-oY4gU_QljCYhf0pIIfwtsGMmdZTOehuxNJFiu4.fVgvULxg3Sdm1zIV5UW9tnteuAuN1A02ieNoXeD43Pw&dib_tag=se&keywords=lipo+lipo+3.7+3000mah&qid=1762758569&sprefix=lipo+lipo+3.7+3000mah%2Caps%2C83&sr=8-2)
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


## Configuration

### 1. Initial Setup

Configuration is now managed through LittleFS using a `config.json` file. See the [LittleFS Configuration Guide](platformio/docs/LITTLEFS_CONFIG.md) for details.

For quick setup, edit `include/config.h` with your basic settings:

```cpp
// Location for weather (find at https://www.latlong.net/)
#define LOC_LATITUDE 40.7128    // Example: New York
#define LOC_LONGITUDE -74.0060   // Example: New York
```

### 2. Calendar and Weather Configuration

All runtime configuration is now managed through `data/config.json`. Create this file with your settings:

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_NETWORK",
    "password": "YOUR_PASSWORD"
  },
  "location": {
    "latitude": 40.7128,
    "longitude": -74.0060,
    "timezone": "America/New_York"
  },
  "calendars": [
    {
      "name": "Personal",
      "url": "https://calendar.google.com/calendar/ical/YOUR_CALENDAR_ID/private.ics",
      "color": "red",
      "enabled": true,
      "days_to_fetch": 30
    },
    {
      "name": "Work",
      "url": "https://outlook.office365.com/owa/calendar/YOUR_CALENDAR_ID/calendar.ics",
      "color": "blue",
      "enabled": true,
      "days_to_fetch": 30
    }
  ],
  "display": {
    "update_hour": 5,
    "language": "en"
  }
}
```

**How to get ICS URLs:**
- **Google Calendar**: Settings → Calendar → Integrate → Secret address in iCal format
- **iCloud**: Calendar app → Share Calendar → Public Calendar → Copy link
- **Outlook**: Settings → View all Outlook settings → Calendar → Shared calendars → Publish a calendar → Copy ICS link

**Upload configuration to device:**
```bash
pio run -t uploadfs
```

This uploads the `data/` folder (including `config.json`) to the ESP32's LittleFS filesystem.

### 3. Display Settings

Edit `include/config.h` for display type and orientation:

```cpp
// Display Type Selection
// #define DISP_TYPE_BW   // Uncomment for B/W display
#define DISP_TYPE_6C      // Uncomment for 6-color display

// Display Orientation
#define LANDSCAPE 0       // 800x480 - Calendar left, events right
#define PORTRAIT 1        // 480x800 - Calendar top, events/weather bottom
#define DISPLAY_ORIENTATION LANDSCAPE  // Change to PORTRAIT for vertical layout

// Time & Locale (can be overridden in config.json)
#define TIME_FORMAT_24H true          // true for 24-hour, false for 12-hour
#define FIRST_DAY_OF_WEEK 1           // 0=Sunday, 1=Monday
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
2. Configure your settings in `config.json` (see Configuration section above)
3. Connect your ESP32 board via USB

### Standard Build
1. Open the project folder
2. Select your board environment in `platformio.ini`:
   - `esp32-s3-devkitm-1` for ESP32-S3 boards
   - Add your own environment for other boards
3. Build the project:
   ```bash
   pio run -e esp32-s3-devkitm-1
   ```
4. Upload to your board:
   ```bash
   pio run -t upload -e esp32-s3-devkitm-1
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
   pio run -t upload -e esp32-s3-devkitm-1
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

### Configuration not loading
- **Solution**: Upload your `config.json` file to LittleFS:
  ```bash
  pio run -t uploadfs
  ```
- See [LittleFS Configuration Guide](platformio/docs/LITTLEFS_CONFIG.md) for details

### Display not updating
- Check wiring connections
- Verify SPI pins in config.h match your wiring
- Try the display test function in setup()

### WiFi connection fails
- Verify SSID and password in `data/config.json`
- Upload filesystem with `pio run -t uploadfs`
- Check if 2.4GHz WiFi is enabled (ESP32 doesn't support 5GHz)
- Increase connection timeout if needed

### No calendar events showing
- Verify your calendar URL is accessible
- Check if the URL requires authentication
- For ICS URLs, make sure they're publicly accessible
- Check serial monitor for error messages

### Time is incorrect
- Verify timezone setting in `data/config.json` (e.g., "America/New_York", "Europe/Rome")
- Upload filesystem with `pio run -t uploadfs`
- Check that NTP servers are accessible
- System automatically handles DST based on timezone

## Display Layout

The display supports two orientations, configured via `DISPLAY_ORIENTATION` in `config.h`:

### Landscape Mode (800x480) - Default

**Left Side - Month Calendar (400px)**
- **Header**: Large day number and month/year, centered with sunrise/sunset times
- **Calendar Grid**:
  - Day of week labels (localized, red on color displays)
  - 6-week calendar grid with smart visual hierarchy
  - Current day with colored border (red on color displays)
  - Event indicators (dots) in calendar cells
  - Weekend cells with 25% yellow (color) or 10% gray (B/W) background
  - Previous/next month days in green (color) or 20% gray (B/W)

**Right Side - Events & Weather (400px)**
- **Event Sections** (top 340px):
  - "Today" section with ALL of today's events (even if passed)
  - "Tomorrow" section
  - Smart date grouping for future events
  - Event details: time (12/24h format), title
  - Up to 10 events displayed with "+N more" indicator
- **Weather Display** (bottom 100px):
  - Side-by-side today and tomorrow forecast
  - 64x64 weather icons (red on color displays)
  - Temperature range and precipitation probability

**Status Bar** (bottom, full width):
- WiFi status with RSSI (left)
- Battery percentage (left)
- Version info (right)

### Portrait Mode (480x800) - Optional

**Top Section - Header & Calendar (410px)**
- **Header**: Compact centered day number and month/year
- **Calendar Grid**: Full-width calendar (7 columns × 480px)
  - Compact 35px cell height for space efficiency
  - Same visual features as landscape (borders, dots, weekend highlighting)

**Bottom Section - Events & Weather (340px)**
- **Left Panel - Weather (180px)**:
  - Vertical stacked layout
  - Today and tomorrow sections
  - 48x48 weather icons
  - Temperature and precipitation probability
- **Right Panel - Events (290px)**:
  - Event list with smart date grouping
  - Up to 8 events displayed
  - Time and title per line
  - "+N more" indicator if needed

**Status Bar** (bottom, full width):
- Same layout as landscape mode

### Switching Orientations

Change the `DISPLAY_ORIENTATION` constant in `include/config.h`:

```cpp
#define DISPLAY_ORIENTATION LANDSCAPE  // For 800x480 horizontal
// or
#define DISPLAY_ORIENTATION PORTRAIT   // For 480x800 vertical
```

The display rotation and layout are automatically configured at compile-time. Only the code for the selected orientation is compiled, saving ~6KB flash memory.

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

## Project Structure

```
platformio/
├── include/
│   ├── config.h                   # User configuration (orientation, colors, etc.)
│   ├── display_manager.h          # Display control with orientation-specific declarations
│   ├── calendar_client.h          # Calendar fetching
│   ├── error_manager.h            # Error handling
│   ├── localization.h             # Language selection
│   ├── version.h                  # Version management
│   └── lang/                      # Language files
│       ├── en.h                   # English
│       └── it.h                   # Italian
├── src/
│   ├── main.cpp                   # Main program
│   ├── debug.cpp                  # Debug mode (orientation-aware)
│   ├── display_manager.cpp        # Core display + orientation dispatcher
│   ├── display_landscape.cpp      # Landscape layout (compiled only if LANDSCAPE)
│   ├── display_portrait.cpp       # Portrait layout (compiled only if PORTRAIT)
│   ├── display_calendar_helpers.cpp # Shared calendar utilities
│   ├── display_shared.cpp         # Shared display functions (status bar, errors)
│   ├── calendar_client.cpp        # Calendar parsing
│   ├── error_manager.cpp          # Error handling
│   └── wifi_manager.cpp           # WiFi management
├── data/
│   └── config.json                # Configuration file (uploaded to LittleFS)
├── lib/
│   └── esp32-calendar-assets/     # Icons and fonts
├── docs/
│   ├── ARCHITECTURE.md            # System architecture and design
│   ├── LITTLEFS_CONFIG.md         # LittleFS configuration guide
│   └── CALENDAR_SPECS.md          # ICS format specification
└── platformio.ini                 # Build configuration
```

### Display Module Organization (Compile-Time Optimized)

The display code is organized for maximum efficiency:

- **`display_manager.cpp`**: Core functions + orientation dispatcher (always compiled)
- **`display_landscape.cpp`**: Complete landscape implementation (compiled only when `DISPLAY_ORIENTATION == LANDSCAPE`)
- **`display_portrait.cpp`**: Complete portrait implementation (compiled only when `DISPLAY_ORIENTATION == PORTRAIT`)
- **`display_calendar_helpers.cpp`**: Shared calendar utilities used by both orientations (always compiled)
- **`display_shared.cpp`**: Shared functions like status bar and error screens (always compiled)

This architecture ensures that only the code for the selected orientation is included in the final binary, reducing flash usage by ~6KB.

## Memory Usage

Typical resource usage (ESP32-S3 with 4MB flash):

- **Flash (Landscape)**: ~1.34MB (44.4% of 3.0MB available)
- **Flash (Portrait)**: ~1.33MB (44.2% of 3.0MB available) - *6KB smaller due to compile-time optimization*
- **RAM**: ~112KB (34.2% of 320KB)
- **PSRAM**: Used for large calendar data when available

## Known Issues

1. **ICS Timezone Parsing**: Events with UTC timestamps need manual timezone adjustment
2. **Event Limits**: Display limited to MAX_EVENTS_TO_SHOW (default: 10)

## License

This project is released under the GNU General Public License v3.0 (GPLv3).

Why GPLv3?

This project incorporates source code from an external project, lmarzen/esp32-weather-epd, which is licensed under the GPLv3.

Due to the "strong copyleft" nature of the GPLv3, any work that includes or is derived from GPLv3-licensed code must also be licensed under the GPLv3. This license is intended to guarantee users the freedom to share and change all versions of the software.

## Contributing

Contributions are welcome! Please submit pull requests or open issues for bugs and feature requests.

## Acknowledgments

- GxEPD2 library for e-paper display support
- Adafruit GFX library for graphics primitives
- ArduinoJson for JSON parsing
- ESP32 community for platform support