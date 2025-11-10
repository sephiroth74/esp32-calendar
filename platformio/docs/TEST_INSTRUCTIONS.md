# Running Tests for ESP32 Calendar Project

## Overview
This project includes comprehensive unit tests for both native (desktop) and embedded (ESP32) environments.

## Test Structure

### Native Tests (`test/test_native/`)
- Tests that can run on your development machine
- Use the doctest framework
- Test business logic without hardware dependencies

### Embedded Tests (`test/test_embedded/`)
- Tests that run on the actual ESP32 hardware
- Use the Unity test framework
- Test hardware-specific functionality including:
  - Configuration loading from LittleFS
  - ICS calendar parsing with PSRAM
  - Memory management

## Running Tests

### Important Note About Test Compilation
Due to the structure of embedded tests, the `debug.cpp` and `main.cpp` files contain their own `setup()` and `loop()` functions which conflict with the test runner. To work around this:

### Method 1: Use the Test Script (Recommended)
```bash
# Run all embedded tests
./run_tests.sh

# Build tests without uploading
./run_tests.sh --without-uploading

# Build tests without testing
./run_tests.sh --without-testing --without-uploading
```

The script automatically:
1. Temporarily moves conflicting files (`debug.cpp` and `main.cpp`)
2. Runs the tests
3. Restores the files after testing

### Method 2: Manual Testing
```bash
# Temporarily move conflicting files
mv src/debug.cpp src/debug.cpp.bak
mv src/main.cpp src/main.cpp.bak

# Run tests
pio test -e esp32-s3-devkitm-1

# Restore files
mv src/debug.cpp.bak src/debug.cpp
mv src/main.cpp.bak src/main.cpp
```

### Method 3: Run Native Tests Only
```bash
pio test -e native
```

## Test Files

### `test_main.cpp`
The main test runner that coordinates all embedded tests. Contains the only `setup()` and `loop()` functions for testing.

### `test_config_loader_esp32.cpp`
Tests for configuration loading from LittleFS:
- LittleFS initialization
- Default configuration handling
- Multiple calendar support
- Configuration save/load
- Backward compatibility

### `test_ics_parser_esp32.cpp`
Tests for ICS calendar parsing:
- Parser initialization
- Event parsing
- Date range filtering
- Recurring events (RRULE)
- Timezone support
- Memory management

### `test_psram_usage.cpp`
Tests for PSRAM memory management:
- PSRAM allocation
- Large calendar handling
- Memory cleanup

## Troubleshooting

### Tests are Skipped
If tests show as "SKIPPED", ensure you're using the correct filter:
```bash
pio test -e esp32-s3-devkitm-1 -f test_embedded
```

### Multiple Definition Errors
This occurs when `debug.cpp` or `main.cpp` are included in the test build. Use the provided script or manually move these files as described above.

### Unity Test Output
Test results are output via Serial. If testing on hardware, ensure:
- The device is connected
- Serial monitor baud rate is 115200
- Reset the board if no output appears

## Configuration

The test configuration is in `platformio.ini`:
```ini
[env:esp32-s3-devkitm-1]
test_framework = unity
test_build_src = yes  # Include source files for testing
test_ignore = test_native  # Ignore native tests for embedded environment
test_port = /dev/cu.usbmodem*  # Adjust for your system
test_speed = 115200
```

## Adding New Tests

1. Add test functions to the appropriate test file
2. Add forward declarations in `test_main.cpp`
3. Add `RUN_TEST()` calls in `test_main.cpp`
4. Follow Unity test framework conventions:
   - Test functions start with `test_`
   - Use `TEST_ASSERT_*` macros for assertions
   - Each test should be independent

## Continuous Integration

For CI/CD pipelines, use:
```bash
# Build and run tests without user interaction
./run_tests.sh --without-uploading --without-testing
```

This ensures tests compile correctly without requiring hardware connection.