# Configuration Guide - ESP32 E-Paper Calendar

This document explains how to configure the ESP32 E-Paper Calendar using the LittleFS filesystem configuration system.

## Overview

The ESP32 Calendar loads all configuration from a JSON file stored in the LittleFS filesystem. This approach provides:

- ✅ **No recompilation needed** - Change settings without rebuilding firmware
- ✅ **Secure credentials** - WiFi passwords stay out of source control
- ✅ **Easy deployment** - Configure multiple devices with different settings
- ✅ **Simple updates** - Change configuration via filesystem upload

## Configuration File Structure

The configuration is stored in `/data/config.json` with the following structure:

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD"
  },
  "location": {
    "latitude": 40.7128,
    "longitude": -74.0060,
    "name": "New York"
  },
  "calendar": {
    "type": "ics",
    "url": "https://calendar.google.com/calendar/ical/YOUR_CALENDAR_ID/public/basic.ics",
    "days_to_fetch": 30
  },
  "display": {
    "timezone": "EST5EDT,M3.2.0,M11.1.0",
    "update_hour": 5
  }
}
```

## Setup Instructions

### 1. Prepare Configuration File

1. Copy the example configuration:
   ```bash
   cp data/config.json.example data/config.json
   ```

2. Edit `data/config.json` with your settings:
   - **WiFi credentials**: Your network SSID and password
   - **Location**: Latitude/longitude for weather data
   - **Calendar URL**: Your ICS calendar feed URL
   - **Timezone**: Your timezone string (see examples below)

### 2. Upload Filesystem

Upload the configuration to the ESP32's filesystem:

```bash
pio run -t uploadfs -e esp32-s3-devkitc-1
```

This command will:
- Create a LittleFS image from the `data/` directory
- Upload it to the ESP32's flash memory
- The configuration will persist across power cycles

### 3. Upload Firmware

After uploading the filesystem, upload the main firmware:

```bash
pio run -t upload -e esp32-s3-devkitc-1
```

### 4. Monitor Output

Check that configuration loaded correctly:

```bash
pio device monitor
```

You should see output like:
```
LittleFS mounted successfully
Configuration loaded successfully from LittleFS
=== Current Configuration ===
WiFi:
  SSID: YourNetwork
  Password: ********
Location:
  Name: New York
  Latitude: 40.712800
  Longitude: -74.006000
```

## Testing Configuration Reset

The firmware includes a test mode for configuration reset:

### Enable Test Mode

In `include/config.h`, ensure:
```c
#define DISABLE_DEEP_SLEEP true
```

This keeps the device awake to monitor button presses.

### Reset Configuration

1. Hold the button (GPIO 2) for 3 seconds
2. The display will show "Configuration Reset"
3. Device will restart
4. You'll need to upload a new config.json

### Production Mode

For normal operation, disable test mode:
```c
#define DISABLE_DEEP_SLEEP false
```

The device will then enter deep sleep between updates to save battery.

## Timezone Examples

Common timezone strings for the `display.timezone` field:

- **US Eastern**: `"EST5EDT,M3.2.0,M11.1.0"`
- **US Pacific**: `"PST8PDT,M3.2.0,M11.1.0"`
- **US Central**: `"CST6CDT,M3.2.0,M11.1.0"`
- **UK**: `"GMT0BST,M3.5.0,M10.5.0"`
- **Central Europe**: `"CET-1CEST,M3.5.0,M10.5.0"`
- **Japan**: `"JST-9"`
- **Australia Sydney**: `"AEST-10AEDT,M10.1.0,M4.1.0"`

## Google Calendar URL

To get your Google Calendar ICS URL:

1. Open Google Calendar
2. Go to Settings → Settings for my calendars
3. Select your calendar
4. Scroll to "Integrate calendar"
5. Copy the "Secret address in iCal format"
6. Use this URL in the `calendar.url` field

## Troubleshooting

### Configuration Not Loading

If you see "Configuration Missing" on the display:

1. Verify config.json exists in the data/ directory
2. Check that WiFi credentials are set (not empty)
3. Re-upload filesystem: `pio run -t uploadfs`
4. Monitor serial output for detailed error messages

### WiFi Connection Failed

1. Verify SSID and password are correct
2. Check that the network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Ensure the device is within range

### Filesystem Upload Failed

If `uploadfs` fails:

1. Make sure the device is connected
2. Try erasing flash first: `pio run -t erase`
3. Then upload filesystem again

## File Locations

- **Configuration file**: `data/config.json` (git-ignored)
- **Example template**: `data/config.json.example` (in repository)
- **Configuration class**: `src/littlefs_config.cpp`
- **Test settings**: `include/config.h`

## Security Notes

- The `data/config.json` file is in `.gitignore` to prevent committing credentials
- Always use `config.json.example` as a template
- Never commit actual WiFi passwords or API keys
- The configuration is stored in plain text on the device's flash memory