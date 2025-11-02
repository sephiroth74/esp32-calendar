#ifndef CALENDAR_WRAPPER_H
#define CALENDAR_WRAPPER_H

#include <Arduino.h>
#include "calendar_stream_parser.h"
#include "littlefs_config.h"
#include <vector>

/**
 * @brief Wrapper class that combines stream parser with calendar configuration
 *
 * CalendarWrapper manages a single calendar source with its configuration properties.
 * It handles loading calendar data from remote URLs or local files, caching events,
 * and providing filtered access to events within date ranges.
 *
 * Key features:
 * - Binary event caching to LittleFS filesystem
 * - Cache filename based on URL hash to prevent duplicates
 * - Supports both remote HTTP(S) URLs and local file:// paths
 * - Stream parsing (no ICS file storage - handles unlimited file sizes)
 * - Configurable days_to_fetch per calendar
 * - Event filtering by date range
 * - Graceful degradation (uses stale cache if network fails)
 * - Stale data detection (cache vs fresh data)
 */
class CalendarWrapper {
private:
    CalendarStreamParser parser;                   ///< Stream parser for efficient ICS parsing
    CalendarConfig config;                         ///< Calendar configuration (URL, name, color, etc.)
    std::vector<CalendarEvent*> cachedEvents;      ///< In-memory cache of parsed events

    String cachedFilename;                         ///< LittleFS binary cache filename for this calendar
    String lastError;                              ///< Last error message if loading failed
    time_t lastFetchTime;                          ///< Unix timestamp of last successful fetch
    bool loaded;                                   ///< True if calendar has been loaded
    bool debug;                                    ///< Enable debug output

    /**
     * @brief Calculate binary cache filename for this calendar based on URL hash
     *
     * Uses djb2 hash algorithm on the calendar URL to generate a unique,
     * deterministic filename. This ensures the same URL always maps to
     * the same cache file, preventing duplicate caches.
     *
     * @return Binary cache filename in format "/cache/events_{hash}.bin"
     */
    String getCacheFilename() const;

    /**
     * @brief Check if binary cache is still valid
     *
     * Cache is considered valid if it exists, passes integrity checks,
     * and is less than 24 hours old.
     *
     * @return true if binary cache is valid and can be used
     */
    bool isCacheValid() const;

public:
    /**
     * @brief Construct a new Calendar Wrapper object
     */
    CalendarWrapper();

    /**
     * @brief Destroy the Calendar Wrapper object and free all cached events
     */
    ~CalendarWrapper();

    bool isStale = false;       ///< True if data was loaded from cache instead of fresh fetch

    // Configuration
    /**
     * @brief Set calendar configuration
     * @param calConfig Calendar configuration containing URL, name, color, etc.
     */
    void setConfig(const CalendarConfig& calConfig) { config = calConfig; }

    /**
     * @brief Enable or disable debug output
     * @param enable true to enable debug logging
     */
    void setDebug(bool enable) { debug = enable; parser.setDebug(enable); }

    // Get configuration properties
    /** @brief Get calendar name */
    const String& getName() const { return config.name; }
    /** @brief Get calendar URL (HTTP(S) or local file path) */
    const String& getUrl() const { return config.url; }
    /** @brief Get calendar color code (for display) */
    const String& getColor() const { return config.color; }
    /** @brief Check if calendar is enabled */
    bool isEnabled() const { return config.enabled; }
    /** @brief Get number of days to fetch events for */
    int getDaysToFetch() const { return config.days_to_fetch; }

    /**
     * @brief Load calendar data from network with binary event caching
     *
     * Loading strategy:
     * 1. If !forceRefresh and binary cache valid: Load from binary cache (fast)
     * 2. Stream parse from HTTP (no ICS file cache - handles any file size)
     * 3. Save parsed events to binary cache for future use
     * 4. If network fails: Fall back to stale binary cache (graceful degradation)
     *
     * This approach:
     * - Handles unlimited ICS file sizes via stream parsing
     * - Uses compact binary cache (~200 bytes per event)
     * - Provides offline fallback when network fails
     * - Sets isStale flag to indicate cache vs fresh data
     *
     * @param forceRefresh If true, bypass cache and fetch fresh data
     * @return true if calendar was successfully loaded (fresh or stale)
     */
    bool load(bool forceRefresh = false);

    /**
     * @brief Get events within specific date range
     *
     * Filters cached events to only return those within the specified date range.
     * Respects the calendar's days_to_fetch configuration.
     *
     * @param startDate Start of date range (Unix timestamp)
     * @param endDate End of date range (Unix timestamp)
     * @return Vector of CalendarEvent pointers (do not delete - managed by wrapper)
     */
    std::vector<CalendarEvent*> getEvents(time_t startDate, time_t endDate);

    /**
     * @brief Get all events for the configured days_to_fetch period
     * @return Vector of all cached CalendarEvent pointers
     */
    std::vector<CalendarEvent*> getAllEvents();

    // Status
    /** @brief Check if calendar has been successfully loaded */
    bool isLoaded() const { return loaded; }
    /** @brief Get total number of cached events */
    size_t getEventCount() const { return cachedEvents.size(); }
    /**
     * @brief Get count of events within specified date range
     * @param startDate Start of date range (Unix timestamp)
     * @param endDate End of date range (Unix timestamp)
     * @return Number of events in range
     */
    size_t getEventCountInRange(time_t startDate, time_t endDate) const;
    /** @brief Get last error message if loading failed */
    String getLastError() const { return lastError; }

    /**
     * @brief Clear cached events from memory
     *
     * Frees all event objects and clears the in-memory cache.
     * Does not affect LittleFS binary cache files.
     */
    void clearCache();
};

/**
 * @brief Manager for multiple calendar sources
 *
 * CalendarManager coordinates multiple CalendarWrapper instances, handles
 * loading all enabled calendars, and provides merged event access across
 * all calendars. Supports up to MAX_CALENDARS (default 3) calendar sources.
 *
 * Features:
 * - Automatic calendar loading from RuntimeConfig
 * - Parallel loading of all enabled calendars
 * - Event merging and sorting across all calendars
 * - Per-calendar stale data tracking
 * - Status reporting and debugging
 */
class CalendarManager {
private:
    std::vector<CalendarWrapper*> calendars;  ///< Collection of calendar wrappers
    bool debug;                               ///< Enable debug output

public:
    /**
     * @brief Construct a new Calendar Manager object
     */
    CalendarManager();

    /**
     * @brief Destroy the Calendar Manager and free all calendars
     */
    ~CalendarManager();

    /**
     * @brief Enable or disable debug output for all calendars
     * @param enable true to enable debug logging
     */
    void setDebug(bool enable) { debug = enable; }

    /**
     * @brief Load calendar configurations from RuntimeConfig
     *
     * Clears existing calendars and creates new CalendarWrapper instances
     * for each calendar in the configuration. Respects MAX_CALENDARS limit.
     *
     * @param config Runtime configuration containing calendar array
     * @return true if calendars were successfully configured
     */
    bool loadFromConfig(const RuntimeConfig& config);

    /**
     * @brief Clear all calendars and free memory
     */
    void clear();

    /**
     * @brief Load data for all enabled calendars
     *
     * Iterates through all calendars and loads their event data.
     * Each calendar loads independently - failures are tracked per calendar.
     *
     * @param forceRefresh If true, bypass cache and fetch fresh data for all calendars
     * @return true if at least one calendar loaded successfully
     */
    bool loadAll(bool forceRefresh = false);

    /**
     * @brief Get merged events from all enabled calendars within date range
     *
     * Collects events from all enabled calendars, merges them, and sorts
     * chronologically. Each event retains its calendar source metadata
     * (name, color) for display purposes.
     *
     * @param startDate Start of date range (Unix timestamp)
     * @param endDate End of date range (Unix timestamp)
     * @return Vector of merged CalendarEvent pointers (do not delete - managed by wrappers)
     */
    std::vector<CalendarEvent*> getAllEvents(time_t startDate, time_t endDate);

    /**
     * @brief Get specific calendar by index
     * @param index Calendar index (0 to getCalendarCount()-1)
     * @return Pointer to CalendarWrapper or nullptr if index invalid
     */
    CalendarWrapper* getCalendar(size_t index);

    /**
     * @brief Get total number of configured calendars
     * @return Number of calendars (enabled and disabled)
     */
    size_t getCalendarCount() const { return calendars.size(); }

    // Status
    /**
     * @brief Get total event count across all calendars
     * @return Sum of events from all enabled calendars
     */
    size_t getTotalEventCount() const;

    /**
     * @brief Print status of all calendars to debug output
     *
     * Shows calendar name, enabled status, event count, and stale flag
     * for each configured calendar. Useful for debugging.
     */
    void printStatus() const;

    /**
     * @brief Check if any calendar is using stale/cached data
     * @return true if at least one calendar has isStale=true
     */
    bool isAnyCalendarStale() const;
};

#endif // CALENDAR_WRAPPER_H