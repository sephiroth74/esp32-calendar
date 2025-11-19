# Multiple Calendar Support

As of version 1.4.0, the ESP32 E-Paper Calendar supports fetching and merging events from multiple calendar sources.

## Features

- **Multiple ICS Sources**: Fetch events from up to 3 ICS calendar URLs (remote or local)
- **Local ICS Files**: Support for ICS files stored in LittleFS partition
- **Individual Calendar Settings**: Each calendar has its own name, color, and enabled status
- **Automatic Event Merging**: Events from all calendars are automatically merged and sorted chronologically
- **Color-Coded Indicators**: On 6-color displays, calendar events show colored circles based on source
- **Enable/Disable Calendars**: Temporarily disable calendars without removing them from configuration
- **Backward Compatibility**: Existing single-calendar configurations continue to work

## Configuration

### New Format (Multiple Calendars)

Create or update your `config.json` file with the new `calendars` array:

```json
{
  "wifi": {
    "ssid": "YourWiFiSSID",
    "password": "YourWiFiPassword"
  },
  "location": {
    "name": "Zurich",
    "latitude": 47.325138,
    "longitude": 8.514618
  },
  "calendars": [
    {
      "name": "Personal Calendar",
      "url": "https://calendar.google.com/calendar/ical/personal-id/basic.ics",
      "color": "red",
      "enabled": true
    },
    {
      "name": "Work Calendar",
      "url": "https://calendar.google.com/calendar/ical/work-id/basic.ics",
      "color": "blue",
      "enabled": true
    },
    {
      "name": "Family Calendar",
      "url": "https://calendar.google.com/calendar/ical/family-id/basic.ics",
      "color": "green",
      "enabled": true
    },
    {
      "name": "Holidays",
      "url": "https://calendar.google.com/calendar/ical/holidays/basic.ics",
      "color": "orange",
      "enabled": false
    }
  ],
  "days_to_fetch": 30,
  "display": {
    "timezone": "CET-1CEST,M3.5.0,M10.5.0",
    "update_hours": [6, 12, 18]
  }
}
```

### Calendar Properties

- **name**: Display name for the calendar (shown in debug logs)
- **url**: Full ICS calendar URL
- **color**: Color identifier for this calendar's events (for future display features)
- **enabled**: Set to `false` to temporarily disable a calendar without removing it

### Legacy Format (Single Calendar)

The old format is still supported for backward compatibility:

```json
{
  "calendar": {
    "url": "https://calendar.google.com/calendar/ical/your-id/basic.ics",
    "days_to_fetch": 30
  }
}
```

## Local ICS Files

As of version 1.4.1, you can store ICS files directly on the ESP32's LittleFS filesystem:

### Setting Up Local ICS Files

1. **Upload ICS files to LittleFS**: Store your `.ics` files in the LittleFS partition
2. **Reference in config**: Use a path starting with `/` or `local://` prefix

### Example Configuration

```json
{
  "calendars": [
    {
      "name": "Remote Google Calendar",
      "url": "https://calendar.google.com/calendar/ical/...",
      "color": "red",
      "enabled": true
    },
    {
      "name": "Local Holidays",
      "url": "/calendars/holidays.ics",
      "color": "green",
      "enabled": true
    },
    {
      "name": "Local Events",
      "url": "local:///events/company.ics",
      "color": "orange",
      "enabled": true
    }
  ]
}
```

### Advantages of Local ICS Files

- **No Internet Required**: Events display even without WiFi (after initial sync)
- **Faster Loading**: No HTTP requests needed for local files
- **Privacy**: Keep sensitive calendars completely offline
- **Manual Control**: Update calendars only when you choose

### File Size Limits

- Maximum file size: 500KB per ICS file
- Recommended total: Keep under 1MB for all local ICS files combined

## How It Works

1. **Parallel Fetching**: The system fetches events from each enabled calendar
2. **Event Metadata**: Each event is tagged with its source calendar name and color
3. **Merging**: All events are combined into a single list
4. **Sorting**: Events are sorted chronologically by date and time
5. **Display**: Events are shown in order, regardless of source calendar

## Debug Output

When fetching from multiple calendars, you'll see debug output like:

```
--- Fetching from 3 calendars ---

Fetching calendar: Personal Calendar
  Retrieved 5 events

Fetching calendar: Work Calendar
  Retrieved 12 events

Fetching calendar: Family Calendar
  No events retrieved or fetch failed

--- Calendar fetch summary ---
Successful: 2 calendars
Failed: 1 calendars
Total events: 17
Events sorted chronologically
```

## Google Calendar URLs

To find your Google Calendar ICS URLs:

1. Open Google Calendar
2. Click the gear icon → Settings
3. Select the calendar from the left sidebar
4. Scroll to "Integrate calendar"
5. Copy the "Secret address in iCal format"

## Tips

- **Performance**: Each calendar requires a separate HTTPS request, so limit to essential calendars
- **Memory**: More calendars mean more memory usage - monitor PSRAM usage in debug output
- **Colors**: Choose distinct colors for each calendar for future visual differentiation
- **Testing**: Use the `enabled` flag to test with different calendar combinations
- **Debugging**: Check serial output to see which calendars are being fetched and their status

## Troubleshooting

- **Calendar not fetching**: Check the URL is accessible and in ICS format
- **Missing events**: Verify the calendar is enabled and has events in the configured date range
- **Memory issues**: Reduce the number of calendars or decrease `days_to_fetch`
- **Slow updates**: Each calendar adds ~2-5 seconds to the update time

## Future Enhancements

The color metadata is currently stored but not displayed. Future versions may:
- Show color indicators on the calendar grid
- Display calendar names in the event list
- Add color-coded event bullets
- Provide filtering by calendar source