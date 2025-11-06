#ifndef EVENT_CACHE_H
#define EVENT_CACHE_H

#ifdef NATIVE_TEST
#include "../test/mock_arduino.h"
#include "../test/mock_littlefs.h"
#else
#include <Arduino.h>
#include <LittleFS.h>
#endif

#include "calendar_event.h"
#include "config.h"
#include <vector>

/**
 * @brief Binary event cache manager for LittleFS storage
 *
 * EventCache provides efficient binary serialization and deserialization of
 * CalendarEvent objects to/from LittleFS. This approach:
 * - Stores only processed events (not raw ICS files)
 * - Uses compact binary format (~200 bytes per event)
 * - Enables offline fallback when network fails
 * - Handles unlimited source file sizes via stream parsing
 *
 * Cache file format:
 * - Header: Magic number, version, metadata, checksum
 * - Body: Array of serialized events
 *
 * Typical usage: 3 calendars × 50 events = ~30KB total storage
 */
class EventCache {
  public:
    /**
     * @brief Save events to binary cache file
     *
     * Serializes the event vector to a compact binary format with header
     * validation and checksum. Creates parent directories if needed.
     *
     * @param cachePath Full path to cache file (e.g., "/cache/events_abc123.bin")
     * @param events Vector of CalendarEvent pointers to serialize
     * @param calendarUrl Original calendar URL for validation
     * @return true if save succeeded
     */
    static bool save(const String& cachePath,
                     const std::vector<CalendarEvent*>& events,
                     const String& calendarUrl);

    /**
     * @brief Load events from binary cache file
     *
     * Deserializes events from binary cache. Validates header magic number,
     * version compatibility, and checksum. Returns empty vector on any error.
     *
     * @param cachePath Full path to cache file
     * @param calendarUrl Expected calendar URL (for validation)
     * @return Vector of CalendarEvent pointers (caller must delete), empty on failure
     */
    static std::vector<CalendarEvent*> load(const String& cachePath, const String& calendarUrl);

    /**
     * @brief Check if cache file is valid and not expired
     *
     * Validates cache file existence, magic number, version, and age.
     * Does not perform full integrity check (use load() for that).
     *
     * @param cachePath Full path to cache file
     * @param maxAge Maximum cache age in seconds (default from config.h)
     * @return true if cache exists and is valid
     */
    static bool isValid(const String& cachePath, time_t maxAge = EVENT_CACHE_VALIDITY_SECONDS);

    /**
     * @brief Delete cache file
     *
     * Removes the cache file from LittleFS. Safe to call even if file doesn't exist.
     *
     * @param cachePath Full path to cache file
     * @return true if file was deleted or didn't exist
     */
    static bool remove(const String& cachePath);

    /**
     * @brief Get cache file size in bytes
     *
     * @param cachePath Full path to cache file
     * @return File size in bytes, or 0 if file doesn't exist
     */
    static size_t getSize(const String& cachePath);

  private:
    // Cache file format constants (defined in config.h)
    static const uint32_t CACHE_MAGIC   = EVENT_CACHE_MAGIC;   ///< Magic number for file validation
    static const uint32_t CACHE_VERSION = EVENT_CACHE_VERSION; ///< Cache format version
    static const size_t MAX_EVENTS = EVENT_CACHE_MAX_EVENTS;   ///< Safety limit on events per cache

    /**
     * @brief Binary cache file header structure
     */
    struct __attribute__((packed)) CacheHeader {
        uint32_t magic;        ///< Magic number (0xCAFEEVNT)
        uint32_t version;      ///< Cache format version
        uint32_t eventCount;   ///< Number of events in cache
        time_t timestamp;      ///< Cache creation timestamp (Unix epoch)
        char calendarUrl[256]; ///< Calendar URL for validation
        uint32_t checksum;     ///< CRC32 of events data
    };

    /**
     * @brief Serialized event structure (fixed-size binary format)
     * Note: startTimeStr and endTimeStr removed in v2 (computed on-demand from timestamps)
     */
    struct __attribute__((packed)) SerializedEvent {
        char title[128];        ///< Event title/summary
        char location[64];      ///< Event location
        char date[16];          ///< Date in YYYY-MM-DD format
        time_t startTime;       ///< Start timestamp (Unix epoch)
        time_t endTime;         ///< End timestamp (Unix epoch)
        uint8_t flags;          ///< Bit flags: allDay, isToday, isTomorrow, isMultiDay
        uint8_t dayOfMonth;     ///< Day of month (1-31)
        char calendarName[32];  ///< Source calendar name
        char calendarColor[16]; ///< Calendar color code
        char summary[128];      ///< Alternative summary field
    };

    // Bit flags for SerializedEvent.flags
    static const uint8_t FLAG_ALL_DAY      = 0x01;
    static const uint8_t FLAG_IS_TODAY     = 0x02;
    static const uint8_t FLAG_IS_TOMORROW  = 0x04;
    static const uint8_t FLAG_IS_MULTI_DAY = 0x08;
    static const uint8_t FLAG_IS_HOLIDAY   = 0x10;

    /**
     * @brief Calculate CRC32 checksum of data
     *
     * @param data Pointer to data buffer
     * @param length Data length in bytes
     * @return CRC32 checksum value
     */
    static uint32_t calculateCRC32(const uint8_t* data, size_t length);

    /**
     * @brief Serialize a CalendarEvent to binary format
     *
     * @param event Source CalendarEvent
     * @param serialized Output SerializedEvent structure
     */
    static void serializeEvent(const CalendarEvent* event, SerializedEvent& serialized);

    /**
     * @brief Deserialize binary data to CalendarEvent
     *
     * @param serialized Input SerializedEvent structure
     * @return New CalendarEvent pointer (caller must delete)
     */
    static CalendarEvent* deserializeEvent(const SerializedEvent& serialized);

    /**
     * @brief Ensure cache directory exists
     *
     * Creates /cache directory if it doesn't exist.
     *
     * @return true if directory exists or was created
     */
    static bool ensureCacheDirectory();
};

#endif // EVENT_CACHE_H
