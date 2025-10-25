#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <Arduino.h>

class StringUtils {
public:
    // Convert Unicode accented characters to ASCII approximations
    // e.g., è → e', à → a', ò → o', ù → u', ì → i'
    static String convertAccents(const String& text);

    // Alias for convertAccents (removes accented characters)
    static String removeAccents(const String& text) { return convertAccents(text); }

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