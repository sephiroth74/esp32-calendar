#ifndef TIMEZONE_MAP_H
#define TIMEZONE_MAP_H

#include <map>

// Note: This header requires String class to be defined before including
// It's automatically available in Arduino (WString.h) or mock_arduino.h for tests

/**
 * IANA Timezone ID to POSIX TZ String Mapping
 *
 * POSIX TZ format: std offset [dst [offset] [,start[/time],end[/time]]]
 * Example: "CET-1CEST,M3.5.0,M10.5.0/3"
 *   - CET: Standard timezone abbreviation
 *   - -1: UTC offset (negative means east of Greenwich)
 *   - CEST: Daylight saving time abbreviation
 *   - M3.5.0: Start DST on last Sunday (5) of March (3) at 2am (default)
 *   - M10.5.0/3: End DST on last Sunday (5) of October (10) at 3am
 *
 * Sources:
 * - https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
 * - https://github.com/nayarsystems/posix_tz_db
 */

// IANA Timezone ID → POSIX TZ String mapping
static const std::map<String, String> IANA_TO_POSIX_TZ = {
    // Western Europe (UTC+0/+1)
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Dublin", "GMT0IST,M3.5.0/1,M10.5.0"},
    {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0"},

    // Central Europe (UTC+1/+2)
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Vienna", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Zurich", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3"},

    // Eastern Europe (UTC+2/+3)
    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Bucharest", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},

    // US Eastern (UTC-5/-4)
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Detroit", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Toronto", "EST5EDT,M3.2.0,M11.1.0"},

    // US Central (UTC-6/-5)
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Mexico_City", "CST6CDT,M4.1.0,M10.5.0"},

    // US Mountain (UTC-7/-6)
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},

    // US Pacific (UTC-8/-7)
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Vancouver", "PST8PDT,M3.2.0,M11.1.0"},

    // Other common timezones
    {"Asia/Tokyo", "JST-9"},
    {"Asia/Hong_Kong", "HKT-8"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Singapore", "SGT-8"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
};

/**
 * Get POSIX TZ string for an IANA timezone identifier
 *
 * @param ianaID IANA timezone identifier (e.g., "Europe/Amsterdam")
 * @return POSIX TZ string, or empty string if timezone not found
 */
inline String getPosixTZ(const String& ianaID) {
    auto it = IANA_TO_POSIX_TZ.find(ianaID);
    if (it != IANA_TO_POSIX_TZ.end()) {
        return it->second;
    }
    return "";  // Unknown timezone - caller should handle this
}

/**
 * Check if a timezone ID is supported
 *
 * @param ianaID IANA timezone identifier
 * @return true if timezone is in the mapping table
 */
inline bool isSupportedTimezone(const String& ianaID) {
    return IANA_TO_POSIX_TZ.find(ianaID) != IANA_TO_POSIX_TZ.end();
}

#endif // TIMEZONE_MAP_H
