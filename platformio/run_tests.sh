#!/bin/bash

# Script to run embedded tests by temporarily excluding conflicting files

echo "Running ESP32-S3 Embedded Tests..."
echo "=================================="

# Backup conflicting files
if [ -f "src/debug.cpp" ]; then
    mv src/debug.cpp src/debug.cpp.test_backup
    echo "Temporarily moved debug.cpp"
fi

if [ -f "src/main.cpp" ]; then
    mv src/main.cpp src/main.cpp.test_backup
    echo "Temporarily moved main.cpp"
fi

# Run the tests
echo "Running tests..."
pio test -e esp32-s3-devkitm-1 -f test_embedded "$@"
TEST_RESULT=$?

# Restore files
if [ -f "src/debug.cpp.test_backup" ]; then
    mv src/debug.cpp.test_backup src/debug.cpp
    echo "Restored debug.cpp"
fi

if [ -f "src/main.cpp.test_backup" ]; then
    mv src/main.cpp.test_backup src/main.cpp
    echo "Restored main.cpp"
fi

echo "=================================="
echo "Test run complete!"

exit $TEST_RESULT