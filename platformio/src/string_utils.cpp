#include "string_utils.h"

String StringUtils::convertToFontEncoding(const String& text) {
    String result = "";

    for (unsigned int i = 0; i < text.length(); i++) {
        unsigned char c = text[i];

        // Check if this is the start of a UTF-8 sequence
        if ((c & 0x80) == 0) {
            // ASCII character (0x00-0x7F), add as-is
            result += (char)c;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8 sequence
            if (i + 1 < text.length()) {
                unsigned char c1 = text[i];
                unsigned char c2 = text[i + 1];
                // Decode UTF-8 to Unicode code point
                uint16_t unicode = ((c1 & 0x1F) << 6) | (c2 & 0x3F);

                // Check if character is in Latin-1 range (0x80-0xFF)
                // GFXfont supports 0x20-0xFF range
                if (unicode >= 0x0020 && unicode <= 0x00FF) {
                    // Convert to Latin-1 single-byte character
                    result += (char)unicode;
                } else {
                    // Character not in Latin-1, use fallback
                    result += '?';
                }
                i++; // Skip the second byte
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 sequence
            if (i + 2 < text.length()) {
                unsigned char c1 = text[i];
                unsigned char c2 = text[i + 1];
                unsigned char c3 = text[i + 2];
                // Decode UTF-8 to Unicode code point
                uint32_t unicode = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);

                // Check if somehow in Latin-1 range (unlikely but handle it)
                if (unicode >= 0x0020 && unicode <= 0x00FF) {
                    result += (char)unicode;
                } else {
                    // Not in Latin-1, use fallback
                    result += '?';
                }
                i += 2; // Skip the next two bytes
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8 sequence (definitely outside Latin-1)
            if (i + 3 < text.length()) {
                result += '?';
                i += 3; // Skip the next three bytes
            }
        } else {
            // Invalid UTF-8 or continuation byte, skip
            result += '?';
        }
    }

    return result;
}

String StringUtils::convertAccents(const String& text) {
    return convertToFontEncoding(text); // Use new encoding conversion
}

String StringUtils::removeAccents(const String& text) {
    return convertToFontEncoding(text); // Use new encoding conversion
}

String StringUtils::truncate(const String& text, size_t maxLength, const String& suffix) {
    if (text.length() <= maxLength) {
        return text;
    }

    // If maxLength is too small to fit even part of text + suffix, just return suffix
    if (maxLength <= suffix.length()) {
        return suffix;
    }

    return text.substring(0, maxLength - suffix.length()) + suffix;
}

String StringUtils::trim(const String& text) {
    String result = text;

    // Trim leading whitespace
    while (result.length() > 0 && isspace(result[0])) {
        result.remove(0, 1);
    }

    // Trim trailing whitespace
    while (result.length() > 0 && isspace(result[result.length() - 1])) {
        result.remove(result.length() - 1);
    }

    return result;
}

String StringUtils::replaceAll(const String& text, const String& from, const String& to) {
    // Handle empty 'from' string to avoid infinite loop
    if (from.length() == 0) {
        return text;
    }

    String result = text;
    int index     = 0;

    while ((index = result.indexOf(from, index)) != -1) {
        result = result.substring(0, index) + to + result.substring(index + from.length());
        index += to.length();
    }

    return result;
}

bool StringUtils::startsWith(const String& text, const String& prefix) {
    if (prefix.length() > text.length()) {
        return false;
    }

    return text.substring(0, prefix.length()) == prefix;
}

bool StringUtils::endsWith(const String& text, const String& suffix) {
    if (suffix.length() > text.length()) {
        return false;
    }

    return text.substring(text.length() - suffix.length()) == suffix;
}

String StringUtils::toTitleCase(const String& text) {
    String result  = text;
    bool nextUpper = true;

    for (unsigned int i = 0; i < result.length(); i++) {
        if (isalpha(result[i])) {
            if (nextUpper) {
                result[i] = toupper(result[i]);
                nextUpper = false;
            } else {
                result[i] = tolower(result[i]);
            }
        } else {
            // Any non-alphabetic character (space, hyphen, dot, number, etc.) starts a new word
            nextUpper = true;
        }
    }

    return result;
}