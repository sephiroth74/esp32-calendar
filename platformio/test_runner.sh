#!/bin/bash
# Wrapper script to run doctest with verbose output
# This ensures all test cases are visible in PlatformIO output

PROGRAM="$1"
shift  # Remove first argument (program path)

# Run the test program with --success flag to show all test results
"$PROGRAM" --success --duration "$@"
