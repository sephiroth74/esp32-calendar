#include "debug_config.h"
#include "display_manager.h"
#include "localization.h"
#include "string_utils.h"
#include "version.h"
#include <assets/fonts.h>
#include <assets/icons/icons.h>

// ============================================================================
// Shared Display Functions (used by both orientations)
// ============================================================================

String DisplayManager::formatEventDate(const String& eventDate,
                                       int currentYear,
                                       int currentMonth,
                                       int currentDay) {
    // Parse event date
    if (eventDate.length() < 10)
        return "";

    int eventYear  = eventDate.substring(0, 4).toInt();
    int eventMonth = eventDate.substring(5, 7).toInt();
    int eventDay   = eventDate.substring(8, 10).toInt();

    DEBUG_VERBOSE_PRINTLN("formatEventDate: Event date " + String(eventYear) + "-" +
                          String(eventMonth) + "-" + String(eventDay) + " vs Current " +
                          String(currentYear) + "-" + String(currentMonth) + "-" +
                          String(currentDay));

    // Check if today
    if (eventYear == currentYear && eventMonth == currentMonth && eventDay == currentDay) {
        DEBUG_VERBOSE_PRINTLN("  -> Returning TODAY: " + String(LOC_TODAY));
        return String(LOC_TODAY);
    }

    // Check if tomorrow
    time_t now;
    time(&now);
    time_t tomorrow         = now + 86400; // Add 24 hours
    struct tm* tomorrowInfo = localtime(&tomorrow);

    DEBUG_VERBOSE_PRINTLN("  Tomorrow check: " + String(tomorrowInfo->tm_year + 1900) + "-" +
                          String(tomorrowInfo->tm_mon + 1) + "-" + String(tomorrowInfo->tm_mday));

    if (eventYear == tomorrowInfo->tm_year + 1900 && eventMonth == tomorrowInfo->tm_mon + 1 &&
        eventDay == tomorrowInfo->tm_mday) {
        DEBUG_VERBOSE_PRINTLN("  -> Returning TOMORROW: " + String(LOC_TOMORROW));
        return String(LOC_TOMORROW);
    }

    // Calculate day of week for the event date using Zeller's congruence
    int m = eventMonth;
    int y = eventYear;
    if (m < 3) {
        m += 12;
        y--;
    }
    int k          = y % 100;
    int j          = y / 100;
    int h          = (eventDay + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    int dayOfWeek  = (h + 6) % 7; // Convert to 0=Sunday

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
    return dayName + " " + String(eventDay) + " " + String(MONTH_NAMES[eventMonth]) + " " +
           String(eventYear);
}

void DisplayManager::drawStatusBar(bool wifiConnected,
                                   int rssi,
                                   float batteryVoltage,
                                   int batteryPercentage,
                                   int currentDay,
                                   int currentMonth,
                                   int currentYear,
                                   const String& currentTime,
                                   bool isStale) {
    display.setFont(nullptr); // Use default font for status bar
    int y = DISPLAY_HEIGHT;   // Position for status bar at bottom edge

    // Draw status icons and info at bottom
    int iconX = 10;
    int textY = y - 5;

    // WiFi icon and RSSI
    if (wifiConnected) {
        // Draw WiFi icon based on signal strength
        if (rssi > -60) {
            display.drawInvertedBitmap(iconX, y - 16, wifi_3_bar_16x16, 16, 16, GxEPD_BLACK);
        } else if (rssi > -75) {
            display.drawInvertedBitmap(iconX, y - 16, wifi_2_bar_16x16, 16, 16, GxEPD_BLACK);
        } else {
            display.drawInvertedBitmap(iconX, y - 16, wifi_1_bar_16x16, 16, 16, GxEPD_BLACK);
        }
    } else {
        display.drawInvertedBitmap(iconX, y - 16, wifi_off_16x16, 16, 16, GxEPD_BLACK);
    }

    // Battery icon and percentage
    iconX += 20;
    if (batteryPercentage > 90) {
        display.drawInvertedBitmap(iconX, y - 16, battery_full_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 75) {
        display.drawInvertedBitmap(iconX, y - 16, battery_6_bar_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 60) {
        display.drawInvertedBitmap(iconX, y - 16, battery_5_bar_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 45) {
        display.drawInvertedBitmap(iconX, y - 16, battery_4_bar_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 30) {
        display.drawInvertedBitmap(iconX, y - 16, battery_3_bar_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 15) {
        display.drawInvertedBitmap(iconX, y - 16, battery_2_bar_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 5) {
        display.drawInvertedBitmap(iconX, y - 16, battery_1_bar_0deg_16x16, 16, 16, GxEPD_BLACK);
    } else {
        display.drawInvertedBitmap(iconX, y - 16, battery_alert_0deg_16x16, 16, 16, GxEPD_BLACK);
    }

    iconX += 20;
    display.setCursor(iconX, textY);
    display.print(String(batteryPercentage) + "%");

    // Show stale data indicator if applicable
    if (isStale) {
        iconX += 50;
        display.setCursor(iconX, textY);
        display.print("[STALE]");
    }

    // Version info on the right side
    String versionStr = "v" + String(VERSION);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(versionStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(DISPLAY_WIDTH - w - 10, textY);
    display.print(versionStr);
}

void DisplayManager::drawNoEvents(int x, int y) {
    display.setFont(&FONT_NO_EVENTS);
    display.setCursor(x, y);
    display.print(LOC_NO_EVENTS);
}

void DisplayManager::drawError(const String& error) {
    display.setFont(&FONT_ERROR_MESSAGE);
    centerText(error, 0, 100, DISPLAY_WIDTH, &FONT_ERROR_MESSAGE);
}

void DisplayManager::showFullScreenError(const ErrorInfo& error) {
    display.firstPage();
    do {
        fillScreen(GxEPD_WHITE);

        // Use generic error icon
        const uint8_t* errorIcon = error_icon_96x96;

        // Draw error icon centered
        int iconSize = 96;
        int iconX    = (DISPLAY_WIDTH - iconSize) / 2;
        int iconY    = 80;

#ifdef DISP_TYPE_6C
        display.drawInvertedBitmap(iconX, iconY, errorIcon, iconSize, iconSize, GxEPD_RED);
#else
        display.drawInvertedBitmap(iconX, iconY, errorIcon, iconSize, iconSize, GxEPD_BLACK);
#endif

        // Draw error title
        display.setFont(&FONT_ERROR_TITLE);
        centerText("ERROR", 0, 40, DISPLAY_WIDTH, &FONT_ERROR_TITLE);

        // Draw error message
        display.setFont(&FONT_ERROR_MESSAGE);
        centerText(error.message, 0, iconY + iconSize + 50, DISPLAY_WIDTH, &FONT_ERROR_MESSAGE);

        // Draw error details if available
        if (error.details.length() > 0) {
            display.setFont(&FONT_ERROR_DETAILS);
            centerText(
                error.details, 0, iconY + iconSize + 130, DISPLAY_WIDTH, &FONT_ERROR_DETAILS);
        }

        // Draw error code at bottom
        display.setFont(nullptr);
        String errorCode = "Error Code: " + String((int)error.code);
        display.setCursor(20, DISPLAY_HEIGHT - 10);
        display.print(errorCode);

        // Draw retry info if applicable
        if (error.recoverable && error.maxRetries > 0) {
            display.setFont(&FONT_ERROR_DETAILS);
            String retryText = "Retrying...";
            retryText += " (" + String(error.retryCount) + "/" + String(error.maxRetries) + ")";
            display.setCursor(20, DISPLAY_HEIGHT - 30);
            display.print(retryText);
        }

    } while (display.nextPage());
}
