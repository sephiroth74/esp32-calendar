#include "calendar_fetcher.h"
#include "debug_config.h"
#include <WiFi.h>

CalendarFetcher::CalendarFetcher() : streamClient(nullptr), timeout(30000), debug(false) {
    // Initialize HTTP client settings
    http.setReuse(false);
    http.setTimeout(timeout);
}

CalendarFetcher::~CalendarFetcher() {
    endStream();
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
    DEBUG_INFO_PRINTLN("=== Calendar Fetcher ===");
    DEBUG_INFO_PRINTLN("Fetching from: " + url);

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
        DEBUG_ERROR_PRINTLN("Error: " + result.error);
        return result;
    }

    // Start HTTP request
    DEBUG_INFO_PRINTLN("Starting HTTP request...");

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
                    DEBUG_VERBOSE_PRINTLN("Content preview:");
                    DEBUG_VERBOSE_PRINTLN(result.data.substring(0, min(200, (int)result.dataSize)));
                }
            }
        } else {
            result.error = "HTTP error: " + String(httpCode);
            DEBUG_ERROR_PRINTLN(result.error);
        }
    } else {
        result.error = "HTTP request failed: " + http.errorToString(httpCode);
        DEBUG_ERROR_PRINTLN(result.error);
    }

    http.end();
    return result;
}

FetchResult CalendarFetcher::fetchFromFile(const String& path) {
    FetchResult result;

    // Initialize LittleFS if needed
    if (!LittleFS.begin(false)) {
        result.error = "Failed to mount LittleFS";
        DEBUG_ERROR_PRINTLN("Error: " + result.error);
        return result;
    }

    // Check if file exists
    if (!LittleFS.exists(path)) {
        result.error = "File not found: " + path;
        DEBUG_ERROR_PRINTLN("Error: " + result.error);
        return result;
    }

    // Open the file
    File file = LittleFS.open(path, "r");
    if (!file) {
        result.error = "Failed to open file: " + path;
        DEBUG_ERROR_PRINTLN("Error: " + result.error);
        return result;
    }

    // Read file content
    result.dataSize = file.size();
    DEBUG_INFO_PRINTF("Reading local file: %s (%d bytes)\n", path.c_str(), result.dataSize);

    if (result.dataSize > 0) {
        result.data = file.readString();
        result.success = true;

        DEBUG_INFO_PRINTLN("File loaded successfully");
        // Show first 200 characters for debugging
        DEBUG_VERBOSE_PRINTLN("Content preview:");
        DEBUG_VERBOSE_PRINTLN(result.data.substring(0, min(200, (int)result.dataSize)));
    } else {
        result.error = "File is empty";
        DEBUG_ERROR_PRINTLN("Error: " + result.error);
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

Stream* CalendarFetcher::fetchStream(const String& url) {
    endStream(); // Clean up any existing stream

    DEBUG_INFO_PRINTLN("=== Calendar Fetcher (Stream) ===");
    DEBUG_INFO_PRINTLN("Fetching stream from: " + url);

    if (isLocalUrl(url)) {
        String path = getLocalPath(url);

        // Initialize LittleFS if needed
        if (!LittleFS.begin(false)) {
            DEBUG_ERROR_PRINTLN("Error: Failed to mount LittleFS");
            return nullptr;
        }

        // Check if file exists
        if (!LittleFS.exists(path)) {
            DEBUG_ERROR_PRINTLN("Error: File not found: " + path);
            return nullptr;
        }

        // Open the file
        fileStream = LittleFS.open(path, "r");
        if (!fileStream) {
            DEBUG_ERROR_PRINTLN("Error: Failed to open file: " + path);
            return nullptr;
        }

        DEBUG_INFO_PRINTLN("Opened local file stream: " + path + " (" + String(fileStream.size()) + " bytes)");
        return &fileStream;

    } else {
        // Check WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_ERROR_PRINTLN("Error: WiFi not connected");
            return nullptr;
        }

        int maxRetries = 3;
        int retries = maxRetries;
        while (retries > 0) {
            int attemptNum = maxRetries - retries + 1;

            // Start HTTP request directly (without explicit WiFiClient)
            DEBUG_INFO_PRINTF(">>> HTTP Fetch Attempt %d/%d for calendar\n", attemptNum, maxRetries);
            DEBUG_INFO_PRINTLN("Starting HTTP stream request...");

            if (!http.begin(url)) {
                DEBUG_ERROR_PRINTLN(">>> Attempt " + String(attemptNum) + " FAILED: Could not begin HTTP request");
                retries--;
                if (retries > 0) {
                    DEBUG_INFO_PRINTLN(">>> Retrying in 5 seconds...");
                    delay(5000);
                }
                continue;
            }

            http.setTimeout(timeout);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

            // Add headers
            http.addHeader("User-Agent", "ESP32-Calendar/1.0");
            http.addHeader("Accept", "text/calendar");

            // Perform GET request
            unsigned long requestStart = millis();
            int httpCode = http.GET();
            unsigned long requestDuration = millis() - requestStart;

            if (httpCode <= 0) {
                DEBUG_ERROR_PRINTF(">>> Attempt %d FAILED: HTTP request failed (code: %d, error: %s, duration: %lums)\n",
                             attemptNum, httpCode, http.errorToString(httpCode).c_str(), requestDuration);
                http.end();
                retries--;
                if (retries > 0) {
                    DEBUG_INFO_PRINTLN(">>> Retrying in 5 seconds...");
                    delay(5000);
                }
                continue;
            }

            DEBUG_INFO_PRINTF(">>> Attempt %d: HTTP response code: %d (request took %lums)\n", attemptNum, httpCode, requestDuration);

            if (httpCode != HTTP_CODE_OK) {
                DEBUG_ERROR_PRINTF(">>> Attempt %d FAILED: HTTP error %d\n", attemptNum, httpCode);
                http.end();
                retries--;
                if (retries > 0) {
                    DEBUG_INFO_PRINTLN(">>> Retrying in 5 seconds...");
                    delay(5000);
                }
                continue;
            }

            // Get the stream
            Stream* stream = http.getStreamPtr();
            if (!stream) {
                DEBUG_ERROR_PRINTF(">>> Attempt %d FAILED: Could not get HTTP stream pointer\n", attemptNum);
                http.end();
                retries--;
                if (retries > 0) {
                    DEBUG_INFO_PRINTLN(">>> Retrying in 5 seconds...");
                    delay(5000);
                }
                continue;
            }

            // Get content length for debugging
            int contentLength = http.getSize();
            if (contentLength >= 0) {
                DEBUG_INFO_PRINTF(">>> Attempt %d SUCCESS: HTTP stream opened (Content-Length: %d bytes)\n", attemptNum, contentLength);
            } else {
                DEBUG_INFO_PRINTF(">>> Attempt %d SUCCESS: HTTP stream opened (chunked transfer, size unknown)\n", attemptNum);
            }

            return stream; // Success
        }

        DEBUG_ERROR_PRINTLN(">>> All " + String(maxRetries) + " HTTP fetch attempts FAILED");
        return nullptr; // All retries failed
    }
}

void CalendarFetcher::endStream() {
    // Close file stream if open
    if (fileStream) {
        fileStream.close();
    }

    // Close HTTP stream if open
    if (http.connected()) {
        http.end();
    }

    // Clean up WiFi client (if we still have one from old code)
    if (streamClient) {
        if (streamClient->connected()) {
            streamClient->stop();
        }
        delete streamClient;
        streamClient = nullptr;
    }
}