#include "display_manager.h"
#include "localization.h"
#include "debug_config.h"
#include "weather_client.h"
#include "string_utils.h"
#include "version.h"
#include "date_utils.h"
#include <assets/fonts.h>
#include <assets/icons/icons.h>

// ============================================================================
// Landscape Layout Implementation
// ============================================================================
// This file contains all landscape-specific rendering code.
// It is only compiled when DISPLAY_ORIENTATION == LANDSCAPE

#if DISPLAY_ORIENTATION == LANDSCAPE

// ============================================================================
// Header
// ============================================================================

void DisplayManager::drawLandscapeHeader(time_t now, const WeatherData* weatherData)
{
    // Extract date/time components
    struct tm* timeinfo = localtime(&now);
    int currentDay = timeinfo->tm_mday;
    int currentMonth = timeinfo->tm_mon + 1;
    int currentYear = timeinfo->tm_year + 1900;

    // Format month/year string
    String monthYear;
    if (currentYear <= 1970 || currentMonth < 1 || currentMonth > 12) {
        monthYear = "---";
    } else {
        monthYear = String(MONTH_NAMES[currentMonth]) + " " + String(currentYear);
    }

    // Header for left side only (within 400px width) - centered
    String dayStr = (currentDay > 0 && currentDay <= 31) ? String(currentDay) : "--";

    // Draw sunrise and sunset at corners if weather data is available
    if (weatherData && !weatherData->dailyForecast.empty()) {
        const WeatherDay& today = weatherData->dailyForecast[0];
        display.setFont(&FONT_SUNRISE_SUNSET);
        display.setTextColor(GxEPD_BLACK);

        // Draw sunrise at top-left corner with icon
        if (today.sunrise.length() >= 16) {
            String sunriseTime = today.sunrise.substring(11, 16);
            display.drawInvertedBitmap(10, 8, wi_sunrise_24x24, 24, 24, GxEPD_BLACK);
            display.setCursor(38, 22);
            display.print(sunriseTime);
        }

        // Draw sunset at top-right corner with icon
        if (today.sunset.length() >= 16) {
            String sunsetTime = today.sunset.substring(11, 16);
            int16_t textWidth = getTextWidth(sunsetTime, &FONT_SUNRISE_SUNSET);
            int iconX = LEFT_WIDTH - textWidth - 38;
            display.drawInvertedBitmap(iconX, 8, wi_sunset_24x24, 24, 24, GxEPD_BLACK);
            display.setCursor(iconX + 28, 22);
            display.print(sunsetTime);
        }
    }

    // Use font metrics for accurate centering
    int16_t dayWidth = getTextWidth(dayStr, &FONT_HEADER_DAY_NUMBER);
    int16_t monthYearWidth = getTextWidth(monthYear, &FONT_HEADER_MONTH_YEAR);

    // Calculate vertical positions
    int16_t dayBaseline = getFontBaseline(&FONT_HEADER_DAY_NUMBER);
    int16_t dayY = 10 + dayBaseline;

    // Center and draw the day number
    display.setFont(&FONT_HEADER_DAY_NUMBER);
    int dayX = (LEFT_WIDTH - dayWidth) / 2;
    display.setCursor(dayX, dayY);

#ifdef DISP_TYPE_6C
    display.setTextColor(COLOR_HEADER_DAY_NUMBER);
    display.print(dayStr);
    display.setTextColor(GxEPD_BLACK);
#else
    display.print(dayStr);
#endif

    // Position month/year
    int16_t monthYearY = dayY + getFontHeight(&FONT_HEADER_DAY_NUMBER) - 28;

    // Center and draw the month and year
    display.setFont(&FONT_HEADER_MONTH_YEAR);
    int monthYearX = (LEFT_WIDTH - monthYearWidth) / 2;
    display.setCursor(monthYearX, monthYearY);
    display.print(monthYear);

    // Draw separator
    int16_t separatorY = calculateYPosition(monthYearY, &FONT_HEADER_MONTH_YEAR, 15);
    display.drawLine(10, separatorY, LEFT_WIDTH - 10, separatorY, GxEPD_BLACK);
}

// ============================================================================
// Calendar
// ============================================================================

void DisplayManager::drawLandscapeCalendar(const MonthCalendar& calendar,
                                           const std::vector<CalendarEvent*>& events)
{
    int startX = CALENDAR_MARGIN;
    int startY = CALENDAR_START_Y;
    int cellWidth = (LEFT_WIDTH - (2 * CALENDAR_MARGIN)) / 7;

    // Draw day labels
    drawCalendarDayLabels(startX, startY, cellWidth);

    // Move down for calendar grid
    startY += DAY_LABEL_HEIGHT + 10;

    int row = 0, col = 0;

    // Draw previous month days
    drawCalendarPrevMonthDays(startX, startY, cellWidth, calendar, events, row, col);

    // Draw current month days
    drawCalendarCurrentMonthDays(startX, startY, cellWidth, calendar, row, col);

    // Draw next month days
    drawCalendarNextMonthDays(startX, startY, cellWidth, calendar, events, row, col);
}

// ============================================================================
// Events
// ============================================================================

void DisplayManager::drawLandscapeEvents(const std::vector<CalendarEvent*>& events)
{
    // Draw events in the top right section
    DEBUG_VERBOSE_PRINTLN("=== DrawLandscapeEvents called ===");
    DEBUG_VERBOSE_PRINTLN("Total events received: " + String(events.size()));

    int x = RIGHT_START_X + 20;
    int y = 25;
    const int maxY = WEATHER_START_Y - 30;

    if (events.empty()) {
        // Use font metrics for proper centering
        String noEventsText = String(LOC_NO_EVENTS);
        int16_t textWidth = getTextWidth(noEventsText, &FONT_NO_EVENTS);
        int16_t baseline = getFontBaseline(&FONT_NO_EVENTS);

        int boxWidth = display.width() - RIGHT_START_X;
        int boxHeight = WEATHER_START_Y - HEADER_HEIGHT;

        int centerX = RIGHT_START_X + boxWidth / 2 - textWidth / 2;
        int centerY = HEADER_HEIGHT + boxHeight / 2 + baseline / 2;

        display.setFont(&FONT_NO_EVENTS);
        display.setCursor(centerX, centerY);
        display.print(noEventsText);
        return;
    }

    // Get current date for formatting
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    int currentYear = timeinfo->tm_year + 1900;
    int currentMonth = timeinfo->tm_mon + 1;
    int currentDay = timeinfo->tm_mday;

    String currentDateHeader = "";
    int eventCount = 0;

    for (size_t i = 0; i < events.size() && y < maxY; i++) {
        CalendarEvent* event = events[i];

        if (eventCount >= MAX_EVENTS_TO_SHOW) {
            display.setFont(&FONT_EVENT_TITLE);
            display.setCursor(x, y);
            display.print("+" + String(events.size() - eventCount) + " " + LOC_MORE_EVENTS);
            break;
        }

        // Format date header
        String dateHeader = formatEventDate(event->date, currentYear, currentMonth, currentDay);

        // Draw date header if changed
        if (dateHeader != currentDateHeader) {
            currentDateHeader = dateHeader;

            if (y > 25) {
                y += 10;
            }

            display.setFont(&FONT_EVENT_DATE_HEADER);

#ifdef DISP_TYPE_6C
            if (dateHeader == String(LOC_TODAY)) {
                display.setTextColor(COLOR_EVENT_TODAY_HEADER);
            } else if (dateHeader == String(LOC_TOMORROW)) {
                display.setTextColor(COLOR_EVENT_TOMORROW_HEADER);
            } else {
                display.setTextColor(COLOR_EVENT_OTHER_HEADER);
            }
#endif

            display.setCursor(x, y);
            display.print(dateHeader);
            display.setTextColor(GxEPD_BLACK);

            y += 20;
        }

        // Draw event time
        display.setFont(&FONT_EVENT_TIME);
        String eventTime = event->allDay ? "--" : event->getStartTimeStr();
        display.setCursor(x, y);
        display.print(eventTime);

        // Draw event title
        display.setFont(&FONT_EVENT_TITLE);
        int titleX = x + 50;
        int titleMaxWidth = DISPLAY_WIDTH - titleX - 20;
        String title = StringUtils::truncate(StringUtils::removeAccents(event->title), titleMaxWidth, "...");
        display.setCursor(titleX, y);
        display.print(title);

        y += 20;
        eventCount++;
    }
}

// ============================================================================
// Weather
// ============================================================================

void DisplayManager::drawLandscapeWeatherPlaceholder()
{
    int x = RIGHT_START_X + 20;
    int y = WEATHER_START_Y + 6;

#ifdef DISP_TYPE_6C
    display.drawInvertedBitmap(x + 10, y - 14, wi_na_48x48, 48, 48, COLOR_WEATHER_ICON);
#else
    display.drawInvertedBitmap(x + 10, y - 14, wi_na_48x48, 48, 48, GxEPD_BLACK);
#endif

    display.setFont(&FONT_WEATHER_TEMP_MAIN);
    display.setCursor(x + 70, y + 15);
    display.print("--\260 / --\260");

    y += 40;

    display.setFont(&FONT_WEATHER_MESSAGE);
    display.setCursor(x, y + 20);
    display.print(LOC_WEATHER_COMING_SOON);
}

void DisplayManager::drawLandscapeWeather(const WeatherData& weatherData)
{
    if (weatherData.dailyForecast.empty()) {
        display.setFont(&FONT_WEATHER_MESSAGE);
        int x = RIGHT_START_X + 20;
        int y = WEATHER_START_Y + 50;
        display.setCursor(x, y);
        display.print(LOC_NO_WEATHER_DATA);
        return;
    }

    int sectionWidth = RIGHT_WIDTH - 40;
    int halfWidth = sectionWidth / 2;
    int startX = RIGHT_START_X + 20;
    int startY = WEATHER_START_Y + 10;

    WeatherClient tempClient(nullptr);

    // Today's weather
    if (weatherData.dailyForecast.size() > 0) {
        const WeatherDay& today = weatherData.dailyForecast[0];
        int x = startX;
        int y = startY;

        const uint8_t* todayIcon = tempClient.getWeatherIconBitmap(today.weatherCode, true, 64);
        if (todayIcon) {
#ifdef DISP_TYPE_6C
            display.drawInvertedBitmap(x, y, todayIcon, 64, 64, COLOR_WEATHER_ICON);
#else
            display.drawInvertedBitmap(x, y, todayIcon, 64, 64, GxEPD_BLACK);
#endif
        }

        display.setFont(&FONT_WEATHER_LABEL);
        display.setCursor(x + 70, y + 15);
        display.print(LOC_TODAY);

        display.setFont(&FONT_WEATHER_RAIN);
        display.setCursor(x + 70, y + 35);
        display.print(String(today.precipitationProbability) + "% " + LOC_RAIN);

        display.setFont(&FONT_WEATHER_TEMP_MAIN);
        String tempRange = String(int(today.tempMin)) + "\260 / " + String(int(today.tempMax)) + "\260";
        display.setCursor(x + 70, y + 55);
        display.print(tempRange);
    }

    // Tomorrow's weather
    if (weatherData.dailyForecast.size() > 1) {
        const WeatherDay& tomorrow = weatherData.dailyForecast[1];
        int x = startX + halfWidth;
        int y = startY;

        const uint8_t* tomorrowIcon = tempClient.getWeatherIconBitmap(tomorrow.weatherCode, true, 64);
        if (tomorrowIcon) {
#ifdef DISP_TYPE_6C
            display.drawInvertedBitmap(x, y, tomorrowIcon, 64, 64, COLOR_WEATHER_ICON);
#else
            display.drawInvertedBitmap(x, y, tomorrowIcon, 64, 64, GxEPD_BLACK);
#endif
        }

        display.setFont(&FONT_WEATHER_LABEL);
        display.setCursor(x + 70, y + 15);
        display.print(LOC_TOMORROW);

        display.setFont(&FONT_WEATHER_RAIN);
        display.setCursor(x + 70, y + 35);
        display.print(String(tomorrow.precipitationProbability) + "% " + LOC_RAIN);

        display.setFont(&FONT_WEATHER_TEMP_MAIN);
        String tempRange = String(int(tomorrow.tempMin)) + "\260 / " + String(int(tomorrow.tempMax)) + "\260";
        display.setCursor(x + 70, y + 55);
        display.print(tempRange);
    }
}

// ============================================================================
// Status Bar
// ============================================================================

void DisplayManager::drawLandscapeStatusBar(bool wifiConnected, int rssi,
    float batteryVoltage, int batteryPercentage,
    time_t now, bool isStale)
{
    display.setFont(&FONT_STATUSBAR);
    int y = DISPLAY_HEIGHT;
    int textY = y - 5;

    // LEFT: Battery icon and percentage
    int iconX = 10;
    if (batteryPercentage > 90) {
        display.drawInvertedBitmap(iconX, y - 16, battery_full_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 75) {
        display.drawInvertedBitmap(iconX, y - 16, battery_6_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 60) {
        display.drawInvertedBitmap(iconX, y - 16, battery_5_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 45) {
        display.drawInvertedBitmap(iconX, y - 16, battery_4_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 30) {
        display.drawInvertedBitmap(iconX, y - 16, battery_3_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 15) {
        display.drawInvertedBitmap(iconX, y - 16, battery_2_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else if (batteryPercentage > 5) {
        display.drawInvertedBitmap(iconX, y - 16, battery_1_bar_90deg_16x16, 16, 16, GxEPD_BLACK);
    } else {
        display.drawInvertedBitmap(iconX, y - 16, battery_alert_90deg_16x16, 16, 16, GxEPD_BLACK);
    }

    iconX += 20;
    display.setCursor(iconX, textY);
    display.print(String(batteryPercentage) + "%");

    // CENTER: Last update date and time
    String dateTimeStr = DateUtils::formatDate(now) + " " + DateUtils::formatTime(now);
    if (isStale) {
        dateTimeStr = "[!] " + dateTimeStr;
    }
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dateTimeStr, 0, 0, &x1, &y1, &w, &h);
    int centerX = (DISPLAY_WIDTH - w) / 2;
    display.setCursor(centerX, textY);
    display.print(dateTimeStr);

    // RIGHT: WiFi icon and RSSI
    if (wifiConnected) {
        String rssiStr = String(rssi) + "dBm";
        display.getTextBounds(rssiStr, 0, 0, &x1, &y1, &w, &h);
        int rssiX = DISPLAY_WIDTH - w - 10;
        display.setCursor(rssiX, textY);
        display.print(rssiStr);

        int wifiIconX = rssiX - 20;
        if (rssi > -60) {
            display.drawInvertedBitmap(wifiIconX, y - 16, wifi_3_bar_16x16, 16, 16, GxEPD_BLACK);
        } else if (rssi > -75) {
            display.drawInvertedBitmap(wifiIconX, y - 16, wifi_2_bar_16x16, 16, 16, GxEPD_BLACK);
        } else {
            display.drawInvertedBitmap(wifiIconX, y - 16, wifi_1_bar_16x16, 16, 16, GxEPD_BLACK);
        }
    } else {
        String disconnectedStr = "WiFi Off";
        display.getTextBounds(disconnectedStr, 0, 0, &x1, &y1, &w, &h);
        int disconnectedX = DISPLAY_WIDTH - w - 10;
        display.setCursor(disconnectedX, textY);
        display.print(disconnectedStr);

        int wifiIconX = disconnectedX - 20;
        display.drawInvertedBitmap(wifiIconX, y - 16, wifi_off_16x16, 16, 16, GxEPD_BLACK);
    }
}

#endif // DISPLAY_ORIENTATION == LANDSCAPE
