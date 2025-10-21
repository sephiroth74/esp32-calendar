#include "calendar_wrapper.h"
#include <algorithm>

// CalendarWrapper implementation

CalendarWrapper::CalendarWrapper()
    : lastFetchTime(0), loaded(false), debug(false) {
}

CalendarWrapper::~CalendarWrapper() {
    // Parser will clean up its own events
}

String CalendarWrapper::getCacheFilename() const {
    // Use the fetcher's utility to get a cache filename
    return CalendarFetcher::getFilenameFromUrl(config.url);
}

bool CalendarWrapper::isCacheValid() const {
    // Cache is valid for 1 hour (3600 seconds)
    const unsigned long CACHE_DURATION = 3600;

    // Check if cache file exists and is recent
    String cacheFile = getCacheFilename();
    return CalendarFetcher::isCacheValid(cacheFile, CACHE_DURATION);
}

bool CalendarWrapper::load(bool forceRefresh) {
    if (debug) {
        Serial.println("=== CalendarWrapper::load ===");
        Serial.println("Calendar: " + config.name);
        Serial.println("URL: " + config.url);
        Serial.println("Enabled: " + String(config.enabled ? "Yes" : "No"));
        Serial.println("Days to fetch: " + String(config.days_to_fetch));
    }

    // Check if calendar is enabled
    if (!config.enabled) {
        if (debug) Serial.println("Calendar is disabled, skipping");
        loaded = false;
        return true; // Not an error, just disabled
    }

    // Check if URL is configured
    if (config.url.isEmpty()) {
        if (debug) Serial.println("Error: No URL configured");
        loaded = false;
        return false;
    }

    String icsData;

    // Check cache first (unless force refresh)
    if (!forceRefresh && isCacheValid()) {
        String cacheFile = getCacheFilename();
        icsData = CalendarFetcher::loadFromCache(cacheFile);

        if (!icsData.isEmpty()) {
            if (debug) Serial.println("Loaded from cache: " + cacheFile);
        }
    }

    // If no cached data, fetch from source
    if (icsData.isEmpty()) {
        if (debug) Serial.println("Fetching from source...");

        FetchResult result = fetcher.fetch(config.url);

        if (!result.success) {
            if (debug) Serial.println("Fetch failed: " + result.error);
            loaded = false;
            return false;
        }

        icsData = result.data;

        // Cache the data if it's from a remote source
        if (!config.url.startsWith("local://") &&
            !config.url.startsWith("file://") &&
            !config.url.startsWith("/")) {
            String cacheFile = getCacheFilename();
            if (CalendarFetcher::cacheToFile(icsData, cacheFile)) {
                if (debug) Serial.println("Cached to: " + cacheFile);
            }
        }
    }

    // Parse the ICS data
    if (debug) Serial.println("Parsing ICS data...");

    if (!parser.loadFromString(icsData)) {
        if (debug) Serial.println("Parse failed: " + parser.getLastError());
        loaded = false;
        return false;
    }

    loaded = true;
    lastFetchTime = time(nullptr);

    if (debug) {
        Serial.println("Successfully loaded calendar");
        Serial.println("Events found: " + String(parser.getEventCount()));
        Serial.println("Calendar name: " + parser.getCalendarName());
    }

    return true;
}

std::vector<CalendarEvent*> CalendarWrapper::getEvents(time_t startDate, time_t endDate) {
    if (!loaded) {
        return std::vector<CalendarEvent*>();
    }

    // Apply days_to_fetch limit
    time_t now = time(nullptr);
    time_t maxEndDate = now + (config.days_to_fetch * 86400); // days to seconds

    // Use the earlier of the requested end date or the days_to_fetch limit
    if (endDate > maxEndDate) {
        endDate = maxEndDate;
    }

    // Get events from parser
    std::vector<CalendarEvent*> events = parser.getEventsInRange(startDate, endDate);

    // Add calendar metadata to each event
    for (auto event : events) {
        if (event) {
            // Store calendar name and color in the event
            // This helps the display manager show which calendar an event belongs to
            event->calendarName = config.name;
            event->calendarColor = config.color;
        }
    }

    return events;
}

std::vector<CalendarEvent*> CalendarWrapper::getAllEvents() {
    if (!loaded) {
        return std::vector<CalendarEvent*>();
    }

    // Get events for the configured days_to_fetch period
    time_t now = time(nullptr);
    time_t endDate = now + (config.days_to_fetch * 86400);

    return getEvents(now, endDate);
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
        Serial.println("=== CalendarManager::loadFromConfig ===");
        Serial.println("Number of calendars: " + String(config.calendars.size()));
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
            Serial.println("Added calendar: " + calConfig.name);
            Serial.println("  URL: " + calConfig.url);
            Serial.println("  Enabled: " + String(calConfig.enabled ? "Yes" : "No"));
            Serial.println("  Days to fetch: " + String(calConfig.days_to_fetch));
            Serial.println("  Color: " + calConfig.color);
        }
    }

    return true;
}

bool CalendarManager::loadAll(bool forceRefresh) {
    if (debug) {
        Serial.println("=== CalendarManager::loadAll ===");
        Serial.println("Loading " + String(calendars.size()) + " calendars");
    }

    bool allSuccess = true;
    int loadedCount = 0;
    int errorCount = 0;

    for (size_t i = 0; i < calendars.size(); i++) {
        CalendarWrapper* cal = calendars[i];

        if (debug) {
            Serial.println("\nLoading calendar " + String(i+1) + "/" + String(calendars.size()));
        }

        if (cal->load(forceRefresh)) {
            if (cal->isLoaded()) {
                loadedCount++;
                if (debug) {
                    Serial.println("✓ Loaded: " + cal->getName());
                    Serial.println("  Events: " + String(cal->getEventCount()));
                }
            }
        } else {
            errorCount++;
            allSuccess = false;
            if (debug) {
                Serial.println("✗ Failed: " + cal->getName());
                Serial.println("  Error: " + cal->getLastError());
            }
        }
    }

    if (debug) {
        Serial.println("\n=== Load Summary ===");
        Serial.println("Loaded: " + String(loadedCount) + " calendars");
        Serial.println("Errors: " + String(errorCount) + " calendars");
        Serial.println("Total events: " + String(getTotalEventCount()));
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
        Serial.println("Merged events from all calendars: " + String(allEvents.size()) + " events");
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
    Serial.println("\n=== Calendar Manager Status ===");
    Serial.println("Total calendars: " + String(calendars.size()));

    int enabledCount = 0;
    int loadedCount = 0;

    for (size_t i = 0; i < calendars.size(); i++) {
        CalendarWrapper* cal = calendars[i];

        if (cal->isEnabled()) enabledCount++;
        if (cal->isLoaded()) loadedCount++;

        Serial.println("\nCalendar " + String(i+1) + ": " + cal->getName());
        Serial.println("  Enabled: " + String(cal->isEnabled() ? "Yes" : "No"));
        Serial.println("  Loaded: " + String(cal->isLoaded() ? "Yes" : "No"));

        if (cal->isLoaded()) {
            Serial.println("  Events: " + String(cal->getEventCount()));
        }

        Serial.println("  Days to fetch: " + String(cal->getDaysToFetch()));
        Serial.println("  Color: " + cal->getColor());
    }

    Serial.println("\nSummary:");
    Serial.println("  Enabled: " + String(enabledCount) + "/" + String(calendars.size()));
    Serial.println("  Loaded: " + String(loadedCount) + "/" + String(calendars.size()));
    Serial.println("  Total events: " + String(getTotalEventCount()));
}