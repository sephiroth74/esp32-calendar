#include "display_manager.h"
#include "localization.h"
#include "debug_config.h"
#include "string_utils.h"
#include "version.h"
#include <assets/fonts.h>
#include <assets/icons/icons.h>

// ============================================================================
// Events, Status Bar, and Error Display Functions
// ============================================================================

String DisplayManager::formatEventDate(const String& eventDate, int currentYear, int currentMonth, int currentDay)
{
    // Parse event date
    if (eventDate.length() < 10) return "";

    int eventYear = eventDate.substring(0, 4).toInt();
    int eventMonth = eventDate.substring(5, 7).toInt();
    int eventDay = eventDate.substring(8, 10).toInt();

    DEBUG_VERBOSE_PRINTLN("formatEventDate: Event date " + String(eventYear) + "-" + String(eventMonth) + "-" + String(eventDay) +
                   " vs Current " + String(currentYear) + "-" + String(currentMonth) + "-" + String(currentDay));

    // Check if today
    if (eventYear == currentYear && eventMonth == currentMonth && eventDay == currentDay) {
        DEBUG_VERBOSE_PRINTLN("  -> Returning TODAY: " + String(LOC_TODAY));
        return String(LOC_TODAY);
    }

    // Check if tomorrow
    time_t now;
    time(&now);
    time_t tomorrow = now + 86400; // Add 24 hours
    struct tm* tomorrowInfo = localtime(&tomorrow);

    DEBUG_VERBOSE_PRINTLN("  Tomorrow check: " + String(tomorrowInfo->tm_year + 1900) + "-" +
                   String(tomorrowInfo->tm_mon + 1) + "-" + String(tomorrowInfo->tm_mday));

    if (eventYear == tomorrowInfo->tm_year + 1900 &&
        eventMonth == tomorrowInfo->tm_mon + 1 &&
        eventDay == tomorrowInfo->tm_mday) {
        DEBUG_VERBOSE_PRINTLN("  -> Returning TOMORROW: " + String(LOC_TOMORROW));
        return String(LOC_TOMORROW);
    }

    // Calculate day of week for the event date
    // Using Zeller's congruence
    int m = eventMonth;
    int y = eventYear;
    if (m < 3) {
        m += 12;
        y--;
    }
    int k = y % 100;
    int j = y / 100;
    int h = (eventDay + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    int dayOfWeek = (h + 6) % 7; // Convert to 0=Sunday

    String dayName = String(DAY_NAMES[dayOfWeek]);

    // Same month and year - just show "Sunday 19"
    if (eventYear == currentYear && eventMonth == currentMonth) {
        return dayName + " " + String(eventDay);
    }

    // Same year but different month - show "Saturday, November 4"
    if (eventYear == currentYear) {
        return dayName + " " + String(eventDay) + " " + String(MONTH_NAMES[eventMonth]);
    }

    // Different year - show "Sunday, January 1, 2026"
    return dayName + " " + String(eventDay) + " " + String(MONTH_NAMES[eventMonth]) + " " + String(eventYear);
}

void DisplayManager::drawEventsSection(const std::vector<CalendarEvent*>& events)
{
    // Draw events in the top right section
    DEBUG_VERBOSE_PRINTLN("=== DrawEventsSection called ===");
    DEBUG_VERBOSE_PRINTLN("Total events received: " + String(events.size()));
    if (DEBUG_LEVEL >= DEBUG_VERBOSE) {
        for (size_t i = 0; i < events.size() && i < 5; i++) {
            DEBUG_VERBOSE_PRINTLN("Event " + String(i) + ": " + events[i]->title + " on " + events[i]->date +
                          " isToday=" + String(events[i]->isToday) + " isTomorrow=" + String(events[i]->isTomorrow));
        }
    }

    int x = RIGHT_START_X + 20;
    int y = 25; // Start a bit higher to maximize space
    const int maxY = WEATHER_START_Y - 10; // Stop 10px before weather section

    if (events.empty()) {
        // Use font metrics for proper centering
        String noEventsText = String(LOC_NO_EVENTS);
        int16_t textWidth = getTextWidth(noEventsText, &FONT_NO_EVENTS);
        int16_t baseline = getFontBaseline(&FONT_NO_EVENTS);

        // Calculate box dimensions
        int boxWidth = display.width() - RIGHT_START_X;
        int boxHeight = WEATHER_START_Y - HEADER_HEIGHT;

        // Center using font metrics
        int textX = RIGHT_START_X + (boxWidth - textWidth) / 2;
        int textY = HEADER_HEIGHT + (boxHeight / 2) + (baseline / 2);

        display.setFont(&FONT_NO_EVENTS);
        display.setCursor(textX, textY);
        display.print(noEventsText);
        return;
    }

    // Get current date for comparison
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    int currentYear = timeinfo->tm_year + 1900;
    int currentMonth = timeinfo->tm_mon + 1;
    int currentDay = timeinfo->tm_mday;

    // Track displayed events and last date header
    int eventCount = 0;
    String lastDateHeader = "";
    const int timeColumnWidth = 70; // Reduced from 85px

    // Process all events in chronological order
    for (auto event : events) {
        // Check if we have space for at least one more date header + event (minimum 35px)
        if (y + 35 > maxY) {
            break; // No more space
        }

        // Get formatted date for this event
        String dateHeader = formatEventDate(event->date, currentYear, currentMonth, currentDay);

        // Debug output for date header
        DEBUG_VERBOSE_PRINTLN("Date header for event: '" + dateHeader + "' (last: '" + lastDateHeader + "')");

        // If this is a new date, show the date header
        if (dateHeader != lastDateHeader) {
            // Add spacing between date groups (except for first)
            if (lastDateHeader != "") {
                y += 20;  // More space between different dates
            }

            // Check if we still have space after spacing
            if (y + 35 > maxY) break;

            // Draw date header
            display.setFont(&FONT_EVENT_DATE_HEADER);
            display.setCursor(x, y);

            DEBUG_VERBOSE_PRINTLN("Drawing date header at y=" + String(y) + ": " + dateHeader);

#ifdef DISP_TYPE_6C
            // Use proper date comparison instead of string matching
            if (event->isToday) {
                display.setTextColor(GxEPD_RED);  // Red for today
            } else if (event->isTomorrow) {
                display.setTextColor(GxEPD_RED);  // Red for tomorrow
            } else {
                display.setTextColor(GxEPD_BLACK);  // Black for other dates
            }
            display.print(dateHeader);
            display.setTextColor(GxEPD_BLACK); // Reset to black
#else
            display.print(dateHeader);
#endif
            y += 25;  // More space after date header

            lastDateHeader = dateHeader;
        }

        // Check if we have space for this event (minimum 20px)
        if (y + 20 > maxY) break;

        // Time or "All Day" in smaller font
        display.setFont(&FONT_EVENT_TIME);
        String timeStr;
        if (event->allDay) {
            timeStr = "--";
        } else {
            timeStr = formatTime(event->startTimeStr);
        }
        display.setCursor(x, y);
        display.print(timeStr);

        // Event title with smaller font - SINGLE LINE with ellipsis
        display.setFont(&FONT_EVENT_TITLE);
        String title = StringUtils::convertAccents(event->title);
        int eventTextX = x + timeColumnWidth - 8; // Moved 8px left closer to time
        int maxPixelWidth = DISPLAY_WIDTH - eventTextX - 10;

        // Single line truncation with ellipsis
        int16_t x1, y1;
        uint16_t w, h;
        String displayText = title;

        // Check if text fits
        display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
        if (w > maxPixelWidth) {
            // Find how much fits with ellipsis
            for (int i = title.length() - 1; i > 0; i--) {
                String testStr = title.substring(0, i) + "...";
                display.getTextBounds(testStr, 0, 0, &x1, &y1, &w, &h);
                if (w <= maxPixelWidth) {
                    displayText = testStr;
                    break;
                }
            }
        }

        // Draw single line
        display.setCursor(eventTextX, y);
        display.print(displayText);

        // Single line spacing (allows for more events to fit)
        y += 20;

        eventCount++;
    }
}

void DisplayManager::drawStatusBar(bool wifiConnected, int rssi,
    float batteryVoltage, int batteryPercentage,
    int currentDay, int currentMonth, int currentYear, const String& currentTime, bool isStale)
{
    display.setFont(nullptr);   // Use default font for status bar
    int y = DISPLAY_HEIGHT; // Position for status bar at bottom edge

    // WiFi status with icon and RSSI text (bottom right)
    int iconSize = 16; // Use 16x16 icons for status bar
    int x = DISPLAY_WIDTH - 100; // Leave space for icon and text

    if (wifiConnected) {
        // Choose WiFi icon based on signal strength
        if (rssi > -50) {
            // Excellent signal - full WiFi icon
            display.drawInvertedBitmap(x, y - iconSize, wifi_16x16, 16, 16, GxEPD_BLACK);
        } else if (rssi > -60) {
            // Good signal - 3 bars
            display.drawInvertedBitmap(x, y - iconSize, wifi_3_bar_16x16, 16, 16, GxEPD_BLACK);
        } else if (rssi > -70) {
            // Fair signal - 2 bars
            display.drawInvertedBitmap(x, y - iconSize, wifi_2_bar_16x16, 16, 16, GxEPD_BLACK);
        } else {
            // Weak signal - 1 bar
            display.drawInvertedBitmap(x, y - iconSize, wifi_1_bar_16x16, 16, 16, GxEPD_BLACK);
        }

        // Display RSSI value as text
        display.setCursor(x + iconSize + 5, y - 8);
        display.print(String(rssi) + "dBm");
    } else {
        // No connection - show WiFi X icon
        display.drawInvertedBitmap(x, y - iconSize, wifi_x_16x16, 16, 16, GxEPD_BLACK);

        // Show "No WiFi" text
        display.setCursor(x + iconSize + 5, y - 8);
        display.print("No WiFi");
    }

    // Battery status with icon (bottom left)
    if (batteryVoltage > 0) {
        x = 20;

        // Choose battery icon based on percentage - moved icon 4 pixels down
        if (batteryPercentage >= 100) {
            // Full battery
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_full_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else if (batteryPercentage >= 85) {
            // 6 bars
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_6_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else if (batteryPercentage >= 70) {
            // 5 bars
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_5_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else if (batteryPercentage >= 55) {
            // 4 bars
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_4_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else if (batteryPercentage >= 40) {
            // 3 bars
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_3_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else if (batteryPercentage >= 25) {
            // 2 bars
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_2_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else if (batteryPercentage >= 10) {
            // 1 bar
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_1_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        } else {
            // Empty battery
            display.drawInvertedBitmap(x, y - iconSize + 4, battery_0_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
        }

        // Show percentage as text
        display.setFont(nullptr); // Use default font
        display.setCursor(x + iconSize + 5, y - 8);
        display.print(String(batteryPercentage) + "%");
    }

    // Date/time and version info (center area)

    // Format date and time string
    String dateTimeStr = String(currentDay) + "/" + String(currentMonth) + "/" + String(currentYear) + " " + currentTime;
    String versionStr = "v" + getVersionString();
    String combinedStr = dateTimeStr + "  |  " + versionStr;

    if (isStale) {
        auto icon = getBitmap(sync_problem, 16);
        // Draw stale icon before the text
        if(icon) {
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(combinedStr, 0, 0, &x1, &y1, &w, &h);
            int iconX = (DISPLAY_WIDTH - w) / 2 - 20; // 20px space before text
            int iconY = y - 12; // Center vertically with text
            display.drawInvertedBitmap(iconX, iconY, icon, 16, 16, GxEPD_BLACK);
        }
        // combinedStr += " (stale)";
    }

    // Center the combined string
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(combinedStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((DISPLAY_WIDTH - w) / 2, y - 8);
    display.print(combinedStr);
}

void DisplayManager::drawError(const String& error)
{
    display.setFont(&FONT_ERROR_TITLE);
    centerText(LOC_ERROR, 0, DISPLAY_HEIGHT / 2 - 40, DISPLAY_WIDTH, &FONT_ERROR_TITLE);

    display.setFont(&FONT_ERROR_MESSAGE);
    centerText(error, 0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH, &FONT_ERROR_MESSAGE);
}

void DisplayManager::showFullScreenError(const ErrorInfo& error)
{
    DEBUG_INFO_PRINTF("[DisplayManager] showFullScreenError: code=%d, level=%d, icon=%d, message=%s\n",
                      (int)error.code, (int)error.level, (int)error.icon, error.message.c_str());

    display.setTextColor(GxEPD_BLACK);
    display.firstPage();
    do {
        fillScreen(GxEPD_WHITE);

        // Draw large icon at top - use 64x64 for better visibility
        int iconSize = 196;
        int iconX = (DISPLAY_WIDTH - iconSize) / 2;
        int iconY = 50;
        const unsigned char* icon = nullptr;

        // Draw appropriate icon based on error type
        switch (error.icon) {
        case ErrorIcon::WIFI:
            // Use WiFi X icon for WiFi connection errors
            icon = getBitmap(wifi_x, iconSize);
            break;
        case ErrorIcon::BATTERY:
            // Use battery alert icon for battery errors
            icon = getBitmap(battery_alert_0deg, iconSize);
            break;
        case ErrorIcon::MEMORY:
            // Use error icon for memory errors
            icon = getBitmap(error_icon, iconSize);
            break;
        case ErrorIcon::CALENDAR:
        case ErrorIcon::CLOCK:
        case ErrorIcon::NETWORK:
        case ErrorIcon::SETTINGS:
        case ErrorIcon::UPDATE:
            // Use warning icon for other error types
            icon = getBitmap(warning_icon, iconSize);
            break;
        default:
            // Default based on error level
            if (error.level == ErrorLevel::WARNING || error.level == ErrorLevel::INFO) {
                icon = getBitmap(warning_icon, iconSize);
            } else {
                icon = getBitmap(error_icon, iconSize);
            }
            break;
        }

        if (icon) {
            display.drawInvertedBitmap(iconX, iconY, icon, iconSize, iconSize, GxEPD_BLACK);
        } else {
            DEBUG_WARN_PRINTF("[DisplayManager] showFullScreenError: No icon found for error icon type %d\n", (int)error.icon);
        }

        // Draw error level text
        display.setFont(&FONT_ERROR_MESSAGE);
        String levelText;
        switch (error.level) {
        case ErrorLevel::INFO:
            levelText = LOC_ERROR_LEVEL_INFO;
            break;
        case ErrorLevel::WARNING:
            levelText = LOC_ERROR_LEVEL_WARNING;
            break;
        case ErrorLevel::ERROR:
            levelText = LOC_ERROR_LEVEL_ERROR;
            break;
        case ErrorLevel::CRITICAL:
            levelText = LOC_ERROR_LEVEL_CRITICAL;
            break;
        }
        centerText(levelText, 0, iconY + iconSize + 40, DISPLAY_WIDTH, &FONT_ERROR_MESSAGE);

        // Draw main error message
        display.setFont(&FONT_ERROR_TITLE);
        centerText(error.message, 0, iconY + iconSize + 90, DISPLAY_WIDTH, &FONT_ERROR_TITLE);

        // Draw error details if available
        if (error.details.length() > 0) {
            display.setFont(&FONT_ERROR_DETAILS);
            centerText(error.details, 0, iconY + iconSize + 130, DISPLAY_WIDTH, &FONT_ERROR_DETAILS);
        }

        // Draw error code at bottom
        display.setFont(nullptr);
        String errorCode = "Error Code: " + String((int)error.code);
        display.setCursor(20, DISPLAY_HEIGHT - 10);
        display.print(errorCode);

        // Draw retry info if applicable
        if (error.recoverable && error.maxRetries > 0) {
            display.setFont(&FONT_ERROR_DETAILS);
            String retryText = LOC_ERROR_RETRYING;
            retryText += " (" + String(error.retryCount) + "/" + String(error.maxRetries) + ")";
            display.setCursor(20, DISPLAY_HEIGHT - 10);
            display.print(retryText);
        }

        // Draw action hint
        display.setFont(&FONT_ERROR_DETAILS);
        String actionText;
        if (!error.recoverable) {
            actionText = LOC_ERROR_CHECK_SETTINGS;
        } else if (error.retryCount < error.maxRetries) {
            actionText = LOC_ERROR_PLEASE_WAIT;
        } else {
            actionText = LOC_ERROR_RESTART_DEVICE;
        }

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(actionText, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(DISPLAY_WIDTH - w - 20, DISPLAY_HEIGHT - 10);
        display.print(actionText);

    } while (display.nextPage());
}

void DisplayManager::drawNoEvents(int x, int y)
{
    display.setFont(&FONT_NO_EVENTS);
    display.setCursor(x, y);
    display.print(LOC_NO_EVENTS);

    display.setFont(&FONT_EVENT_DETAILS);
    display.setCursor(x, y + 40);
    display.print(LOC_ENJOY_FREE_DAY);
}
