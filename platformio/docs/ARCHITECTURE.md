# ESP32 Calendar System Architecture

**Version:** 1.7.7
**Last Updated:** 2025-01-02
**Platform:** ESP32-S3 with 7.5" E-Paper Display

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Data Flow](#data-flow)
4. [Core Components](#core-components)
5. [Storage Strategy](#storage-strategy)
6. [Network & Caching](#network--caching)
7. [Display Pipeline](#display-pipeline)
8. [Configuration System](#configuration-system)
9. [Key Design Decisions](#key-design-decisions)

---

## Overview

This is a low-power ESP32-based calendar display that fetches events from multiple ICS calendar feeds, displays them on a 7.5" e-paper screen, and uses deep sleep to conserve battery. The system prioritizes reliability, efficiency, and graceful degradation.

### Key Features

- **Stream Parsing**: Parses ICS files directly from HTTP without storing raw data
- **Binary Event Cache**: Stores processed events (~200 bytes each) for offline fallback
- **Multi-Calendar Support**: Up to 3 calendars with different colors and settings
- **Holiday Support**: Special rendering for holiday calendars
- **Weather Integration**: Open-Meteo API for weather forecasts
- **Battery Monitoring**: MAX17048 fuel gauge integration
- **Deep Sleep**: Wakes daily at configured time or on button press

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         ESP32-S3 Main                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ WiFiManager  │  │CalendarManager│  │WeatherClient │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
│         │                  │                  │                   │
│         ├──────────────────┼──────────────────┘                  │
│         │                  │                                      │
│  ┌──────▼──────────────────▼──────────────┐                      │
│  │      CalendarWrapper (per calendar)     │                      │
│  │  ┌────────────────────────────────────┐ │                      │
│  │  │   CalendarStreamParser             │ │                      │
│  │  │   ┌─────────────┐ ┌──────────────┐│ │                      │
│  │  │   │CalendarFetcher│ EventCache   ││ │                      │
│  │  │   └─────────────┘ └──────────────┘│ │                      │
│  │  └────────────────────────────────────┘ │                      │
│  └─────────────────────────────────────────┘                      │
│         │                                                          │
│  ┌──────▼──────────────────────────────────────┐                 │
│  │           DisplayManager                     │                 │
│  │  ┌───────────────────────────────────────┐  │                 │
│  │  │  Header │ Month View │ Events │ Weather│ │                 │
│  │  └───────────────────────────────────────┘  │                 │
│  └──────────────┬───────────────────────────────┘                 │
│                 │                                                  │
│  ┌──────────────▼──────────────┐                                  │
│  │    GxEPD2 (7.5" Display)    │                                  │
│  └─────────────────────────────┘                                  │
│                                                                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │  LittleFS   │  │Battery Mon. │  │ Deep Sleep  │              │
│  │  (1MB)      │  │ (MAX17048)  │  │  Manager    │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

### 1. Calendar Event Fetch & Cache Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ 1. FETCH ATTEMPT (with retries)                                 │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
    ┌──────────────────────────────────────┐
    │ CalendarWrapper::load()              │
    │ - Retry up to 3 times (2s delay)    │
    └──────────────────┬───────────────────┘
                       │
                       ▼
    ┌──────────────────────────────────────┐
    │ CalendarStreamParser::fetchEventsInRange() │
    └──────────────────┬───────────────────┘
                       │
                       ▼
    ┌──────────────────────────────────────┐
    │ CalendarFetcher::fetchStream()       │
    │ - Opens HTTPS connection             │
    │ - Returns Stream* (WiFiClientSecure) │
    └──────────────────┬───────────────────┘
                       │
                       ▼
    ┌──────────────────────────────────────┐
    │ CalendarStreamParser::streamParseFromStream() │
    │ - Reads line-by-line from HTTP stream│
    │ - Parses ICS format on-the-fly       │
    │ - Filters by date range              │
    │ - Calls callback for each event      │
    └──────────────────┬───────────────────┘
                       │
                       ▼
    ┌──────────────────────────────────────┐
    │ CalendarWrapper receives events      │
    │ - Adds calendar metadata             │
    │ - Marks holidays if applicable       │
    └──────────────────┬───────────────────┘
                       │
                       ▼
    ┌──────────────────────────────────────┐
    │ SUCCESS: Save to binary cache        │
    │ EventCache::save()                   │
    │ /cache/events_{hash}.bin             │
    └──────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ 2. FALLBACK (if all retries fail)                               │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
    ┌──────────────────────────────────────┐
    │ EventCache::load()                   │
    │ - Load stale binary cache            │
    │ - Mark as "isStale = true"           │
    └──────────────────────────────────────┘
```

### 2. Display Rendering Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ DisplayManager::updateDisplay()                                  │
└─────────────────────────────────────────────────────────────────┘
           │
           ├────▶ drawHeader()           [Date, sunrise/sunset]
           │
           ├────▶ drawMonthCalendar()    [Month grid with event markers]
           │      - Marks days with events (colored dots)
           │      - Marks holidays (weekend background)
           │      - Highlights today (red border)
           │
           ├────▶ drawEventsSection()    [Upcoming events list]
           │      - Today's events (red header)
           │      - Tomorrow's events (orange header)
           │      - Future events (green header)
           │      - Single-line with ellipsis truncation
           │
           └────▶ drawWeatherSection()   [Weather forecast]
                  - Today/Tomorrow weather
                  - Temperature, rain probability
                  - Weather icons
```

---

## Core Components

### CalendarManager

**File:** `calendar_wrapper.h/cpp`
**Responsibility:** Manages multiple calendar sources

```cpp
class CalendarManager {
    - loadFromConfig()      // Initialize from RuntimeConfig
    - loadAll()             // Fetch all calendars (with retry)
    - getAllEvents()        // Merge and sort events from all calendars
    - isAnyCalendarStale()  // Check if using cached data
};
```

### CalendarWrapper

**File:** `calendar_wrapper.h/cpp`
**Responsibility:** Wraps a single calendar source

**Key Features:**
- Retry logic: 3 attempts with 2-second delays
- Binary cache fallback on network failure
- Adds calendar metadata to events (name, color, holiday flag)

```cpp
class CalendarWrapper {
    - load(forceRefresh)           // Fetch with retry, fallback to cache
    - isCacheValid()               // Check if binary cache is fresh
    - getCacheFilename()           // Generate hash-based cache path
    - getEvents(startDate, endDate) // Filter cached events by date
};
```

### CalendarStreamParser

**File:** `calendar_stream_parser.h/cpp`
**Responsibility:** Parse ICS files from HTTP streams

**Key Innovation:** No intermediate file storage!

```cpp
class CalendarStreamParser {
    - fetchEventsInRange()         // Entry point for fetching
    - streamParse()                // Coordinate stream fetch + parse
    - streamParseFromStream()      // Core ICS parser (line-by-line)
    - parseEventFromBuffer()       // Parse individual VEVENT
};
```

**Parsing Strategy:**
- State machine: `LOOKING_FOR_CALENDAR` → `IN_HEADER` → `IN_EVENT` → `DONE`
- Reads line-by-line (handles CRLF, LF, line folding)
- Buffers each VEVENT (max 8KB)
- Filters events by date range during parse
- Calls callback for each matched event

### EventCache

**File:** `event_cache.h/cpp`
**Responsibility:** Binary serialization of processed events

**Cache File Format:**
```
┌────────────────────────────────────────┐
│ CacheHeader (packed struct)            │
│ - magic: 0xCAFEEE00                    │
│ - version: 1                           │
│ - eventCount: uint32_t                 │
│ - timestamp: time_t                    │
│ - calendarUrl: char[256]               │
│ - checksum: CRC32                      │
├────────────────────────────────────────┤
│ SerializedEvent[eventCount]            │
│ - title: char[128]                     │
│ - location: char[64]                   │
│ - date: char[16]                       │
│ - startTimeStr: char[8]                │
│ - endTimeStr: char[8]                  │
│ - startTime: time_t                    │
│ - endTime: time_t                      │
│ - flags: uint8_t (packed booleans)     │
│ - dayOfMonth: uint8_t                  │
│ - calendarName: char[32]               │
│ - calendarColor: char[16]              │
│ - summary: char[128]                   │
└────────────────────────────────────────┘
```

**Size:** ~200 bytes per event
**Example:** 3 calendars × 50 events = ~30KB total

### DisplayManager

**File:** `display_manager.h/cpp`
**Responsibility:** Render everything to e-paper display

**Layout Zones:**
```
┌─────────────────────────────────────────────────────────┐
│ HEADER (80px)                                           │
│ - Day number (32pt) │ Month/Year (26pt) │ Sun/Set      │
├─────────────────────────────────────────────────────────┤
│ MONTH CALENDAR (Left, 300px)                            │
│ - 7x6 grid of days                                      │
│ - Event markers (colored dots)                          │
│ - Holiday backgrounds                                   │
│ - Today highlight                                       │
├─────────────────────────────────────────────────────────┤
│ EVENTS LIST (Right, 500px)                              │
│ - Today (red header)                                    │
│ - Tomorrow (orange header)                              │
│ - Future dates (green header)                           │
│ - Time + Single-line summary (ellipsis)                 │
│ - Max 10 events shown                                   │
├─────────────────────────────────────────────────────────┤
│ WEATHER (Bottom, 100px)                                 │
│ - Today │ Tomorrow                                      │
│ - Icons, Temp, Rain %                                   │
└─────────────────────────────────────────────────────────┘
```

---

## Storage Strategy

### Flash Partition Layout (ESP32-S3, 4MB)

```
┌──────────────────────────────────────────────────────┐
│ Offset    Size       Type        Purpose             │
├──────────────────────────────────────────────────────┤
│ 0x9000    20KB       nvs         Config storage      │
│ 0x10000   2.875MB    app         Firmware (44% used) │
│ 0x2F0000  1MB        spiffs      Event cache         │
│ 0x3F0000  64KB       coredump    Debug data          │
└──────────────────────────────────────────────────────┘
```

**Why 1MB for cache?**
- Binary events: ~200 bytes each
- 3 calendars × 50 events = ~30KB
- 1MB allows ~5,000 events (massive headroom)
- Future-proof for SD card expansion

### LittleFS File Structure

```
/
├── config.json              # Runtime configuration
└── cache/
    ├── events_{hash1}.bin   # Calendar 1 binary cache
    ├── events_{hash2}.bin   # Calendar 2 binary cache
    └── events_{hash3}.bin   # Calendar 3 binary cache
```

**Cache Filename Generation:**
- djb2 hash of calendar URL
- Ensures URL uniqueness
- Example: `events_A3F2C1B4.bin`

---

## Network & Caching

### Fetch Strategy (with Retry & Fallback)

```cpp
// Configuration (config.h)
#define CALENDAR_FETCH_MAX_RETRIES 3         // 3 attempts
#define CALENDAR_FETCH_RETRY_DELAY_MS 2000   // 2 seconds between retries
#define EVENT_CACHE_VALIDITY_SECONDS 86400   // 24 hours

// Logic (calendar_wrapper.cpp)
for (retry = 0; retry < MAX_RETRIES; retry++) {
    result = parser.fetchEventsInRange(url, startDate, endDate);
    if (success) {
        EventCache::save(cachePath, events, url);
        return true;
    }
    delay(RETRY_DELAY_MS);
}

// All retries failed - try cache
cachedEvents = EventCache::load(cachePath, url);
if (!cachedEvents.empty()) {
    isStale = true;  // Warn user
    return true;
}

return false;  // Total failure
```

### Cache Validation

```cpp
bool EventCache::isValid(cachePath, maxAge) {
    - Check file exists
    - Read header
    - Validate magic number (0xCAFEEE00)
    - Validate version (1)
    - Check timestamp (now - timestamp < maxAge)
    return valid;
}
```

### HTTP Stream Details

**CalendarFetcher** uses `WiFiClientSecure` for HTTPS:
- Follows redirects (max 5)
- Sets user agent
- Configures TLS (insecure mode for simplicity)
- Returns raw `Stream*` to parser

**No intermediate storage** - parser reads directly from socket!

---

## Display Pipeline

### E-Paper Display (GxEPD2)

**Model:** 7.5" 6-color (Black, White, Red, Yellow, Orange, Green)
**Resolution:** 800×480 pixels
**Refresh Time:** ~30 seconds (full refresh)
**Power:** ~50mA during refresh, 0mA when off

### Color Usage

```cpp
// config.h
#define COLOR_CALENDAR_TODAY_TEXT        GxEPD_RED
#define COLOR_CALENDAR_TODAY_BORDER      GxEPD_RED
#define COLOR_EVENT_TODAY_HEADER         GxEPD_RED
#define COLOR_EVENT_TOMORROW_HEADER      GxEPD_ORANGE
#define COLOR_EVENT_OTHER_HEADER         GxEPD_GREEN
#define COLOR_CALENDAR_WEEKEND_BG        GxEPD_YELLOW  // with dithering
```

### Font Strategy

**Selected Fonts:**
- Header: Luna ITC Bold (32pt, 26pt)
- Calendar: Luna ITC Bold (12pt)
- Events: Ubuntu Regular (9pt)
- Weather: Montserrat Regular (9pt, 8pt)

**Why these fonts?**
- Readability at distance
- Efficient glyph sizes
- Professional appearance

### Dithering for Subtle Colors

```cpp
void drawDitheredRectangle(x, y, w, h, color1, color2, level) {
    // level = 25 → 25% color2, 75% color1
    // Used for weekend backgrounds (GxEPD_YELLOW at 25%)
}
```

---

## Configuration System

### Configuration Sources

**Priority (highest to lowest):**
1. `data/config.json` (LittleFS) - Runtime configuration
2. `config.h` - Compile-time defaults

### Runtime Configuration (config.json)

```json
{
  "wifi": {
    "ssid": "YourSSID",
    "password": "YourPassword"
  },
  "location": {
    "latitude": 45.4642,
    "longitude": 9.1900,
    "name": "Milan"
  },
  "calendars": [
    {
      "name": "Personal",
      "url": "https://calendar.google.com/...",
      "enabled": true,
      "days_to_fetch": 30,
      "color": "red",
      "holiday_calendar": false
    },
    {
      "name": "Holidays",
      "url": "https://calendar.google.com/...",
      "enabled": true,
      "days_to_fetch": 365,
      "color": "green",
      "holiday_calendar": true
    }
  ],
  "display": {
    "timezone": "CET-1CEST,M3.5.0,M10.5.0/3",
    "update_hour": 5
  }
}
```

### Compile-Time Configuration (config.h)

**Key Constants:**
```cpp
// Display
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define MAX_EVENTS_TO_SHOW 10
#define MAX_CALENDARS 3

// Cache
#define EVENT_CACHE_MAGIC 0xCAFEEE00
#define EVENT_CACHE_VERSION 1
#define EVENT_CACHE_MAX_EVENTS 200
#define EVENT_CACHE_VALIDITY_SECONDS 86400

// Network
#define CALENDAR_FETCH_MAX_RETRIES 3
#define CALENDAR_FETCH_RETRY_DELAY_MS 2000

// Power
#define DISABLE_DEEP_SLEEP false
#define DEFAULT_UPDATE_HOUR 5
```

---

## Key Design Decisions

### 1. Why Stream Parsing Instead of File Download?

**Problem:** Large ICS files (>2MB) exceeded LittleFS partition
**Solution:** Parse directly from HTTP stream

**Benefits:**
- ✅ Handles unlimited file sizes
- ✅ Faster (no disk I/O)
- ✅ Lower memory usage
- ✅ No risk of filling flash

**Trade-off:**
- ❌ Can't re-parse without re-downloading (but cache mitigates this)

### 2. Why Binary Event Cache Instead of ICS Cache?

**Problem:** Caching raw ICS files wastes space and requires re-parsing
**Solution:** Cache processed events in compact binary format

**Comparison:**
```
Raw ICS Cache:
- Size: 1-5MB per calendar
- Format: Text (UTF-8)
- Requires: Re-parsing on load
- Space: 3 calendars = 3-15MB

Binary Event Cache:
- Size: ~200 bytes per event
- Format: Packed structs
- Requires: Simple deserialization
- Space: 3 calendars × 50 events = ~30KB (500x smaller!)
```

### 3. Why Retry Logic with Cache Fallback?

**Goal:** Maximize fresh data availability while providing graceful degradation

**Strategy:**
1. **Primary:** Fetch from remote (3 retries, 2s delay)
2. **Fallback:** Load stale cache (up to 24 hours old)
3. **Failure:** Show error message

**Benefits:**
- ✅ Resilient to transient network issues
- ✅ Offline functionality with stale data
- ✅ Clear user feedback (stale indicator)

### 4. Why djb2 Hash for Cache Filenames?

**Requirement:** Deterministic URL → filename mapping
**Solution:** djb2 hash algorithm

```cpp
unsigned long hash = 5381;
for (char c : url) {
    hash = ((hash << 5) + hash) + c;  // hash * 33 + c
}
String filename = "/cache/events_" + String(hash, HEX) + ".bin";
```

**Benefits:**
- ✅ Deterministic (same URL → same filename)
- ✅ Fast (simple math)
- ✅ Filesystem-safe (hex output)
- ✅ No collisions in practice

### 5. Why Holiday Calendar Flag?

**Use Case:** Italian holidays calendar should mark days as holidays
**Solution:** Per-calendar `holiday_calendar` flag

```cpp
struct CalendarConfig {
    bool holiday_calendar;  // If true, all-day events are holidays
};

// When fetching events:
event->isHoliday = (config.holiday_calendar && event->allDay);

// In month view:
if (hasHoliday[day]) {
    // Draw weekend background (yellow dithering)
}
```

**Benefits:**
- ✅ Flexible (works with any holiday ICS feed)
- ✅ Visual clarity (holidays stand out)
- ✅ No hardcoding holiday dates

### 6. Why Single-Line Event Summaries?

**Problem:** Long event titles wasted vertical space
**Solution:** Truncate to 1 line with ellipsis

**Before:**
```
10:00 Very long event title that
      wraps to multiple lines
      taking up valuable space
```

**After:**
```
10:00 Very long event title that...
11:30 Another event
```

**Benefits:**
- ✅ Shows more events per screen
- ✅ Cleaner layout
- ✅ Consistent spacing

---

## Memory Usage

### Flash (2.875MB app partition)

```
Total:  2,875 KB (2.875 MB)
Used:   1,325 KB (43.9%)
Free:   1,550 KB
```

**Breakdown:**
- Firmware core: ~800 KB
- GxEPD2 + fonts: ~300 KB
- ArduinoJson: ~50 KB
- WiFi/TLS stack: ~150 KB
- Application code: ~25 KB

### RAM (320KB total)

```
Total:  320 KB
Used:   112 KB (34.2%)
Free:   208 KB
```

**Breakdown:**
- Display buffer: ~48 KB (800×480 / 8 bits)
- Event vectors: ~10 KB (50 events × 200 bytes)
- Stack: ~20 KB
- WiFi/TLS: ~30 KB
- Other: ~4 KB

### LittleFS (1MB partition)

```
Total:  1,024 KB (1 MB)
Typical: ~50 KB used
- config.json: ~2 KB
- Event caches: ~30 KB (3 calendars)
- Free: ~970 KB
```

---

## Power Management

### Deep Sleep Strategy

**Normal Operation:**
1. Wake at configured time (default: 5:00 AM)
2. Connect to WiFi (~5s)
3. Fetch calendars and weather (~10s)
4. Update display (~30s)
5. Enter deep sleep (~23h 59m)

**Button Wake:**
1. User presses wake button (GPIO 2)
2. System performs full refresh
3. Returns to deep sleep

**Power Consumption:**
- Active: ~200 mA (WiFi + display)
- Deep sleep: ~10 μA
- Average: ~0.5 mAh/day

**Battery Life Estimate:**
- 3000 mAh battery: ~16 years (in practice: 6-12 months)

---

## Error Handling

### Network Errors

```cpp
// Retry with exponential backoff (up to 3 times)
if (fetch_fails) {
    retry_count++;
    if (retry_count < MAX_RETRIES) {
        delay(RETRY_DELAY_MS);
        retry();
    } else {
        // Fallback to cache
        load_cache();
    }
}
```

### Cache Errors

```cpp
// Validate cache before loading
if (!isValid(cachePath)) {
    // Cache expired or corrupted
    return empty_vector;
}

// Check CRC32 checksum
if (calculated_crc != header.checksum) {
    // Cache corrupted
    return empty_vector;
}
```

### Display Errors

```cpp
// ErrorManager handles all error states
error_manager.displayError(ERROR_WIFI_FAILED);
error_manager.displayError(ERROR_CALENDAR_FAILED);
error_manager.displayError(ERROR_BATTERY_LOW);

// Each error type has custom retry interval
```

---

## Testing Strategy

### Unit Tests (Native)

**Framework:** doctest
**Location:** `test/test_native/`

**Tested Components:**
- `EventCache` (15 tests, 93% pass rate)
  - Save/load round-trip
  - CRC32 validation
  - Cache expiration
  - Corrupted data handling
- `StringUtils` (string manipulation)
- `CalendarStreamParser` (ICS parsing)

**Run:**
```bash
pio test -e native
```

### Integration Tests (Embedded)

**Framework:** Unity
**Location:** `test/test_embedded/`

**Tested Components:**
- WiFi connection
- HTTP fetching
- Display rendering

**Run:**
```bash
pio test -e esp32-s3-devkitm-1
```

### Mock Environment

**Files:**
- `test/mock_arduino.h` - Mock Arduino API
- `test/mock_littlefs.h` - Mock filesystem with persistence
- `test/mock_arduino.cpp` - Global mock instances

**Key Innovation:** File persistence using `shared_ptr<FileData>`

---

## Future Enhancements

### Planned Features

1. **SD Card Support** (for unlimited event storage)
2. **Multi-timezone Support** (display events in different timezones)
3. **Recurring Event Expansion** (RRULE parsing)
4. **Event Filtering** (by category, keyword)
5. **Custom Display Layouts** (user-configurable)
6. **OTA Updates** (over-the-air firmware updates)

### Performance Optimizations

1. **Partial Display Updates** (faster refresh for weather/clock)
2. **Incremental Event Parsing** (stop early when date range exceeded)
3. **Compressed Binary Cache** (zlib compression)

---

## File Structure Reference

```
platformio/
├── include/
│   ├── config.h                    # Compile-time configuration
│   ├── calendar_wrapper.h          # Multi-calendar manager
│   ├── calendar_stream_parser.h    # ICS stream parser
│   ├── event_cache.h               # Binary event cache
│   ├── calendar_event.h            # Event data structure
│   ├── display_manager.h           # Display rendering
│   ├── littlefs_config.h           # Runtime config loader
│   └── ...
├── src/
│   ├── main.cpp                    # Main loop + deep sleep
│   ├── calendar_wrapper.cpp        # Calendar fetch logic
│   ├── calendar_stream_parser.cpp  # ICS parser implementation
│   ├── event_cache.cpp             # Binary serialization
│   ├── display_manager.cpp         # Display rendering
│   └── ...
├── data/
│   └── config.json                 # Runtime configuration
├── test/
│   ├── test_native/                # Unit tests (native)
│   └── test_embedded/              # Integration tests (embedded)
├── docs/
│   ├── ARCHITECTURE.md             # This file
│   ├── CALENDAR_SPECS.md           # ICS format details
│   └── ...
└── platformio.ini                  # Build configuration
```

---

## Useful Links

- [ICS Specification (RFC 5545)](https://datatracker.ietf.org/doc/html/rfc5545)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [Open-Meteo API](https://open-meteo.com/)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

---

**Document Maintained By:** Claude (Anthropic)
**Source Repository:** [esp32-calendar](https://github.com/sephiroth74/arduino/tree/main/esp32-calendar)
