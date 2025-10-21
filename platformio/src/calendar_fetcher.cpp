#include "calendar_fetcher.h"
#include <WiFi.h>

CalendarFetcher::CalendarFetcher() : timeout(30000), debug(false) {
    // Initialize HTTP client settings
    http.setReuse(false);
    http.setTimeout(timeout);
}

CalendarFetcher::~CalendarFetcher() {
    if (http.connected()) {
        http.end();
    }
}

bool CalendarFetcher::isLocalUrl(const String& url) const {
    return url.startsWith("local://") ||
           url.startsWith("file://") ||
           url.startsWith("/");
}

String CalendarFetcher::getLocalPath(const String& url) const {
    if (url.startsWith("local://")) {
        return url.substring(8); // Remove "local://"
    } else if (url.startsWith("file://")) {
        return url.substring(7); // Remove "file://"
    }
    return url; // Already a path
}

FetchResult CalendarFetcher::fetch(const String& url) {
    if (debug) {
        Serial.println("=== Calendar Fetcher ===");
        Serial.println("Fetching from: " + url);
    }

    if (isLocalUrl(url)) {
        String path = getLocalPath(url);
        return fetchFromFile(path);
    } else {
        return fetchFromHttp(url);
    }
}

FetchResult CalendarFetcher::fetchFromHttp(const String& url) {
    FetchResult result;

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        result.error = "WiFi not connected";
        if (debug) Serial.println("Error: " + result.error);
        return result;
    }

    // Start HTTP request
    if (debug) Serial.println("Starting HTTP request...");

    http.begin(url);
    http.setTimeout(timeout);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Add headers
    http.addHeader("User-Agent", "ESP32-Calendar/1.0");
    http.addHeader("Accept", "text/calendar");

    // Perform GET request
    int httpCode = http.GET();
    result.httpCode = httpCode;

    if (httpCode > 0) {
        if (debug) Serial.printf("HTTP response code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            // Get the response
            result.data = http.getString();
            result.dataSize = result.data.length();
            result.success = true;

            if (debug) {
                Serial.printf("Received %d bytes\n", result.dataSize);

                // Show first 200 characters for debugging
                if (result.dataSize > 0) {
                    Serial.println("Content preview:");
                    Serial.println(result.data.substring(0, min(200, (int)result.dataSize)));
                }
            }
        } else {
            result.error = "HTTP error: " + String(httpCode);
            if (debug) Serial.println(result.error);
        }
    } else {
        result.error = "HTTP request failed: " + http.errorToString(httpCode);
        if (debug) Serial.println(result.error);
    }

    http.end();
    return result;
}

FetchResult CalendarFetcher::fetchFromFile(const String& path) {
    FetchResult result;

    // Initialize LittleFS if needed
    if (!LittleFS.begin(false)) {
        result.error = "Failed to mount LittleFS";
        if (debug) Serial.println("Error: " + result.error);
        return result;
    }

    // Check if file exists
    if (!LittleFS.exists(path)) {
        result.error = "File not found: " + path;
        if (debug) Serial.println("Error: " + result.error);
        return result;
    }

    // Open the file
    File file = LittleFS.open(path, "r");
    if (!file) {
        result.error = "Failed to open file: " + path;
        if (debug) Serial.println("Error: " + result.error);
        return result;
    }

    // Read file content
    result.dataSize = file.size();
    if (debug) Serial.printf("Reading local file: %s (%d bytes)\n", path.c_str(), result.dataSize);

    if (result.dataSize > 0) {
        result.data = file.readString();
        result.success = true;

        if (debug) {
            Serial.println("File loaded successfully");
            // Show first 200 characters for debugging
            Serial.println("Content preview:");
            Serial.println(result.data.substring(0, min(200, (int)result.dataSize)));
        }
    } else {
        result.error = "File is empty";
        if (debug) Serial.println("Error: " + result.error);
    }

    file.close();
    return result;
}

String CalendarFetcher::getFilenameFromUrl(const String& url) {
    // Extract a filename from URL for caching
    String filename = url;

    // Remove protocol
    int protocolEnd = filename.indexOf("://");
    if (protocolEnd != -1) {
        filename = filename.substring(protocolEnd + 3);
    }

    // Replace special characters
    filename.replace("/", "_");
    filename.replace(":", "_");
    filename.replace("?", "_");
    filename.replace("&", "_");
    filename.replace("=", "_");

    // Truncate if too long
    if (filename.length() > 64) {
        filename = filename.substring(0, 60) + ".ics";
    } else if (!filename.endsWith(".ics")) {
        filename += ".ics";
    }

    return "/cache/" + filename;
}

bool CalendarFetcher::cacheToFile(const String& data, const String& filename) {
    // Ensure cache directory exists
    if (!LittleFS.exists("/cache")) {
        LittleFS.mkdir("/cache");
    }

    File file = LittleFS.open(filename, "w");
    if (!file) {
        return false;
    }

    size_t written = file.print(data);
    file.close();

    return written == data.length();
}

String CalendarFetcher::loadFromCache(const String& filename) {
    if (!LittleFS.exists(filename)) {
        return "";
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        return "";
    }

    String data = file.readString();
    file.close();

    return data;
}

bool CalendarFetcher::isCacheValid(const String& filename, unsigned long maxAgeSeconds) {
    if (!LittleFS.exists(filename)) {
        return false;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        return false;
    }

    // Get file modification time
    time_t modTime = file.getLastWrite();
    file.close();

    // Check if cache is still valid
    time_t now;
    time(&now);

    return (now - modTime) < maxAgeSeconds;
}