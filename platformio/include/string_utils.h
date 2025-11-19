#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#ifdef NATIVE_TEST
#include "mock_arduino.h"
#else
#include <Arduino.h>
#endif

class StringUtils {
  public:
    // Convert UTF-8 encoded text to Latin-1 encoding for GFXfont rendering
    // Characters in Latin-1 range (0x20-0xFF) are preserved and converted
    // to their single-byte representation for GFX fonts.
    // For example: UTF-8 "à" (\xc3\xa0) → Latin-1 '\340' (octal for 0xE0)
    // Characters outside Latin-1 range are replaced with '?'
    static String convertToFontEncoding(const String& text);

    // Legacy alias - now converts to font encoding instead of ASCII approximations
    static String convertAccents(const String& text);

    // Alias for convertToFontEncoding
    static String removeAccents(const String& text);

    // Truncate string to specified length with ellipsis
    static String truncate(const String& text, size_t maxLength, const String& suffix = "...");

    // Trim whitespace from both ends
    static String trim(const String& text);

    // Replace all occurrences of a substring
    static String replaceAll(const String& text, const String& from, const String& to);

    // Check if string starts with prefix
    static bool startsWith(const String& text, const String& prefix);

    // Check if string ends with suffix
    static bool endsWith(const String& text, const String& suffix);

    // Convert to title case (first letter uppercase)
    static String toTitleCase(const String& text);
};

#endif // STRING_UTILS_H