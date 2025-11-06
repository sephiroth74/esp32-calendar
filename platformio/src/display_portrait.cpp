#include "display_manager.h"
#include "localization.h"
#include "debug_config.h"
#include "weather_client.h"
#include "string_utils.h"
#include "date_utils.h"
#include "version.h"
#include <assets/fonts.h>
#include <assets/icons/icons.h>

// ============================================================================
// Portrait Layout Implementation
// ============================================================================
// This file contains all portrait-specific rendering code.
// It is only compiled when DISPLAY_ORIENTATION == PORTRAIT

#if DISPLAY_ORIENTATION == PORTRAIT

// ============================================================================
// Header
// ============================================================================

void DisplayManager::drawPortraitHeader(time_t now, const WeatherData* weatherData)
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

    // Draw sunrise and sunset at corners if weather data is available
    if (weatherData && !weatherData->dailyForecast.empty()) {
        const WeatherDay& today = weatherData->dailyForecast[0];
        display.setFont(&FONT_SUNRISE_SUNSET);
        display.setTextColor(GxEPD_BLACK);

        // Draw sunrise at top-left corner with icon
        if (today.sunrise.length() >= 16) {
            String sunriseTime = today.sunrise.substring(11, 16);
            display.drawInvertedBitmap(10, 8, wi_sunrise_16x16, 16, 16, GxEPD_BLACK);
            display.setCursor(30, 19);
            display.print(sunriseTime);
        }

        // Draw sunset at top-right corner with icon
        if (today.sunset.length() >= 16) {
            String sunsetTime = today.sunset.substring(11, 16);
            int16_t textWidth = getTextWidth(sunsetTime, &FONT_SUNRISE_SUNSET);
            int iconX = DISPLAY_WIDTH - textWidth - 38;
            display.drawInvertedBitmap(iconX, 8, wi_sunset_16x16, 16, 16, GxEPD_BLACK);
            display.setCursor(iconX + 20, 19);
            display.print(sunsetTime);
        }
    }

    // TITLE: day number and month/year on single centered line
    String dayStr = (currentDay > 0 && currentDay <= 31) ? String(currentDay) : "--";

    // Calculate widths
    int16_t dayWidth = getTextWidth(dayStr, &FONT_HEADER_DAY_NUMBER);
    int16_t monthYearWidth = getTextWidth(monthYear, &FONT_HEADER_MONTH_YEAR);
    int16_t spaceWidth = 15; // More space between day and month (was 10)

    // Total width of the title
    int16_t totalWidth = dayWidth + spaceWidth + monthYearWidth;

    // Calculate starting X position to center the entire title
    int16_t startX = (DISPLAY_WIDTH - totalWidth) / 2;

    // Calculate Y position (using the larger font's baseline)
    int16_t dayBaseline = getFontBaseline(&FONT_HEADER_DAY_NUMBER);
    int16_t monthBaseline = getFontBaseline(&FONT_HEADER_MONTH_YEAR);
    int16_t y = 25 + max(dayBaseline, monthBaseline); // Moved up 5px (was 30)

    // Draw the day number
    display.setFont(&FONT_HEADER_DAY_NUMBER);
    display.setCursor(startX, y);
#ifdef DISP_TYPE_6C
    display.setTextColor(COLOR_HEADER_DAY_NUMBER);
    display.print(dayStr);
    display.setTextColor(GxEPD_BLACK);
#else
    display.print(dayStr);
#endif

    // Draw the month and year next to the day number
    display.setFont(&FONT_HEADER_MONTH_YEAR);
    display.setCursor(startX + dayWidth + spaceWidth, y);
    display.print(monthYear);

    // Draw separator line below the header
    // int16_t separatorY = y + 20;
    // display.drawLine(20, separatorY, DISPLAY_WIDTH - 20, separatorY, GxEPD_BLACK);
}

// ============================================================================
// Calendar
// ============================================================================

void DisplayManager::drawPortraitCalendar(const MonthCalendar& calendar,
                                          const std::vector<CalendarEvent*>& events)
{
    // Draw calendar grid using full width
    int startX = 10;
    int startY = CALENDAR_START_Y;
    int cellWidth = CELL_WIDTH;

    // Draw day labels (M T W T F S S)
    drawCalendarDayLabels(startX, startY, cellWidth);

    // Move down to start drawing dates
    startY += DAY_LABEL_HEIGHT + 5;

    int row = 0;
    int col = 0;

    // Draw previous month days (if any)
    drawCalendarPrevMonthDays(startX, startY, cellWidth, calendar, events, row, col);

    // Draw current month days
    display.setFont(&FONT_CALENDAR_DAY_NUMBERS);

    for (int day = 1; day <= calendar.daysInMonth; day++) {
        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        // Check if this is today
        bool isToday = (day == calendar.today);

        // Check if weekend
        int dayOfWeek = (col + FIRST_DAY_OF_WEEK) % 7;
        bool isWeekend = (dayOfWeek == 0 || dayOfWeek == 6);

        // Draw weekend background
        if (isWeekend && !isToday) {
#ifdef DISP_TYPE_6C
            drawDitheredRectangle(x, y, cellWidth, CELL_HEIGHT,
                GxEPD_WHITE, COLOR_CALENDAR_WEEKEND_BG, static_cast<DitherLevel>(DITHER_CALENDAR_WEEKEND));
#endif
        }

        // Draw today's cell with border
        if (isToday) {
#ifdef DISP_TYPE_6C
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, COLOR_CALENDAR_TODAY_BORDER);
            display.drawRect(x + 2, y + 2, cellWidth - 4, CELL_HEIGHT - 4, COLOR_CALENDAR_TODAY_BORDER);
#else
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, GxEPD_BLACK);
#endif
        }

        // Draw day number - positioned higher in the cell
        String dayStr = String(day);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20); // Moved down from 18 to 20

#ifdef DISP_TYPE_6C
        if (isToday) {
            display.setTextColor(COLOR_CALENDAR_TODAY_TEXT);
        }
#endif
        display.print(dayStr);

#ifdef DISP_TYPE_6C
        if (isToday) {
            display.setTextColor(GxEPD_BLACK);
        }
#endif

        // Draw event indicator if this day has events - positioned lower for better spacing
        if (calendar.hasEvent[day]) {
#ifdef DISP_TYPE_6C
            uint16_t dotColor = GxEPD_BLACK;
            if (!calendar.eventColors[day][0].isEmpty()) {
                String colorStr = calendar.eventColors[day][0];
                if (colorStr == "red") dotColor = GxEPD_RED;
                else if (colorStr == "orange") dotColor = GxEPD_ORANGE;
                else if (colorStr == "yellow") dotColor = GxEPD_YELLOW;
                else if (colorStr == "green") dotColor = GxEPD_GREEN;
            }
            display.fillCircle(x + cellWidth / 2, y + 32, 2, dotColor); // Moved down from 26 to 32
#else
            display.fillCircle(x + cellWidth / 2, y + 32, 2, GxEPD_BLACK); // Moved down from 26 to 32
#endif
        }

        // Move to next cell
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    // Draw next month days to fill the grid
    drawCalendarNextMonthDays(startX, startY, cellWidth, calendar, events, row, col);
}

// ============================================================================
// Events with Weather on Left Side
// ============================================================================

void DisplayManager::drawPortraitEventsWithWeather(const std::vector<CalendarEvent*>& events,
                                                    const WeatherData* weatherData)
{
    // WEATHER section: Left side with large icons ABOVE text
    // Draw weather on the left
    if (weatherData && !weatherData->dailyForecast.empty()) {
        WeatherClient tempClient(nullptr);

        // Get today's and tomorrow's forecast
        const WeatherDay& today = weatherData->dailyForecast[0];
        const WeatherDay* tomorrow = weatherData->dailyForecast.size() > 1 ?
                                     &weatherData->dailyForecast[1] : nullptr;

        // Calculate available space for weather section
        int availableHeight = (DISPLAY_HEIGHT - STATUS_BAR_HEIGHT) - EVENTS_START_Y;
        int iconSize = 96; // Much larger icons
        int textHeight = 55; // Approximate height for 3 text lines with spacing
        int rowHeight = iconSize + textHeight + 10; // Icon + gap + text
        int totalContentHeight = tomorrow ? (rowHeight * 2 + 15) : rowHeight; // 15px spacing between rows

        // Center weather rows vertically
        int weatherStartY = (EVENTS_START_Y - 5) + (availableHeight - totalContentHeight) / 2;

        int weatherX = (WEATHER_WIDTH - iconSize) / 2; // Center horizontally in weather section

        // TODAY's weather row
        int todayY = weatherStartY;

        // Draw today's icon (96x96) - ABOVE text
        const uint8_t* weatherIcon = tempClient.getWeatherIconBitmap(today.weatherCode, true, 96);
        if (weatherIcon) {
#ifdef DISP_TYPE_6C
            display.drawInvertedBitmap(weatherX, todayY, weatherIcon, iconSize, iconSize, COLOR_WEATHER_ICON);
#else
            display.drawInvertedBitmap(weatherX, todayY, weatherIcon, iconSize, iconSize, GxEPD_BLACK);
#endif
        }

        // Text BELOW icon (3 lines, centered horizontally)
        int textStartY = todayY + iconSize + 10; // 10px gap between icon and text

        // Line 1: Day label
        display.setFont(&FONT_WEATHER_LABEL);
        display.setTextColor(GxEPD_BLACK);
        String todayStr = String(LOC_TODAY);
        int16_t textWidth = getTextWidth(todayStr, &FONT_WEATHER_LABEL);
        display.setCursor((WEATHER_WIDTH - textWidth) / 2, textStartY);
        display.print(todayStr);

        // Line 2: Rain probability
        display.setFont(&FONT_WEATHER_RAIN);
        String rainStr = String((int)today.precipitationProbability) + "%";
        textWidth = getTextWidth(rainStr, &FONT_WEATHER_RAIN);
        display.setCursor((WEATHER_WIDTH - textWidth) / 2, textStartY + 22);
        display.print(rainStr);

        // Line 3: Temperature range
        display.setFont(&FONT_WEATHER_TEMP_MAIN);
        String tempStr = String((int)today.tempMin) + "\260/" + String((int)today.tempMax) + "\260";
        textWidth = getTextWidth(tempStr, &FONT_WEATHER_TEMP_MAIN);
        display.setCursor((WEATHER_WIDTH - textWidth) / 2, textStartY + 46);
        display.print(tempStr);

        // TOMORROW's weather row (if available)
        if (tomorrow) {
            int tomorrowY = todayY + rowHeight + 5; // 5px spacing between rows

            // Draw tomorrow's icon (96x96) - ABOVE text
            weatherIcon = tempClient.getWeatherIconBitmap(tomorrow->weatherCode, true, 96);
            if (weatherIcon) {
#ifdef DISP_TYPE_6C
                display.drawInvertedBitmap(weatherX, tomorrowY, weatherIcon, iconSize, iconSize, COLOR_WEATHER_ICON);
#else
                display.drawInvertedBitmap(weatherX, tomorrowY, weatherIcon, iconSize, iconSize, GxEPD_BLACK);
#endif
            }

            // Text BELOW icon
            textStartY = tomorrowY + iconSize + 10;

            // Line 1: Day label
            display.setFont(&FONT_WEATHER_LABEL);
            String tomorrowStr = String(LOC_TOMORROW);
            textWidth = getTextWidth(tomorrowStr, &FONT_WEATHER_LABEL);
            display.setCursor((WEATHER_WIDTH - textWidth) / 2, textStartY);
            display.print(tomorrowStr);

            // Line 2: Rain probability
            display.setFont(&FONT_WEATHER_RAIN);
            rainStr = String((int)tomorrow->precipitationProbability) + "%";
            textWidth = getTextWidth(rainStr, &FONT_WEATHER_RAIN);
            display.setCursor((WEATHER_WIDTH - textWidth) / 2, textStartY + 22);
            display.print(rainStr);

            // Line 3: Temperature range
            display.setFont(&FONT_WEATHER_TEMP_MAIN);
            tempStr = String((int)tomorrow->tempMin) + "\260/" + String((int)tomorrow->tempMax) + "\260";
            textWidth = getTextWidth(tempStr, &FONT_WEATHER_TEMP_MAIN);
            display.setCursor((WEATHER_WIDTH - textWidth) / 2, textStartY + 46);
            display.print(tempStr);
        }
    } else {
        // Weather placeholder - centered in available space
        int availableHeight = (DISPLAY_HEIGHT - STATUS_BAR_HEIGHT) - EVENTS_START_Y;
        int weatherY = EVENTS_START_Y + availableHeight / 2;

        display.setFont(&FONT_WEATHER_MESSAGE);
        display.setTextColor(GxEPD_BLACK);
        String weatherStr = "Weather";
        int16_t textWidth = getTextWidth(weatherStr, &FONT_WEATHER_MESSAGE);
        display.setCursor((WEATHER_WIDTH - textWidth) / 2, weatherY - 10);
        display.print(weatherStr);

        String naStr = "N/A";
        textWidth = getTextWidth(naStr, &FONT_WEATHER_MESSAGE);
        display.setCursor((WEATHER_WIDTH - textWidth) / 2, weatherY + 15);
        display.print(naStr);
    }

    // Right side: Events list
    int eventsX = EVENTS_START_X;
    int eventsY = EVENTS_START_Y + 12; // More space from separator (was +10)
    const int maxY = DISPLAY_HEIGHT - STATUS_BAR_HEIGHT - 10; // Stop before status bar

    if (events.empty()) {
        // No events message
        String noEventsText = String(LOC_NO_EVENTS);
        int16_t textWidth = getTextWidth(noEventsText, &FONT_NO_EVENTS);

        display.setFont(&FONT_NO_EVENTS);
        display.setTextColor(GxEPD_BLACK);

        int centerX = EVENTS_START_X + (DISPLAY_WIDTH - EVENTS_START_X) / 2;
        int centerY = EVENTS_START_Y + 100;

        display.setCursor(centerX - textWidth / 2, centerY);
        display.print(noEventsText);
    } else {
        // Draw events list
        String currentDateHeader = "";
        int eventsShown = 0;
        const int maxEvents = 7; // Limit events in portrait mode

        // Get current date info for date formatting
        time_t now;
        time(&now);
        struct tm* timeinfo = localtime(&now);
        int currentYear = timeinfo->tm_year + 1900;
        int currentMonth = timeinfo->tm_mon + 1;
        int currentDay = timeinfo->tm_mday;

        for (size_t i = 0; i < events.size() && eventsShown < maxEvents && eventsY < maxY; i++) {
            CalendarEvent* event = events[i];

            // Format date header
            String dateHeader = formatEventDate(event->date, currentYear, currentMonth, currentDay);

            // Draw date header if it changed
            if (dateHeader != currentDateHeader) {
                currentDateHeader = dateHeader;

                if (eventsY > EVENTS_START_Y + 10) {
                    eventsY += 8; // Add spacing between date groups
                }

                display.setFont(&FONT_EVENT_DATE_HEADER);

#ifdef DISP_TYPE_6C
                // Color code headers
                if (dateHeader == String(LOC_TODAY)) {
                    display.setTextColor(COLOR_EVENT_TODAY_HEADER);
                } else if (dateHeader == String(LOC_TOMORROW)) {
                    display.setTextColor(COLOR_EVENT_TOMORROW_HEADER);
                } else {
                    display.setTextColor(COLOR_EVENT_OTHER_HEADER);
                }
#else
                display.setTextColor(GxEPD_BLACK);
#endif

                display.setCursor(eventsX, eventsY);
                display.print(dateHeader);
                display.setTextColor(GxEPD_BLACK);

                eventsY += getFontHeight(&FONT_EVENT_DATE_HEADER);
            }

            // Draw event time and title
            display.setFont(&FONT_EVENT_TIME);
            String eventTime = event->allDay ? "--" : event->getStartTimeStr();
            display.setCursor(eventsX, eventsY);
            display.print(eventTime);

            // Draw event title next to time
            display.setFont(&FONT_EVENT_TITLE);
            int titleX = eventsX + 50; // Space for time
            int titleMaxWidth = DISPLAY_WIDTH - titleX - 20;
            String title = StringUtils::truncate(StringUtils::removeAccents(event->title), titleMaxWidth, "...");
            display.setCursor(titleX, eventsY);
            display.print(title);

            eventsY += getFontHeight(&FONT_EVENT_TITLE) + 4;
            eventsShown++;
        }

        // Show "more events" indicator if needed
        if (events.size() > maxEvents) {
            display.setFont(&FONT_EVENT_TITLE);
            display.setCursor(eventsX, eventsY);
            display.print("+" + String(events.size() - maxEvents) + " more...");
        }
    }
}

// ============================================================================
// Status Bar
// ============================================================================

void DisplayManager::drawPortraitStatusBar(bool wifiConnected, int rssi,
    float batteryVoltage, int batteryPercentage, time_t now, bool isStale)
{
    // Use 6pt font for compact status bar
    display.setFont(&FONT_STATUSBAR);
    display.setTextColor(GxEPD_BLACK);

    // Position close to bottom of screen
    int iconSize = 16;
    int iconY = DISPLAY_HEIGHT - iconSize + 2;
    int textY = DISPLAY_HEIGHT - 4; // Text baseline close to bottom

    // LEFT SIDE: Battery icon and percentage
    int leftX = 5;

    // Draw battery icon
    if (batteryPercentage > 90) {
        display.drawInvertedBitmap(leftX, iconY, battery_full_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else if (batteryPercentage > 75) {
        display.drawInvertedBitmap(leftX, iconY, battery_6_bar_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else if (batteryPercentage > 60) {
        display.drawInvertedBitmap(leftX, iconY, battery_5_bar_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else if (batteryPercentage > 45) {
        display.drawInvertedBitmap(leftX, iconY, battery_4_bar_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else if (batteryPercentage > 30) {
        display.drawInvertedBitmap(leftX, iconY, battery_3_bar_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else if (batteryPercentage > 15) {
        display.drawInvertedBitmap(leftX, iconY, battery_2_bar_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else if (batteryPercentage > 5) {
        display.drawInvertedBitmap(leftX, iconY, battery_1_bar_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    } else {
        display.drawInvertedBitmap(leftX, iconY, battery_alert_90deg_16x16, iconSize, iconSize, GxEPD_BLACK);
    }

    // Draw battery percentage text
    display.setCursor(leftX + iconSize + 3, textY);
    display.print(String(batteryPercentage) + "%");

    // CENTER: Last update date and time
    String dateTimeStr = DateUtils::formatDate(now) + " " + DateUtils::formatTime(now);
    // String dateTimeStr = String(currentDay) + "/" + String(currentMonth) + " " + currentTime;
    if (isStale) {
        dateTimeStr = "[!] " + dateTimeStr;
    }
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dateTimeStr, 0, 0, &x1, &y1, &w, &h);
    int centerX = (DISPLAY_WIDTH - w) / 2;
    display.setCursor(centerX, textY);
    display.print(dateTimeStr);

    // RIGHT SIDE: WiFi icon and signal strength
    // Calculate RSSI text width to position icon correctly
    String rssiText = String(rssi) + "dBm";
    display.getTextBounds(rssiText, 0, 0, &x1, &y1, &w, &h);
    int rssiTextWidth = w;

    // Position WiFi icon and text from right edge
    int rightX = DISPLAY_WIDTH - rssiTextWidth - iconSize - 8; // 5px margin from right

    // Draw WiFi icon
    iconY -= 2;
    if (wifiConnected) {
        if (rssi > -60) {
            display.drawInvertedBitmap(rightX, iconY, wifi_3_bar_16x16, iconSize, iconSize, GxEPD_BLACK);
        } else if (rssi > -75) {
            display.drawInvertedBitmap(rightX, iconY, wifi_2_bar_16x16, iconSize, iconSize, GxEPD_BLACK);
        } else {
            display.drawInvertedBitmap(rightX, iconY, wifi_1_bar_16x16, iconSize, iconSize, GxEPD_BLACK);
        }
    } else {
        display.drawInvertedBitmap(rightX, iconY, wifi_off_16x16, iconSize, iconSize, GxEPD_BLACK);
    }

    // Draw RSSI text
    display.setCursor(rightX + iconSize + 3, textY);
    display.print(rssiText);
}

#endif // DISPLAY_ORIENTATION == PORTRAIT
