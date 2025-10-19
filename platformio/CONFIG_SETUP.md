# Configuration Setup Guide

## Protecting Your Sensitive Data

This project uses a two-file configuration system to keep your sensitive data (WiFi credentials and location) secure and out of version control.

## Initial Setup

### 1. Create Your Local Configuration

Before you can compile the project, you need to create your local configuration file:

```bash
cd include/
cp config.local.example config.local
```

### 2. Edit config.local

Open `include/config.local` in your editor and fill in your actual values:

```cpp
// Your WiFi credentials
#define WIFI_SSID "YOUR_WIFI_NETWORK_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Your location coordinates
#define LOC_LATITUDE 40.7128    // Example: New York City
#define LOC_LONGITUDE -74.0060   // Example: New York City
```

To find your coordinates, visit: https://www.latlong.net/

### 3. Compile and Upload

Now you can compile and upload the project:

```bash
pio run -t upload -e esp32-s3-devkitc-1
```

## Configuration Files

### config.h
- **Purpose**: Contains all non-sensitive configuration
- **Git Status**: ✅ Tracked in git
- **Contains**: Display settings, pin configuration, calendar settings, etc.

### config.local
- **Purpose**: Contains sensitive data
- **Git Status**: ❌ Excluded from git (.gitignore)
- **Contains**: WiFi credentials, location coordinates

### config.local.example
- **Purpose**: Template for creating config.local
- **Git Status**: ✅ Tracked in git
- **Contains**: Example structure with placeholder values

## Why This Structure?

1. **Security**: Your WiFi password and location never get committed to git
2. **Privacy**: Your personal data stays on your local machine
3. **Sharing**: You can safely share your code without exposing credentials
4. **Collaboration**: Others can use their own credentials without conflicts

## Troubleshooting

### Compilation Error: config.local not found

**Solution**: You forgot to create config.local. Follow step 1 above.

### WiFi Won't Connect

**Solution**: Double-check your WiFi credentials in config.local:
- Ensure SSID is exact (case-sensitive)
- Check for special characters in password
- Verify your network is 2.4GHz (ESP32 doesn't support 5GHz)

### Weather Shows Wrong Location

**Solution**: Update LOC_LATITUDE and LOC_LONGITUDE in config.local
- Use positive values for North/East
- Use negative values for South/West

## Adding More Sensitive Data

If you need to add more sensitive configuration:

1. Add the constants to `config.local`
2. Add example placeholders to `config.local.example`
3. Remove the constants from `config.h` if they were there
4. Document the new constants in this guide

## Version Control Best Practices

✅ **DO commit**:
- config.h
- config.local.example
- .gitignore

❌ **DON'T commit**:
- config.local
- Any file with passwords or API keys

## For Contributors

When contributing to this project:
1. Never add sensitive data to config.h
2. Always use config.local for credentials
3. Update config.local.example if adding new sensitive fields
4. Document any new configuration in this guide