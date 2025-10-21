# ESP32 Calendar Tests

This directory contains comprehensive tests for the ESP32 Calendar project.

## Test Structure

### Native Tests (runs on computer)
- `test_ics_parser.cpp` - Comprehensive ICS parser tests with mocked Arduino environment
- `test_config_simple.cpp` - Simple configuration structure tests

These tests use doctest framework and run on your development machine without needing hardware.

### Embedded Tests (runs on ESP32 device)
- `test_embedded/test_ics_parser_esp32.cpp` - ICS parser tests on actual ESP32
- `test_embedded/test_config_loader_esp32.cpp` - Configuration loading tests with real LittleFS

These tests use Unity framework and run directly on the ESP32 hardware.

## Running Tests

### Native Tests (on computer)
```bash
# Run all native tests
pio test -e native

# Run with verbose output
pio test -e native --verbose
```

### Embedded Tests (on ESP32 device)
First, connect your ESP32-S3 device via USB, then:

```bash
# Run all embedded tests on device
pio test -e esp32-s3-devkitm-1

# Run with specific port
pio test -e esp32-s3-devkitm-1 --upload-port /dev/cu.usbmodem14101

# Monitor test output
pio test -e esp32-s3-devkitm-1 --verbose
```

## Test Coverage

### ICS Parser Tests
- **63 native test cases** covering:
  - Basic parsing and validation
  - Event extraction and counting
  - Date range filtering
  - Recurring event support (RRULE)
  - RFC 5545 date-time formats
  - Swiss holidays calendar (real-world test)
  - Memory management

- **10 embedded test cases** covering:
  - Parser initialization
  - Simple calendar parsing
  - Event parsing (timed, all-day, recurring)
  - Date range filtering
  - Memory management on ESP32
  - File loading from LittleFS
  - RRULE parsing
  - Folded lines (RFC 5545)
  - Timezone support
  - Clear and reload

### Configuration Tests
- **8 embedded test cases** covering:
  - LittleFS initialization
  - Default configuration
  - Save and load configuration
  - Multiple calendars support
  - Local calendar URLs
  - Calendar management (add/remove)
  - Configuration persistence
  - Backward compatibility

## Key Features Tested

### Date-Time Format Support (RFC 5545)
- Local time: `DTSTART:19970714T133000`
- UTC time: `DTSTART:19970714T173000Z`
- Timezone reference: `DTSTART;TZID=America/New_York:19970714T133000`
- Date only (all-day): `DTSTART;VALUE=DATE:19970714`

### Recurring Events
- YEARLY (birthdays, anniversaries)
- MONTHLY (monthly meetings)
- WEEKLY (weekly events)
- With INTERVAL, COUNT, UNTIL parameters

### Multi-Calendar Support
Each calendar can have:
- Individual `days_to_fetch` setting
- Custom color coding
- Enable/disable flag
- Local or remote URL

## Memory Safety

All tests verify proper memory management:
- No memory leaks
- Proper cleanup on destruction
- RAII pattern compliance
- ESP32 heap monitoring

## Adding New Tests

### For Native Tests
1. Create test file in `test/` directory
2. Include doctest and mock headers
3. Write test cases using `TEST_CASE` macro

### For Embedded Tests
1. Create test file in `test/test_embedded/` directory
2. Include Unity framework
3. Write test functions with `test_` prefix
4. Register tests in `setup()` with `RUN_TEST`

## Troubleshooting

### Native Tests Fail to Compile
- Check mock implementations in `test/mock_arduino.h`
- Ensure `NATIVE_TEST` is defined

### Embedded Tests Fail to Upload
- Check USB connection
- Verify correct port in platformio.ini
- Try different USB cable/port

### Tests Pass Locally but Fail on Device
- Check available heap memory
- Verify PSRAM is enabled
- Monitor serial output for detailed error messages

## Test Statistics

- **Total test cases**: 81+ (63 native + 18 embedded)
- **Code coverage**: ~90% of parser functionality
- **Memory tested**: Stack allocation, heap management, PSRAM usage
- **Platforms tested**: Native (macOS/Linux/Windows), ESP32-S3