#include "string_utils.h"

String StringUtils::convertAccents(const String& text) {
    String result = "";

    for (unsigned int i = 0; i < text.length(); i++) {
        char c = text[i];

        // Check if this is the start of a UTF-8 sequence
        if ((c & 0x80) == 0) {
            // ASCII character, add as-is
            result += c;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8 sequence
            if (i + 1 < text.length()) {
                unsigned char c1 = text[i];
                unsigned char c2 = text[i + 1];
                uint16_t unicode = ((c1 & 0x1F) << 6) | (c2 & 0x3F);

                // Convert common Italian accented characters
                switch (unicode) {
                    case 0x00E8: result += "e'"; break; // è
                    case 0x00E9: result += "e'"; break; // é
                    case 0x00E0: result += "a'"; break; // à
                    case 0x00F2: result += "o'"; break; // ò
                    case 0x00F9: result += "u'"; break; // ù
                    case 0x00EC: result += "i'"; break; // ì
                    case 0x00C8: result += "E'"; break; // È
                    case 0x00C9: result += "E'"; break; // É
                    case 0x00C0: result += "A'"; break; // À
                    case 0x00D2: result += "O'"; break; // Ò
                    case 0x00D9: result += "U'"; break; // Ù
                    case 0x00CC: result += "I'"; break; // Ì
                    // Add more as needed
                    default:
                        // For other Unicode characters, try to find ASCII equivalent
                        if (unicode >= 0x00C0 && unicode <= 0x00FF) {
                            // Latin-1 Supplement range, use simplified mapping
                            result += '?';
                        } else {
                            result += '?';
                        }
                }
                i++; // Skip the second byte
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 sequence
            if (i + 2 < text.length()) {
                // For now, replace with ?
                result += '?';
                i += 2; // Skip the next two bytes
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8 sequence
            if (i + 3 < text.length()) {
                // For now, replace with ?
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

String StringUtils::truncate(const String& text, size_t maxLength, const String& suffix) {
    if (text.length() <= maxLength) {
        return text;
    }

    if (maxLength <= suffix.length()) {
        return suffix.substring(0, maxLength);
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
    String result = text;
    int index = 0;

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
    String result = text;
    bool nextUpper = true;

    for (unsigned int i = 0; i < result.length(); i++) {
        if (isalpha(result[i])) {
            if (nextUpper) {
                result[i] = toupper(result[i]);
                nextUpper = false;
            } else {
                result[i] = tolower(result[i]);
            }
        } else if (isspace(result[i])) {
            nextUpper = true;
        }
    }

    return result;
}