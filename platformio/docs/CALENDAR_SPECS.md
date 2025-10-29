# ICS Calendar Technical Specifications

This document describes the technical specifications for ICS (iCalendar) calendar parsing and how the ESP32 E-Paper Calendar implements RFC 5545 standards.

## Table of Contents

- [Overview](#overview)
- [RFC 5545 Compliance](#rfc-5545-compliance)
- [Supported ICS Properties](#supported-ics-properties)
- [Parsing Architecture](#parsing-architecture)
- [Event Processing](#event-processing)
- [Recurring Events (RRULE)](#recurring-events-rrule)
- [Memory Management](#memory-management)
- [Caching Strategy](#caching-strategy)
- [Date/Time Handling](#datetime-handling)
- [Known Limitations](#known-limitations)

---

## Overview

The ESP32 E-Paper Calendar uses a **streaming parser** architecture to efficiently process ICS calendar files without loading the entire file into memory. This is critical for embedded systems with limited RAM.

### Key Features

- **Memory-Efficient Streaming**: Processes events line-by-line without storing full ICS content
- **On-Demand Filtering**: Only events within requested date range are retained
- **Multiple Sources**: Supports remote URLs (HTTPS) and local files (LittleFS)
- **Automatic Caching**: Remote calendars can be cached to reduce network requests
- **Recurring Events**: Full support for RRULE (currently FREQ=YEARLY)
- **Event Expansion**: Recurring events are expanded for the requested date range

---

## RFC 5545 Compliance

The parser implements the following portions of [RFC 5545 - Internet Calendaring and Scheduling Core Object Specification](https://tools.ietf.org/html/rfc5545):

### Supported Components

| Component | Support Level | Notes |
|-----------|---------------|-------|
| `VCALENDAR` | ✅ Full | Root component, required |
| `VEVENT` | ✅ Full | Event component |
| `VTIMEZONE` | ⚠️ Partial | Parsed but not used (relies on system timezone) |
| `VTODO` | ❌ None | To-do items not supported |
| `VJOURNAL` | ❌ None | Journal entries not supported |
| `VFREEBUSY` | ❌ None | Free/busy info not supported |
| `VALARM` | ❌ None | Alarms not supported |

### Line Folding (RFC 5545 Section 3.1)

The parser correctly handles **line folding** where long lines are split at 75 octets:

```ics
DESCRIPTION:This is a very long description that exceeds seventy-five char
 acters and needs to be folded according to RFC 5545 specifications
```

The parser automatically unfolds lines by:
1. Detecting lines starting with whitespace (SPACE or TAB)
2. Removing the line break and leading whitespace
3. Concatenating with the previous line

**Implementation**: See `readLineFromStream()` in `calendar_stream_parser.cpp`

---

## Supported ICS Properties

### Calendar Properties (VCALENDAR)

| Property | Required | Purpose | Example |
|----------|----------|---------|---------|
| `VERSION` | ✅ Yes | ICS version (must be "2.0") | `VERSION:2.0` |
| `PRODID` | ✅ Yes | Product identifier | `PRODID:-//Google Inc//Google Calendar 70.9054//EN` |
| `CALSCALE` | ⚠️ Optional | Calendar scale | `CALSCALE:GREGORIAN` |
| `METHOD` | ⚠️ Optional | iTIP method | `METHOD:PUBLISH` |
| `X-WR-CALNAME` | ⚠️ Optional | Calendar display name | `X-WR-CALNAME:My Calendar` |
| `X-WR-TIMEZONE` | ⚠️ Optional | Default timezone | `X-WR-TIMEZONE:Europe/Rome` |
| `X-WR-CALDESC` | ⚠️ Optional | Calendar description | `X-WR-CALDESC:Personal events` |

### Event Properties (VEVENT)

| Property | Required | Purpose | Example |
|----------|----------|---------|---------|
| `UID` | ✅ Yes | Unique event identifier | `UID:event123@example.com` |
| `DTSTART` | ✅ Yes | Start date/time | `DTSTART:20251029T140000` |
| `DTEND` | ⚠️ Optional | End date/time | `DTEND:20251029T150000` |
| `SUMMARY` | ✅ Yes | Event title | `SUMMARY:Team Meeting` |
| `DESCRIPTION` | ⚠️ Optional | Event description | `DESCRIPTION:Monthly sync` |
| `LOCATION` | ⚠️ Optional | Event location | `LOCATION:Conference Room A` |
| `STATUS` | ⚠️ Optional | Event status | `STATUS:CONFIRMED` |
| `RRULE` | ⚠️ Optional | Recurrence rule | `RRULE:FREQ=YEARLY` |
| `SEQUENCE` | ⚠️ Optional | Revision number | `SEQUENCE:0` |
| `CREATED` | ⚠️ Optional | Creation timestamp | `CREATED:20251001T120000Z` |
| `LAST-MODIFIED` | ⚠️ Optional | Last modified timestamp | `LAST-MODIFIED:20251015T083000Z` |
| `TRANSP` | ⚠️ Optional | Time transparency | `TRANSP:OPAQUE` |

### Unsupported Properties

The following are parsed but **not used**:
- `CATEGORIES` - Event categories
- `CLASS` - Access classification (PUBLIC/PRIVATE)
- `PRIORITY` - Event priority
- `ORGANIZER` - Event organizer
- `ATTENDEE` - Event attendees
- `ATTACH` - File attachments
- `URL` - Associated URL

---

## Parsing Architecture

### Stream-Based Processing

```
┌─────────────┐
│   Source    │  (HTTP/HTTPS or LittleFS file)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  TeeStream  │  (Optional: writes to cache while reading)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│    Line     │  Line-by-line reading with folding support
│   Reader    │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Parser    │  State machine: LOOKING_FOR_CALENDAR → IN_HEADER → IN_EVENT
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Event     │  parseEventFromBuffer() creates CalendarEvent
│  Builder    │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Filter    │  isEventInRange() checks date boundaries
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Callback   │  EventCallback delivers filtered events
└─────────────┘
```

### Parser States

1. **LOOKING_FOR_CALENDAR**: Searching for `BEGIN:VCALENDAR`
2. **IN_HEADER**: Reading calendar properties before first event
3. **IN_EVENT**: Accumulating event data between `BEGIN:VEVENT` and `END:VEVENT`
4. **DONE**: Reached `END:VCALENDAR`

### Event Buffer

Events are accumulated in a string buffer while in the `IN_EVENT` state:

```cpp
String eventBuffer;

// While in IN_EVENT state:
if (line.startsWith("BEGIN:VEVENT")) {
    eventBuffer = "";
    state = IN_EVENT;
} else if (line.startsWith("END:VEVENT")) {
    CalendarEvent* event = parseEventFromBuffer(eventBuffer);
    if (isEventInRange(event, startDate, endDate)) {
        callback(event);
    }
    state = IN_HEADER;
}
```

---

## Event Processing

### CalendarEvent Structure

```cpp
struct CalendarEvent {
    String summary;          // Event title
    String description;      // Event description
    String location;         // Event location
    String dtStart;          // Start date/time (ICS format)
    String dtEnd;            // End date/time (ICS format)
    String rrule;            // Recurrence rule
    time_t startTime;        // Parsed Unix timestamp
    time_t endTime;          // Parsed Unix timestamp
    bool allDay;             // True for date-only events
    bool isRecurring;        // True if has RRULE
    bool isToday;            // Flagged if event is today
    bool isTomorrow;         // Flagged if event is tomorrow
    String calendarName;     // Source calendar name
    String calendarColor;    // Calendar color identifier
};
```

### Property Extraction

Properties are extracted using a simple key-value parser:

```cpp
String extractValue(const String& line, const String& property) {
    int colonPos = line.indexOf(':');
    if (colonPos != -1 && line.startsWith(property)) {
        return line.substring(colonPos + 1);
    }
    return "";
}
```

**Example**:
```
Input:  "SUMMARY:Team Meeting"
Property: "SUMMARY"
Output: "Team Meeting"
```

### Property Parameters

Properties with parameters are handled by finding the first colon:

```
DTSTART;VALUE=DATE:20251029
         └──────┬──────┘
            Ignored for now
```

Currently, property parameters are **not parsed**. The value after the colon is extracted.

---

## Recurring Events (RRULE)

### Supported Recurrence Rules

Currently supports **FREQ=YEARLY** for:
- Birthdays
- Anniversaries
- Annual holidays

#### Example: Birthday Event

```ics
BEGIN:VEVENT
UID:birthday-john@example.com
DTSTART;VALUE=DATE:19801020
RRULE:FREQ=YEARLY
SUMMARY:John's Birthday
END:VEVENT
```

### Event Expansion

When a recurring event is found, it's expanded for the requested date range:

```cpp
std::vector<CalendarEvent*> expandRecurringEvent(
    CalendarEvent* event,
    time_t startDate,
    time_t endDate
) {
    std::vector<CalendarEvent*> instances;

    // For FREQ=YEARLY: Create instance for each year in range
    struct tm startTm;
    localtime_r(&event->startTime, &startTm);

    time_t rangeStart = startDate;
    time_t rangeEnd = endDate;

    // Iterate through years in the date range
    struct tm iterTm = startTm;
    while (mktime(&iterTm) < rangeEnd) {
        // Create new event instance for this year
        CalendarEvent* instance = new CalendarEvent(*event);
        instance->startTime = mktime(&iterTm);
        // ... set other properties ...
        instances.push_back(instance);

        iterTm.tm_year++; // Next year
    }

    return instances;
}
```

### Future RRULE Support

Planned support for:
- `FREQ=MONTHLY` - Monthly recurring events
- `FREQ=WEEKLY` - Weekly recurring events
- `FREQ=DAILY` - Daily recurring events
- `BYDAY` - Specific days (MO, TU, WE, etc.)
- `BYMONTH` - Specific months
- `COUNT` - Occurrence count limit
- `UNTIL` - End date for recurrence

---

## Memory Management

### Event Ownership

**Important**: CalendarEvent pointers are transferred to the caller. The caller is responsible for cleanup:

```cpp
FilteredEvents* events = parser.fetchEventsInRange(url, start, end);

// Use events...

// Clean up when done
delete events; // FilteredEvents destructor automatically deletes all CalendarEvent*
```

### FilteredEvents Auto-Cleanup

The `FilteredEvents` struct provides automatic cleanup:

```cpp
struct FilteredEvents {
    std::vector<CalendarEvent*> events;

    ~FilteredEvents() {
        for (auto event : events) {
            if (event) {
                delete event;
            }
        }
        events.clear();
    }
};
```

### Memory-Efficient Streaming

- **No full file buffering**: Events are processed line-by-line
- **Date range filtering**: Only relevant events are kept in memory
- **Max events limit**: Prevents memory overflow with large calendars
- **Immediate cleanup**: Events outside date range are deleted immediately

---

## Caching Strategy

### Cache File Structure

Remote calendars can be cached to LittleFS:

```
/cache/
  ├── calendar_personal.ics     (Cached from remote URL)
  ├── calendar_work.ics          (Cached from remote URL)
  └── calendar_holidays.ics      (Cached from remote URL)
```

### TeeStream Implementation

The `TeeStream` class enables simultaneous read and cache write:

```cpp
// Create TeeStream that reads from HTTP and writes to cache file
TeeStream* teeStream = new TeeStream(*httpStream, cacheFile);

// Parser reads from teeStream, automatically caching content
streamParse(url, callback, startDate, endDate, cachePath);
```

### Cache Behavior

1. **On cache miss**: Fetch from remote URL → write to cache → parse
2. **On cache hit**: Read from cache file → parse (no network request)
3. **Cache validation**: Based on calendar manager's cache duration setting

---

## Date/Time Handling

### Supported Date/Time Formats

#### Basic Date (All-day events)
```
DTSTART;VALUE=DATE:20251029
```
- Format: `YYYYMMDD`
- No time component
- Marks event as all-day

#### Date-Time (Timed events)
```
DTSTART:20251029T140000
```
- Format: `YYYYMMDDTHHMMSS`
- Local time (uses system timezone)

#### UTC Date-Time
```
DTSTART:20251029T140000Z
```
- Format: `YYYYMMDDTHHMMSSZ`
- UTC time (converted to local timezone)
- **Note**: UTC conversion currently relies on system timezone

### Date Parsing

```cpp
time_t parseDateTime(const String& dtString) {
    struct tm timeinfo = {0};

    // Parse YYYYMMDD
    timeinfo.tm_year = dtString.substring(0, 4).toInt() - 1900;
    timeinfo.tm_mon = dtString.substring(4, 6).toInt() - 1;
    timeinfo.tm_mday = dtString.substring(6, 8).toInt();

    // Parse THHMMSS if present
    if (dtString.length() >= 15 && dtString.charAt(8) == 'T') {
        timeinfo.tm_hour = dtString.substring(9, 11).toInt();
        timeinfo.tm_min = dtString.substring(11, 13).toInt();
        timeinfo.tm_sec = dtString.substring(13, 15).toInt();
    }

    timeinfo.tm_isdst = -1; // Let system determine DST
    return mktime(&timeinfo);
}
```

### Timezone Handling

- **System Timezone**: Set via `setenv("TZ", timezone, 1)` and `tzset()`
- **Configuration**: Timezone specified in `config.json`
- **VTIMEZONE**: Parsed but not used (relies on system timezone)
- **Future**: Full VTIMEZONE support planned

---

## Known Limitations

### Current Limitations

1. **RRULE Support**: Only `FREQ=YEARLY` is implemented
   - WEEKLY, MONTHLY, DAILY not yet supported
   - BYDAY, BYMONTH, COUNT, UNTIL not yet supported

2. **Property Parameters**: Not parsed or used
   - `DTSTART;TZID=Europe/Rome:...` - TZID is ignored
   - `DTSTART;VALUE=DATE:...` - VALUE parameter not validated

3. **VTIMEZONE**: Parsed but not applied
   - Relies on system timezone setting
   - No per-event timezone conversion

4. **Components**: Only VEVENT is supported
   - VTODO, VJOURNAL, VFREEBUSY not supported
   - VALARM not supported

5. **Attendees**: Not parsed or displayed
   - ORGANIZER, ATTENDEE properties ignored

6. **Attachments**: Not supported
   - ATTACH property ignored

### Memory Constraints

- **Max event buffer**: Limited by available RAM (~50KB typical)
- **Event count limit**: Configurable per calendar (default: 100)
- **Line length limit**: 1024 characters per line
- **Large files**: Files >100KB may cause memory issues

### Network Limitations

- **HTTP only**: No authentication support
- **No proxy**: Direct connection required
- **Timeout**: 10 second connection timeout
- **SSL**: Uses WiFiClientSecure with `setInsecure()` (no certificate validation)

---

## Testing

### Unit Tests

The project includes comprehensive unit tests for ICS parsing:

- **Location**: `test/test_native/test_calendar_stream_parser.cpp`
- **Test Suites**:
  - DateTime parsing (date-only, date-time, UTC)
  - CalendarEvent structure
  - ICS content validation
  - Property extraction
  - Line folding/unfolding
  - Memory management

### Running Tests

```bash
# Run all native tests
pio test -e native

# Run specific test suite
.pio/build/native/program --test-suite="CalendarStreamParser*"

# List all tests
.pio/build/native/program --list-test-cases
```

---

## Related Documentation

- [MULTIPLE_CALENDARS.md](MULTIPLE_CALENDARS.md) - Multiple calendar configuration
- [CHANGELOG.md](../CHANGELOG.md) - Version history and changes
- [MIGRATION_GUIDE.md](../MIGRATION_GUIDE.md) - Migration from older versions

---

## References

- [RFC 5545 - iCalendar](https://tools.ietf.org/html/rfc5545)
- [iCalendar Validator](https://icalendar.org/validator.html)
- [Google Calendar ICS Format](https://support.google.com/calendar/answer/37111)

---

*Last Updated: 2025-10-29*
*Version: 1.7.6*
