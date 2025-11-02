#include "calendar_wrapper.h"
#include "event_cache.h"
#include "debug_config.h"
#include "config.h"
#include <algorithm>

// CalendarWrapper implementation

CalendarWrapper::CalendarWrapper()
    : lastFetchTime(0), loaded(false), debug(false) {
}

CalendarWrapper::~CalendarWrapper() {
    clearCache();
}

void CalendarWrapper::clearCache() {
    for (auto event : cachedEvents) {
        if (event) {
            delete event;
        }
    }
    cachedEvents.clear();
}

String CalendarWrapper::getCacheFilename() const {
    // Generate a deterministic cache filename using hash of the URL only
    // This ensures the same URL always maps to the same cache file,
    // preventing duplicate caches for the same calendar URL

    unsigned long hash = 5381;
    for (unsigned int i = 0; i < config.url.length(); i++) {
        hash = ((hash << 5) + hash) + config.url[i]; // hash * 33 + c (djb2 algorithm)
    }

    // Use only the hash for the filename to ensure URL uniqueness
    // Format: /cache/events_{hash}.bin (binary event cache)
    String filename = "events_" + String(hash, HEX);

    return "/cache/" + filename + ".bin";
}

bool CalendarWrapper::isCacheValid() const {
    String cachePath = getCacheFilename();
    return EventCache::isValid(cachePath, EVENT_CACHE_VALIDITY_SECONDS);
}

bool CalendarWrapper::load(bool forceRefresh) {
    if (debug) {
        DEBUG_VERBOSE_PRINTLN("=== CalendarWrapper::load ===");
        DEBUG_VERBOSE_PRINTLN("Calendar: " + config.name);
        DEBUG_VERBOSE_PRINTLN("URL: " + config.url);
        DEBUG_VERBOSE_PRINTLN("Enabled: " + String(config.enabled ? "Yes" : "No"));
        DEBUG_VERBOSE_PRINTLN("Days to fetch: " + String(config.days_to_fetch));
        DEBUG_VERBOSE_PRINTLN("Force refresh: " + String(forceRefresh ? "Yes" : "No"));
    }

    // Clear previous events
    clearCache();
    loaded = false;
    isStale = false; // Reset stale flag

    // Check if calendar is enabled
    if (!config.enabled) {
        if (debug) DEBUG_VERBOSE_PRINTLN("Calendar is disabled, skipping");
        return true; // Not an error, just disabled
    }

    // Check if URL is configured
    if (config.url.isEmpty()) {
        if (debug) DEBUG_ERROR_PRINTLN("Error: No URL configured");
        lastError = "No URL configured";
        return false;
    }

    // Get binary cache path
    String cachePath = getCacheFilename();

    // Configure parser with calendar metadata
    parser.setCalendarName(config.name);
    parser.setDebug(debug);

    // Get date range for fetching events
    time_t now = time(nullptr);
    time_t endDate = now + (config.days_to_fetch * 86400); // days_to_fetch days from now

    if (debug) {
        DEBUG_INFO_PRINTLN("Fetching calendar from remote: " + config.url);
        DEBUG_INFO_PRINTLN("Date range: now to +" + String(config.days_to_fetch) + " days");
    }

    // Try fetching from remote with retries (cache only used as fallback)
    FilteredEvents* result = nullptr;
    int retryCount = 0;
    bool fetchSuccess = false;

    while (retryCount < CALENDAR_FETCH_MAX_RETRIES && !fetchSuccess) {
        if (retryCount > 0) {
            if (debug) DEBUG_INFO_PRINTLN("Retry attempt " + String(retryCount) + "/" + String(CALENDAR_FETCH_MAX_RETRIES));
            delay(CALENDAR_FETCH_RETRY_DELAY_MS);
        }

        // Stream parse directly from HTTP (no ICS file cache)
        result = parser.fetchEventsInRange(config.url, now, endDate, 500, "");

        if (result && result->success && !result->events.empty()) {
            fetchSuccess = true;
        } else {
            retryCount++;
            if (result && !result->error.isEmpty()) {
                lastError = result->error;
                if (debug) DEBUG_WARN_PRINTLN("Fetch failed: " + result->error);
            }
            if (result) {
                delete result;
                result = nullptr;
            }
        }
    }

    // Successfully fetched from remote
    if (fetchSuccess && result) {
        cachedEvents = std::move(result->events);
        result->events.clear(); // Prevent double deletion

        if (debug) {
            DEBUG_INFO_PRINTLN("Successfully fetched " + String(cachedEvents.size()) + " events from remote");
        }

        delete result;

        // Save to binary cache for future use
        if (EventCache::save(cachePath, cachedEvents, config.url)) {
            if (debug) DEBUG_INFO_PRINTLN("Saved events to binary cache");
        } else {
            if (debug) DEBUG_WARN_PRINTLN("Failed to save events to binary cache");
        }

        loaded = true;
        isStale = false;
        lastFetchTime = time(nullptr);
        return true;
    }

    // Remote fetch failed after all retries - try loading binary cache as fallback
    if (debug) {
        DEBUG_WARN_PRINTLN("Remote fetch failed after " + String(retryCount) + " attempts");
        DEBUG_INFO_PRINTLN("Attempting to load stale binary cache as fallback");
    }

    cachedEvents = EventCache::load(cachePath, config.url);

    if (!cachedEvents.empty()) {
        if (debug) {
            DEBUG_WARN_PRINTLN("Using stale cached data (" + String(cachedEvents.size()) + " events)");
        }
        loaded = true;
        isStale = true;
        lastError = "Using stale cached data - remote fetch failed after " + String(retryCount) + " retries";
        return true;
    }

    // Total failure - no remote data and no cache available
    if (debug) DEBUG_ERROR_PRINTLN("Failed to load from remote and no cache available");
    lastError = "Failed to fetch calendar after " + String(retryCount) + " retries and no cache available";
    loaded = false;
    return false;
}

std::vector<CalendarEvent*> CalendarWrapper::getEvents(time_t startDate, time_t endDate) {
    if (!loaded) {
        return std::vector<CalendarEvent*>();
    }

    std::vector<CalendarEvent*> result;

    // Filter cached events by date range
    for (auto event : cachedEvents) {
        if (event) {
            // Check if event is in range
            time_t eventStart = event->startTime;
            time_t eventEnd = event->endTime;

            if (eventEnd == 0) {
                eventEnd = eventStart;
            }

            // Debug: Track specific event
            bool isTargetEvent = (event->uid.indexOf("cor64p3165hjabb364qm2b9k68o3abb26gs3gbb5cco3gc1mcorm8p1o68") >= 0);

            if (isTargetEvent || debug) {
                struct tm* tmStart = localtime(&eventStart);
                struct tm* tmEnd = localtime(&eventEnd);
                struct tm* tmQueryStart = localtime(&startDate);
                struct tm* tmQueryEnd = localtime(&endDate);
                char startStr[32], endStr[32], qStartStr[32], qEndStr[32];
                strftime(startStr, sizeof(startStr), "%Y-%m-%d %H:%M:%S", tmStart);
                strftime(endStr, sizeof(endStr), "%Y-%m-%d %H:%M:%S", tmEnd);
                strftime(qStartStr, sizeof(qStartStr), "%Y-%m-%d %H:%M:%S", tmQueryStart);
                strftime(qEndStr, sizeof(qEndStr), "%Y-%m-%d %H:%M:%S", tmQueryEnd);

                DEBUG_INFO_PRINTF("  [%s] Event: %s\n", config.name.c_str(), event->summary.c_str());
                if (isTargetEvent) {
                    DEBUG_INFO_PRINTF("  >>> TARGET EVENT (Cambio gomme) FOUND <<<\n");
                }
                DEBUG_INFO_PRINTF("    Event Start: %s (unix: %ld)\n", startStr, eventStart);
                DEBUG_INFO_PRINTF("    Event End: %s (unix: %ld)\n", endStr, eventEnd);
                DEBUG_INFO_PRINTF("    Query Range: %s to %s\n", qStartStr, qEndStr);
                DEBUG_INFO_PRINTF("    Query Unix: %ld to %ld\n", startDate, endDate);
                DEBUG_INFO_PRINTF("    Filter check: (%ld <= %ld) && (%ld >= %ld) = %s\n",
                    eventStart, endDate, eventEnd, startDate,
                    ((eventStart <= endDate) && (eventEnd >= startDate)) ? "PASS" : "FAIL");
            }

            // Check if event overlaps with the date range
            if ((eventStart <= endDate) && (eventEnd >= startDate)) {
                // Add calendar metadata to each event
                event->calendarName = config.name;
                event->calendarColor = config.color;
                // Mark as holiday if it's from a holiday calendar and is all-day
                event->isHoliday = (config.holiday_calendar && event->allDay);
                result.push_back(event);
                if (isTargetEvent) {
                    DEBUG_INFO_PRINTF("    >>> PASSED FILTER - ADDED TO RESULTS <<<\n");
                }
            } else {
                if (isTargetEvent) {
                    DEBUG_INFO_PRINTF("    >>> FILTERED OUT - NOT ADDED <<<\n");
                }
            }
        }
    }

    return result;
}

std::vector<CalendarEvent*> CalendarWrapper::getAllEvents() {
    if (!loaded) {
        return std::vector<CalendarEvent*>();
    }

    // Return all cached events with metadata
    for (auto event : cachedEvents) {
        if (event) {
            event->calendarName = config.name;
            event->calendarColor = config.color;
            // Mark as holiday if it's from a holiday calendar and is all-day
            event->isHoliday = (config.holiday_calendar && event->allDay);
        }
    }

    return cachedEvents;
}

size_t CalendarWrapper::getEventCountInRange(time_t startDate, time_t endDate) const {
    if (!loaded) {
        return 0;
    }

    size_t count = 0;

    // Count cached events in range
    for (auto event : cachedEvents) {
        if (event) {
            time_t eventStart = event->startTime;
            time_t eventEnd = event->endTime;

            if (eventEnd == 0) {
                eventEnd = eventStart;
            }

            // Check if event overlaps with the date range
            if ((eventStart <= endDate) && (eventEnd >= startDate)) {
                count++;
            }
        }
    }

    return count;
}

// CalendarManager implementation

CalendarManager::CalendarManager() : debug(false) {
}

CalendarManager::~CalendarManager() {
    clear();
}

void CalendarManager::clear() {
    for (auto cal : calendars) {
        delete cal;
    }
    calendars.clear();
}

bool CalendarManager::loadFromConfig(const RuntimeConfig& config) {
    if (debug) {
        DEBUG_INFO_PRINTLN("=== CalendarManager::loadFromConfig ===");
        DEBUG_INFO_PRINTLN("Number of calendars: " + String(config.calendars.size()));
    }

    // Clear existing calendars
    clear();

    // Create wrapper for each calendar in config
    for (const auto& calConfig : config.calendars) {
        CalendarWrapper* wrapper = new CalendarWrapper();
        wrapper->setConfig(calConfig);
        wrapper->setDebug(debug);
        calendars.push_back(wrapper);

        if (debug) {
            DEBUG_INFO_PRINTLN("Added calendar: " + calConfig.name);
            DEBUG_INFO_PRINTLN("  URL: " + calConfig.url);
            DEBUG_INFO_PRINTLN("  Enabled: " + String(calConfig.enabled ? "Yes" : "No"));
            DEBUG_INFO_PRINTLN("  Days to fetch: " + String(calConfig.days_to_fetch));
            DEBUG_INFO_PRINTLN("  Color: " + calConfig.color);
        }
    }

    return true;
}

bool CalendarManager::loadAll(bool forceRefresh) {
    if (debug) {
        DEBUG_INFO_PRINTLN("=== CalendarManager::loadAll ===");
        DEBUG_INFO_PRINTLN("Loading " + String(calendars.size()) + " calendars");
    }

    bool allSuccess = true;
    int loadedCount = 0;
    int errorCount = 0;

    for (size_t i = 0; i < calendars.size(); i++) {
        CalendarWrapper* cal = calendars[i];

        if (debug) {
            DEBUG_INFO_PRINTLN("\nLoading calendar " + String(i+1) + "/" + String(calendars.size()));
        }

        if (cal->load(forceRefresh)) {
            if (cal->isLoaded()) {
                loadedCount++;
                if (debug) {
                    DEBUG_INFO_PRINTLN("✓ Loaded: " + cal->getName());
                    DEBUG_INFO_PRINTLN("  Events: " + String(cal->getEventCount()));
                }
            }
        } else {
            errorCount++;
            allSuccess = false;
            if (debug) {
                DEBUG_INFO_PRINTLN("✗ Failed: " + cal->getName());
                DEBUG_INFO_PRINTLN("  Error: " + cal->getLastError());
            }
        }
    }

    if (debug) {
        DEBUG_INFO_PRINTLN("\n=== Load Summary ===");
        DEBUG_INFO_PRINTLN("Loaded: " + String(loadedCount) + " calendars");
        DEBUG_INFO_PRINTLN("Errors: " + String(errorCount) + " calendars");
        DEBUG_INFO_PRINTLN("Total events: " + String(getTotalEventCount()));

        // Show events in range for each calendar
        time_t now = time(nullptr);
        for (auto cal : calendars) {
            if (cal->isLoaded()) {
                time_t endDate = now + (cal->getDaysToFetch() * 86400);
                size_t eventsInRange = cal->getEventCountInRange(now, endDate);
                DEBUG_INFO_PRINTLN("  " + cal->getName() + ": " + String(eventsInRange) + " events in next " + String(cal->getDaysToFetch()) + " days");
            }
        }
    }

    return allSuccess;
}

std::vector<CalendarEvent*> CalendarManager::getAllEvents(time_t startDate, time_t endDate) {
    std::vector<CalendarEvent*> allEvents;

    // Collect events from all enabled calendars
    for (auto cal : calendars) {
        if (cal->isEnabled() && cal->isLoaded()) {
            std::vector<CalendarEvent*> calEvents = cal->getEvents(startDate, endDate);
            allEvents.insert(allEvents.end(), calEvents.begin(), calEvents.end());
        }
    }

    // Sort all events by start time
    std::sort(allEvents.begin(), allEvents.end(), [](CalendarEvent* a, CalendarEvent* b) {
        return a->startTime < b->startTime;
    });

    if (debug) {
        DEBUG_INFO_PRINTLN("Merged events from all calendars: " + String(allEvents.size()) + " events");
    }

    return allEvents;
}

CalendarWrapper* CalendarManager::getCalendar(size_t index) {
    if (index >= calendars.size()) {
        return nullptr;
    }
    return calendars[index];
}

size_t CalendarManager::getTotalEventCount() const {
    size_t total = 0;
    for (auto cal : calendars) {
        if (cal->isLoaded()) {
            total += cal->getEventCount();
        }
    }
    return total;
}

void CalendarManager::printStatus() const {
    DEBUG_INFO_PRINTLN("\n=== Calendar Manager Status ===");
    DEBUG_INFO_PRINTLN("Total calendars: " + String(calendars.size()));

    // Calculate the date range we're looking at
    time_t now = time(nullptr);
    time_t startDate = now;

    int enabledCount = 0;
    int loadedCount = 0;
    size_t totalEventsInRange = 0;

    for (size_t i = 0; i < calendars.size(); i++) {
        CalendarWrapper* cal = calendars[i];

        if (cal->isEnabled()) enabledCount++;
        if (cal->isLoaded()) loadedCount++;

        DEBUG_INFO_PRINTLN("\nCalendar " + String(i+1) + ": " + cal->getName());
        DEBUG_INFO_PRINTLN("  Enabled: " + String(cal->isEnabled() ? "Yes" : "No"));
        DEBUG_INFO_PRINTLN("  Loaded: " + String(cal->isLoaded() ? "Yes" : "No"));

        if (cal->isLoaded()) {
            // Calculate events in range for this calendar
            time_t calEndDate = now + (cal->getDaysToFetch() * 86400);
            size_t eventsInRange = cal->getEventCountInRange(startDate, calEndDate);
            totalEventsInRange += eventsInRange;

            DEBUG_INFO_PRINTLN("  Total events: " + String(cal->getEventCount()));
            DEBUG_INFO_PRINTLN("  Events in range: " + String(eventsInRange) + " (next " + String(cal->getDaysToFetch()) + " days)");

            // List events in range
            if (eventsInRange > 0) {
                std::vector<CalendarEvent*> events = cal->getEvents(startDate, calEndDate);
                DEBUG_INFO_PRINTLN("  Events found:");
                for (size_t j = 0; j < events.size(); j++) {
                    if (events[j]) {
                        // Format the event date/time nicely
                        struct tm* tm = localtime(&events[j]->startTime);
                        char dateStr[32];
                        strftime(dateStr, sizeof(dateStr), "%b %d, %H:%M", tm);

                        DEBUG_INFO_PRINTLN("    " + String(j+1) + ". " + events[j]->summary +
                                         " - " + String(dateStr) +
                                         (events[j]->allDay ? " (all day)" : ""));
                    }
                }
            }
        }

        DEBUG_INFO_PRINTLN("  Days to fetch: " + String(cal->getDaysToFetch()));
        DEBUG_INFO_PRINTLN("  Color: " + cal->getColor());
    }

    DEBUG_INFO_PRINTLN("\nSummary:");
    DEBUG_INFO_PRINTLN("  Enabled: " + String(enabledCount) + "/" + String(calendars.size()));
    DEBUG_INFO_PRINTLN("  Loaded: " + String(loadedCount) + "/" + String(calendars.size()));
    DEBUG_INFO_PRINTLN("  Total events: " + String(getTotalEventCount()));
    DEBUG_INFO_PRINTLN("  Total events in range: " + String(totalEventsInRange));
}

bool CalendarManager::isAnyCalendarStale() const {
    for (const auto cal : calendars) {
        if (cal->isLoaded() && cal->isStale) {
            return true;
        }
    }
    return false;
}