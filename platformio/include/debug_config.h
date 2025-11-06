#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

// Debug levels - set DEBUG_LEVEL to control verbosity
#define DEBUG_NONE    0 // No debug output
#define DEBUG_ERROR   1 // Only errors
#define DEBUG_WARNING 2 // Errors and warnings
#define DEBUG_INFO    3 // Errors, warnings, and info messages
#define DEBUG_VERBOSE 4 // Everything including detailed debug

// Set the current debug level here (can be overridden in platformio.ini)
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_INFO // Default to INFO level
#endif

// Debug macros for different levels
#if DEBUG_LEVEL >= DEBUG_ERROR
#define DEBUG_ERROR_PRINT(x)    Serial.print(x)
#define DEBUG_ERROR_PRINTLN(x)  Serial.println(x)
#define DEBUG_ERROR_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_ERROR_PRINT(x)
#define DEBUG_ERROR_PRINTLN(x)
#define DEBUG_ERROR_PRINTF(...)
#endif

#if DEBUG_LEVEL >= DEBUG_WARNING
#define DEBUG_WARN_PRINT(x)    Serial.print(x)
#define DEBUG_WARN_PRINTLN(x)  Serial.println(x)
#define DEBUG_WARN_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_WARN_PRINT(x)
#define DEBUG_WARN_PRINTLN(x)
#define DEBUG_WARN_PRINTF(...)
#endif

#if DEBUG_LEVEL >= DEBUG_INFO
#define DEBUG_INFO_PRINT(x)          Serial.print(x)
#define DEBUG_INFO_PRINTLN(x)        Serial.println(x)
#define DEBUG_INFO_PRINTF(...)       Serial.printf(__VA_ARGS__)
#define DEBUG_INFO_PRINT_FLOAT(x, y) Serial.print((double)x, (int)y)
#else
#define DEBUG_INFO_PRINT(x)
#define DEBUG_INFO_PRINTLN(x)
#define DEBUG_INFO_PRINTF(...)
#define DEBUG_INFO_PRINT_FLOAT(x, y)
#endif

#if DEBUG_LEVEL >= DEBUG_VERBOSE
#define DEBUG_VERBOSE_PRINT(x)          Serial.print(x)
#define DEBUG_VERBOSE_PRINTLN(x)        Serial.println(x)
#define DEBUG_VERBOSE_PRINTF(...)       Serial.printf(__VA_ARGS__)
#define DEBUG_VERBOSE_PRINT_FLOAT(x, y) Serial.print((double)x, (int)y)
#else
#define DEBUG_VERBOSE_PRINT(x)
#define DEBUG_VERBOSE_PRINTLN(x)
#define DEBUG_VERBOSE_PRINTF(...)
#define DEBUG_VERBOSE_PRINT_FLOAT(x, y)
#endif

// Convenience macros for common debug patterns
#define DEBUG_SEPARATOR()   DEBUG_INFO_PRINTLN("=====================================")
#define DEBUG_SECTION(name) DEBUG_INFO_PRINTLN("\n--- " name " ---")

// Memory debug helpers (only in verbose mode)
#if DEBUG_LEVEL >= DEBUG_VERBOSE
#define DEBUG_MEMORY()                                                                             \
    do {                                                                                           \
        DEBUG_VERBOSE_PRINTF("Free heap: %d bytes, Max block: %d bytes\n",                         \
                             ESP.getFreeHeap(),                                                    \
                             ESP.getMaxAllocHeap());                                               \
        if (psramFound()) {                                                                        \
            DEBUG_VERBOSE_PRINTF("PSRAM free: %d bytes\n", ESP.getFreePsram());                    \
        }                                                                                          \
    } while (0)
#else
#define DEBUG_MEMORY()
#endif

// Timing helpers for performance debugging
#if DEBUG_LEVEL >= DEBUG_VERBOSE
#define DEBUG_TIMING_START(name) unsigned long _timing_##name = millis()
#define DEBUG_TIMING_END(name)                                                                     \
    DEBUG_VERBOSE_PRINTF("%s took %lu ms\n", #name, millis() - _timing_##name)
#else
#define DEBUG_TIMING_START(name)
#define DEBUG_TIMING_END(name)
#endif

// Function entry/exit tracing (only in verbose mode)
#if DEBUG_LEVEL >= DEBUG_VERBOSE
#define DEBUG_ENTER_FUNCTION(name) DEBUG_VERBOSE_PRINTLN(">> " name)
#define DEBUG_EXIT_FUNCTION(name)  DEBUG_VERBOSE_PRINTLN("<< " name)
#else
#define DEBUG_ENTER_FUNCTION(name)
#define DEBUG_EXIT_FUNCTION(name)
#endif

#endif // DEBUG_CONFIG_H