#include "display_manager.h"
#include "localization.h"
#include "version.h"
#include "debug_config.h"
#include "string_utils.h"
#include <SPI.h>
#include <assets/fonts.h>
#include <assets/icons/icons.h>
#include <assets/icons/icons_96x96.h>
#include <assets/icons/icons_64x64.h>
#include <assets/icons/icons_48x48.h>
#include <assets/icons/icons_32x32.h>
#include <assets/icons/icons_16x16.h>

DisplayManager::DisplayManager()
    : display(GxEPD2_DRIVER_CLASS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY))
{
}

void DisplayManager::init()
{
    DEBUG_INFO_PRINTLN("Initializing display...");

    DEBUG_INFO_PRINTLN("Configuring SPI for EPD...");
    DEBUG_INFO_PRINTF("SCK: %d, MOSI: %d, CS: %d\n", EPD_SCK, EPD_MOSI, EPD_CS);
    // SPI.end();
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS); // remap SPI for EPD, -1 for MISO (not used)

    // display.init(115200);
    display.init(115200, true, 2, false); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
    DEBUG_INFO_PRINTLN("Display initialized!");
}

void DisplayManager::clear()
{
    DEBUG_VERBOSE_PRINTLN("Clearing display...");
    // display.fillScreen(GxEPD_WHITE);
    display.clearScreen();
}

uint16_t DisplayManager::pages()
{
    return display.pages();
}

uint16_t DisplayManager::pageHeight()
{
    return display.pageHeight();
}

void DisplayManager::setRotation(uint8_t rotation)
{
    display.setRotation(rotation);
}

void DisplayManager::setFont(const GFXfont *f)
{
    display.setFont(f);
}

int16_t DisplayManager::width(void)
{
    return display.width();
}

int16_t DisplayManager::height(void)
{
    return display.height();
}

void DisplayManager::setFullWindow()
{
    display.setFullWindow();
}

void DisplayManager::fillScreen(uint16_t color)
{
    display.fillScreen(color);
}

void DisplayManager::setCursor(int16_t x, int16_t y)
{
    display.setCursor(x, y);
}

size_t DisplayManager::print(const String& s)
{
    return display.print(s);
}

size_t DisplayManager::print(const char str[])
{
    return display.print(str);
}

size_t DisplayManager::print(char c)
{
    return display.print(c);
}

size_t DisplayManager::print(const Printable& x)
{
    return display.print(x);
}

void DisplayManager::firstPage()
{
    display.firstPage();
}

void DisplayManager::setTextColor(uint16_t c)
{
    display.setTextColor(c);
}

void DisplayManager::getTextBounds(const char* string, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h)
{
    display.getTextBounds(string, x, y, x1, y1, w, h);
}

void DisplayManager::getTextBounds(const __FlashStringHelper* s, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h)
{
    display.getTextBounds(s, x, y, x1, y1, w, h);
}

void DisplayManager::getTextBounds(const String& str, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h)
{
    display.getTextBounds(str, x, y, x1, y1, w, h);
}

void DisplayManager::displayScreen()
{
    DEBUG_VERBOSE_PRINTLN("Refreshing display...");
    display.display();
}

void DisplayManager::refresh(bool partial_update_mode)
{
    DEBUG_VERBOSE_PRINTLN("Refreshing display...");
    display.refresh(partial_update_mode);
}

bool DisplayManager::nextPage()
{
    return display.nextPage();
}

void DisplayManager::drawHeader(const String& currentDate, const String& currentTime)
{
    // Draw header with date and time
    display.setFont(&FONT_HEADER_MONTH_YEAR);
    display.setCursor(20, 35);
    display.print(currentDate);

    display.setFont(&FONT_EVENT_TITLE);
    display.setCursor(DISPLAY_WIDTH - 150, 35);
    display.print(currentTime);

    // Draw horizontal line under header
    display.drawLine(0, HEADER_HEIGHT, DISPLAY_WIDTH, HEADER_HEIGHT, GxEPD_BLACK);
    display.drawLine(0, HEADER_HEIGHT + 1, DISPLAY_WIDTH, HEADER_HEIGHT + 1, GxEPD_BLACK);
}

void DisplayManager::drawModernHeader(int currentDay, const String& monthYear, const String& currentTime, const WeatherData* weatherData)
{
    // Header for left side only (within 400px width) - centered
    // Get day string
    String dayStr = (currentDay > 0 && currentDay <= 31) ? String(currentDay) : "--";

    // Draw sunrise and sunset at corners if weather data is available
    if (weatherData && !weatherData->dailyForecast.empty()) {
        const WeatherDay& today = weatherData->dailyForecast[0];
        display.setFont(&FONT_SUNRISE_SUNSET);
        display.setTextColor(GxEPD_BLACK);  // Ensure text is black

        // Draw sunrise at top-left corner with icon
        if (today.sunrise.length() >= 16) {
            String sunriseTime = today.sunrise.substring(11, 16);  // Extract HH:MM
            // Draw sunrise icon
            display.drawInvertedBitmap(10, 10, wi_sunrise_16x16, 16, 16, GxEPD_BLACK);
            // Draw sunrise time next to icon
            display.setCursor(30, 20);  // Position text next to icon
            display.print(sunriseTime);
        }

        // Draw sunset at top-right corner with icon
        if (today.sunset.length() >= 16) {
            String sunsetTime = today.sunset.substring(11, 16);  // Extract HH:MM
            int16_t textWidth = getTextWidth(sunsetTime, &FONT_SUNRISE_SUNSET);
            // Calculate position for right alignment
            int iconX = LEFT_WIDTH - textWidth - 30;  // Account for icon and text width
            // Draw sunset icon
            display.drawInvertedBitmap(iconX, 10, wi_sunset_16x16, 16, 16, GxEPD_BLACK);
            // Draw sunset time next to icon
            display.setCursor(iconX + 20, 20);  // Position text next to icon
            display.print(sunsetTime);
        }
    }

    // Use font metrics for accurate centering
    int16_t dayWidth = getTextWidth(dayStr, &FONT_HEADER_DAY_NUMBER);
    int16_t monthYearWidth = getTextWidth(monthYear, &FONT_HEADER_MONTH_YEAR);

    // Calculate vertical positions using font metrics
    int16_t dayBaseline = getFontBaseline(&FONT_HEADER_DAY_NUMBER);
    int16_t dayY = 4 + dayBaseline;  // Moved up 6 more pixels (was 10, now 4)

    // Center and draw the day number
    display.setFont(&FONT_HEADER_DAY_NUMBER);
    int dayX = (LEFT_WIDTH - dayWidth) / 2;
    display.setCursor(dayX, dayY);

#ifdef DISP_TYPE_6C
    // Use configured color for day number on 6-color displays
    display.setTextColor(COLOR_HEADER_DAY_NUMBER);
    display.print(dayStr);
    display.setTextColor(GxEPD_BLACK); // Reset to black for other text
#else
    display.print(dayStr);
#endif

    // Calculate position for month/year - moved up 40px total
    // Instead of using calculateYPosition, manually position for exact control
    int16_t monthYearY = dayY + getFontHeight(&FONT_HEADER_DAY_NUMBER) - 30;  // Moved up significantly

    // Center and draw the month and year
    display.setFont(&FONT_HEADER_MONTH_YEAR);
    int monthYearX = (LEFT_WIDTH - monthYearWidth) / 2;
    display.setCursor(monthYearX, monthYearY);
    display.print(monthYear);

    // Position separator using font metrics
    int16_t separatorY = calculateYPosition(monthYearY, &FONT_HEADER_MONTH_YEAR, 15);
    display.drawLine(10, separatorY, LEFT_WIDTH - 10, separatorY, GxEPD_BLACK);
}

void DisplayManager::drawCompactCalendar(const MonthCalendar& calendar,
                                        const std::vector<CalendarEvent*>& events)
{
    // Draw calendar on left side only
    int startX = CALENDAR_MARGIN;
    int startY = CALENDAR_START_Y;
    int cellWidth = (LEFT_WIDTH - (2 * CALENDAR_MARGIN)) / 7; // Fit within left side

    // Draw day labels with Luna ITC Bold using localized strings
    display.setFont(&FONT_CALENDAR_DAY_LABELS);

#ifdef DISP_TYPE_6C
    // Use configured color for day labels on 6-color displays
    display.setTextColor(COLOR_CALENDAR_DAY_LABELS);
#endif

    // Use localized day names, taking first character
    for (int i = 0; i < 7; i++) {
        // Calculate actual day index based on FIRST_DAY_OF_WEEK setting
        int dayIndex = (i + FIRST_DAY_OF_WEEK) % 7;

        // Get first character of localized day name
        String shortDay = String(DAY_NAMES_SHORT[dayIndex]).substring(0, 1);

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(shortDay, 0, 0, &x1, &y1, &w, &h);
        int x = startX + (i * cellWidth) + (cellWidth - w) / 2;
        display.setCursor(x, startY + 15);
        display.print(shortDay);
    }

#ifdef DISP_TYPE_6C
    // Reset to black after drawing day labels
    display.setTextColor(GxEPD_BLACK);
#endif

    // Draw calendar days in a clean grid
    startY += DAY_LABEL_HEIGHT + 10;

    // Calculate first day position
    int row = 0;
    int col = 0;

    // Draw previous month days in gray (with dithering)
    int prevMonth = calendar.month - 1;
    int prevYear = calendar.year;
    if (prevMonth < 0) {
        prevMonth = 11;
        prevYear--;
    }
    int daysInPrevMonth = getDaysInMonth(prevYear, prevMonth);
    int prevMonthDay = daysInPrevMonth - calendar.firstDayOfWeek + 1;

    for (col = 0; col < calendar.firstDayOfWeek; col++) {
        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        // Previous month days without background
        display.setFont(&FONT_CALENDAR_OUTSIDE_MONTH);
        String dayStr = String(prevMonthDay);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

#ifdef DISP_TYPE_6C
        // Use green for days outside current month on 6-color displays
        display.setTextColor(COLOR_CALENDAR_OUTSIDE_MONTH);
        display.print(dayStr);
        display.setTextColor(GxEPD_BLACK); // Reset to black
#else
        display.print(dayStr);
#endif

        // Check if this day in the previous month has an event
        bool hasEvent = false;
        for (auto event : events) {
            if (event->date.length() >= 10) {
                int eventYear = event->date.substring(0, 4).toInt();
                int eventMonth = event->date.substring(5, 7).toInt();
                int eventDay = event->date.substring(8, 10).toInt();

                // Month is 1-based in the date string, prevMonth is 0-based
                if (eventYear == prevYear && eventMonth == (prevMonth + 1) && eventDay == prevMonthDay) {
                    hasEvent = true;
                    break;
                }
            }
        }

        // Draw event indicator if there's an event
        if (hasEvent) {
#ifdef DISP_TYPE_6C
            // On 6-color displays, use a colored indicator
            display.fillCircle(x + cellWidth / 2, y + 32, 3, COLOR_CALENDAR_OUTSIDE_MONTH);
#else
            // On BW displays, draw a black circle
            display.fillCircle(x + cellWidth / 2, y + 32, 3, GxEPD_BLACK);
#endif
        }

        prevMonthDay++;
    }

    // Draw current month days with bold font
    display.setFont(&FONT_CALENDAR_DAY_NUMBERS);
    int currentDay = 1;
    col = calendar.firstDayOfWeek;

    while (currentDay <= calendar.daysInMonth) {
        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        // Highlight weekends
        bool isWeekend = false;
        if (FIRST_DAY_OF_WEEK == 0) {
            // Sunday first: Sunday=0, Saturday=6
            isWeekend = (col == 0 || col == 6);
        } else {
            // Monday first: Saturday=5, Sunday=6
            isWeekend = (col == 5 || col == 6);
        }
        if (isWeekend) {
#ifdef DISP_TYPE_6C
            // Use the new dithered rectangle method with config-defined dithering level
            drawDitheredRectangle(x, y, cellWidth, CELL_HEIGHT,
                                GxEPD_WHITE, COLOR_CALENDAR_WEEKEND_BG,
                                static_cast<DitherLevel>(DITHER_CALENDAR_WEEKEND));
#else
            // Use dithering for B/W displays
            drawDitheredRectangle(x, y, cellWidth, CELL_HEIGHT,
                                GxEPD_WHITE, GxEPD_BLACK, DitherLevel::DITHER_10);
#endif
        }

        // Highlight today with a border outline
        bool isToday = (currentDay == calendar.today);
        if (isToday) {
#ifdef DISP_TYPE_6C
            // Draw a colored border for today on 6-color displays
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, COLOR_CALENDAR_TODAY_BORDER);
            display.drawRect(x + 2, y + 2, cellWidth - 4, CELL_HEIGHT - 4, COLOR_CALENDAR_TODAY_BORDER);
#else
            // Draw a black border for today on B/W displays
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, GxEPD_BLACK);
            display.drawRect(x + 2, y + 2, cellWidth - 4, CELL_HEIGHT - 4, GxEPD_BLACK);
#endif
        }

        // Draw day number with bold font
        String dayStr = String(currentDay);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

#ifdef DISP_TYPE_6C
        if (isToday) {
            // Use configured color for current day on 6-color displays
            display.setTextColor(COLOR_CALENDAR_TODAY_TEXT);
            display.print(dayStr);
            display.setTextColor(GxEPD_BLACK); // Reset to black
        } else {
            display.print(dayStr);
        }
#else
        display.print(dayStr);
#endif

        // Draw event indicator(s) below day number if needed
        if (calendar.hasEvent[currentDay]) {
#ifdef DISP_TYPE_6C
            // On 6-color displays, draw colored circles based on calendar colors (max 3)
            int circleCount = 0;
            int circleRadius = 3;
            int circleSpacing = 8; // Space between circles

            // Count how many calendar colors we have for this day
            for (int i = 0; i < MAX_CALENDARS; i++) {
                if (!calendar.eventColors[currentDay][i].isEmpty()) {
                    circleCount++;
                }
            }

            if (circleCount > 0) {
                // Calculate starting X position to center the circles
                int totalWidth = (circleCount * circleRadius * 2) + ((circleCount - 1) * circleSpacing);
                int startX = x + cellWidth / 2 - totalWidth / 2 + circleRadius;

                // Draw each circle with its calendar color
                for (int i = 0; i < circleCount; i++) {
                    String colorStr = calendar.eventColors[currentDay][i];
                    uint16_t circleColor = GxEPD_BLACK; // Default

                    // Map color strings to display colors
                    if (colorStr == "red") {
                        circleColor = GxEPD_RED;
                    } else if (colorStr == "orange") {
                        circleColor = GxEPD_ORANGE;
                    } else if (colorStr == "yellow") {
                        circleColor = GxEPD_YELLOW;
                    } else if (colorStr == "green") {
                        circleColor = GxEPD_GREEN;
                    } else if (colorStr == "blue") {
                        circleColor = GxEPD_BLACK; // 6-color displays don't have blue, use black
                    } else if (colorStr == "black" || colorStr == "default") {
                        circleColor = GxEPD_BLACK;
                    }

                    display.fillCircle(startX + (i * (circleRadius * 2 + circleSpacing)), y + 32, circleRadius, circleColor);
                }
            }
#else
            // On BW displays, draw a single black circle
            display.fillCircle(x + cellWidth / 2, y + 32, 3, GxEPD_BLACK);
#endif
        }

        currentDay++;
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    // Draw remaining days from next month
    int nextMonth = calendar.month + 1;
    int nextYear = calendar.year;
    if (nextMonth > 12) {
        nextMonth = 1;
        nextYear++;
    }

    int nextMonthDay = 1;
    while (row < 6 || (row == 5 && col > 0)) {
        if (col >= 7) {
            col = 0;
            row++;
            if (row >= 6)
                break;
        }

        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        // Next month days without background
        display.setFont(&FONT_CALENDAR_OUTSIDE_MONTH);
        String dayStr = String(nextMonthDay);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

#ifdef DISP_TYPE_6C
        // Use yellow for days outside current month on 6-color displays
        display.setTextColor(COLOR_CALENDAR_OUTSIDE_MONTH);
        display.print(dayStr);
        display.setTextColor(GxEPD_BLACK); // Reset to black
#else
        display.print(dayStr);
#endif

        // Check if this day in the next month has an event
        bool hasEvent = false;
        for (auto event : events) {
            if (event->date.length() >= 10) {
                int eventYear = event->date.substring(0, 4).toInt();
                int eventMonth = event->date.substring(5, 7).toInt();
                int eventDay = event->date.substring(8, 10).toInt();

                if (eventYear == nextYear && eventMonth == nextMonth && eventDay == nextMonthDay) {
                    hasEvent = true;
                    break;
                }
            }
        }

        // Draw event indicator if there's an event
        if (hasEvent) {
#ifdef DISP_TYPE_6C
            // On 6-color displays, use a colored indicator
            display.fillCircle(x + cellWidth / 2, y + 32, 3, COLOR_CALENDAR_OUTSIDE_MONTH);
#else
            // On BW displays, draw a black circle
            display.fillCircle(x + cellWidth / 2, y + 32, 3, GxEPD_BLACK);
#endif
        }

        nextMonthDay++;
        col++;
    }
}

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

        // Event title with smaller font
        display.setFont(&FONT_EVENT_TITLE);
        String title = StringUtils::convertAccents(event->title);
        int eventTextX = x + timeColumnWidth; // Reduced spacing
        int maxPixelWidth = DISPLAY_WIDTH - eventTextX - 10;

        // Improved text wrapping with word boundaries
        int16_t x1, y1;
        uint16_t w, h;

        // Find optimal break point considering word boundaries
        String line1 = "";
        int lastSpaceIdx = -1;
        int maxChars = 0;

        for (int i = 0; i < title.length(); i++) {
            if (title[i] == ' ') lastSpaceIdx = i;

            String testStr = title.substring(0, i + 1);
            display.getTextBounds(testStr, 0, 0, &x1, &y1, &w, &h);

            if (w > maxPixelWidth) {
                // Try to break at last space if available
                if (lastSpaceIdx > 0 && lastSpaceIdx > maxChars - 10) {
                    line1 = title.substring(0, lastSpaceIdx);
                } else {
                    line1 = title.substring(0, i);
                }
                break;
            }
            maxChars = i + 1;
            line1 = testStr;
        }

        // Draw first line
        display.setCursor(eventTextX, y);
        display.print(line1);

        // Handle second line if needed
        bool hasSecondLine = false;
        if (title.length() > line1.length()) {
            String remaining = title.substring(line1.length());
            remaining.trim();

            if (remaining.length() > 0 && y + 13 <= maxY) { // Check space for second line
                hasSecondLine = true;

                // Smart truncation for second line
                String line2 = "";
                for (int i = 0; i < remaining.length(); i++) {
                    String testStr = remaining.substring(0, i + 1);
                    display.getTextBounds(testStr + "...", 0, 0, &x1, &y1, &w, &h);

                    if (w > maxPixelWidth) {
                        // Need to truncate
                        if (i > 0) {
                            // Try to break at word boundary
                            int breakPoint = i - 1;
                            while (breakPoint > 0 && remaining[breakPoint] != ' ') {
                                breakPoint--;
                            }
                            if (breakPoint > i - 8) { // Don't go back too far
                                line2 = remaining.substring(0, breakPoint) + "...";
                            } else {
                                line2 = remaining.substring(0, i - 1) + "...";
                            }
                        }
                        break;
                    }
                    line2 = testStr;
                }

                // Draw second line
                display.setCursor(eventTextX, y + 13);  // Reduced line spacing
                display.print(line2);
            }
        }

        // Adjust spacing based on whether we had a second line
        if (hasSecondLine) {
            y += 33;  // 20px base + 13px for second line
        } else {
            y += 20;  // Just base spacing for single line
        }

        eventCount++;
    }
}

void DisplayManager::drawWeatherPlaceholder()
{
    // Draw placeholder for weather forecast in bottom right
    int x = RIGHT_START_X + 20;
    int y = WEATHER_START_Y + 6;  // Move down 6 pixels

    // Display placeholder weather icon (48x48)
#ifdef DISP_TYPE_6C
    // Use red for weather icon on 6-color displays
    display.drawInvertedBitmap(x + 10, y - 20, wi_na_48x48, 48, 48, COLOR_WEATHER_ICON);
#else
    display.drawInvertedBitmap(x + 10, y - 20, wi_na_48x48, 48, 48, GxEPD_BLACK);
#endif

    // Display placeholder temperatures (moved down and adjusted for 48px icon)
    display.setFont(&FONT_WEATHER_TEMP_MAIN);
    display.setCursor(x + 70, y + 15);  // Moved closer to icon and down
    display.print("--\260 / --\260");

    y += 40;  // Reduced spacing after removing sunrise/sunset

    // Placeholder text
    display.setFont(&FONT_WEATHER_MESSAGE);
    display.setCursor(x, y + 20);
    display.print(LOC_WEATHER_COMING_SOON);
}

void DisplayManager::drawWeatherForecast(const WeatherData& weatherData)
{
    // Draw weather forecast at bottom of right section
    int x = RIGHT_START_X + 20;  // Align with events section
    int y = WEATHER_START_Y + 6;  // Move down 6 pixels

    // Display today's weather icon and min/max temperatures
    if (!weatherData.dailyForecast.empty()) {
        const WeatherDay& today = weatherData.dailyForecast[0];

        // Draw weather icon for today (48x48)
        WeatherClient tempClient(nullptr);
        const uint8_t* todayIcon = tempClient.getWeatherIconBitmap(today.weatherCode, true, 48);
        if (todayIcon) {
            // Position icon
            int iconX = x;  // Slightly offset from left
            int iconY = y - 20;  // Adjusted for 48px icon
#ifdef DISP_TYPE_6C
            // Use red for weather icon on 6-color displays
            display.drawInvertedBitmap(iconX, iconY, todayIcon, 48, 48, COLOR_WEATHER_ICON);
#else
            display.drawInvertedBitmap(iconX, iconY, todayIcon, 48, 48, GxEPD_BLACK);
#endif
        }

        // Display min/max temperatures to the right of icon (moved down)
        display.setFont(&FONT_WEATHER_TEMP_MAIN);
        String tempRange = String(int(today.tempMin)) + "\260 / " + String(int(today.tempMax)) + "\260";
        display.setCursor(x + 70, y + 15);  // Moved closer to icon and down
        display.print(tempRange);
    }

    y += 40;  // Reduced spacing after removing sunrise/sunset

    // Check if we have hourly data
    if (weatherData.hourlyForecast.empty()) {
        display.setFont(&FONT_WEATHER_MESSAGE);
        display.setCursor(x, y + 20);
        display.print(LOC_NO_WEATHER_DATA);
        return;
    }

    // Draw 7 forecasts in a single line (covering 21 hours, one every 3 hours)
    // Each item is approximately 54 pixels wide to fit 7 items
    int itemWidth = 54;
    int itemSpacing = 0;
    int iconSize = 32;  // Larger 32x32 icons for better visibility
    int startX = x - 10;  // Adjust to use full width

    for (size_t i = 0; i < weatherData.hourlyForecast.size() && i < 7; i++) {
        const WeatherHour& hour = weatherData.hourlyForecast[i];

        // Calculate position (single row)
        int itemX = startX + (i * (itemWidth + itemSpacing));
        int itemY = y;

        // Extract hour from time string (format: 2025-10-16T11:00)
        String hourStr = hour.time.substring(11, 13);

        // Display hour (smaller font)
        display.setFont(nullptr); // Use default small font
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(hourStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(itemX + (itemWidth - w) / 2, itemY);
        display.print(hourStr);

        // Draw weather icon (centered)
        int iconX = itemX + (itemWidth - iconSize) / 2;
        int iconY = itemY + 8;

        // Get the appropriate weather icon bitmap
        const uint8_t* iconBitmap = nullptr;
        WeatherClient tempClient(nullptr); // Temporary client just for icon access
        iconBitmap = tempClient.getWeatherIconBitmap(hour.weatherCode, hour.isDay, iconSize);

        if (iconBitmap) {
            display.drawInvertedBitmap(iconX, iconY, iconBitmap, iconSize, iconSize, GxEPD_BLACK);
        }

        // Display temperature (smaller font)
        display.setFont(&FONT_WEATHER_TEMP_HOURLY);
        String tempStr = String(int(hour.temperature)) + "\260";
        display.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(itemX + (itemWidth - w) / 2, iconY + iconSize + 15);
        display.print(tempStr);

        // Precipitation percentage removed to save space
    }
}

void DisplayManager::drawDivider()
{
    // Draw thin vertical line between left and right sides
    // Stop before the bottom status bar area
    display.drawLine(SPLIT_X, 0, SPLIT_X, DISPLAY_HEIGHT - 40, GxEPD_BLACK);
}

void DisplayManager::drawMonthCalendar(const MonthCalendar& calendar, int x, int y,
                                      const std::vector<CalendarEvent*>& events)
{
    // Local constants for old layout compatibility
    const int CALENDAR_WIDTH = 400;
    const int MONTH_HEADER_HEIGHT = 40;

    // Draw month and year header
    display.setFont(&FONT_EVENT_TITLE);
    String monthYear = String(MONTH_NAMES[calendar.month]) + " " + String(calendar.year);
    centerText(monthYear, x, y + 25, CALENDAR_WIDTH - 20, &FONT_EVENT_TITLE);

    // Draw day labels (moved 20 pixels lower)
    drawDayLabels(x, y + MONTH_HEADER_HEIGHT);

    // Draw calendar grid (adjusted for moved labels)
    drawCalendarGrid(x, y + MONTH_HEADER_HEIGHT + 45);

    // Calculate days in previous month
    int prevMonth = calendar.month - 1;
    int prevYear = calendar.year;
    if (prevMonth < 0) {
        prevMonth = 11;
        prevYear--;
    }
    int daysInPrevMonth = getDaysInMonth(prevYear, prevMonth);

    // Draw previous month days (in gray)
    int row = 0;
    int col = 0;
    int prevMonthDay = daysInPrevMonth - calendar.firstDayOfWeek + 1;

    for (col = 0; col < calendar.firstDayOfWeek; col++) {
        // Check if this day in the previous month has an event
        bool hasEvent = false;
        for (auto event : events) {
            if (event->date.length() >= 10) {
                int eventYear = event->date.substring(0, 4).toInt();
                int eventMonth = event->date.substring(5, 7).toInt();
                int eventDay = event->date.substring(8, 10).toInt();

                // Month is 1-based in the date string
                if (eventYear == prevYear && eventMonth == (prevMonth + 1) && eventDay == prevMonthDay) {
                    hasEvent = true;
                    break;
                }
            }
        }

        drawPreviousNextMonthDay(prevMonthDay, col, row, x, y + MONTH_HEADER_HEIGHT + 45,
                               prevMonth + 1, prevYear, hasEvent);
        prevMonthDay++;
    }

    // Draw current month days
    int currentDay = 1;
    col = calendar.firstDayOfWeek;

    while (currentDay <= calendar.daysInMonth) {
        bool hasEvent = calendar.hasEvent[currentDay];
        bool isToday = (currentDay == calendar.today);

        drawCalendarDay(currentDay, col, row, x, y + MONTH_HEADER_HEIGHT + 45,
            hasEvent, isToday);

        currentDay++;
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    // Draw next month days (in gray) to fill remaining cells
    int nextMonth = calendar.month + 1;
    int nextYear = calendar.year;
    if (nextMonth > 12) {
        nextMonth = 1;
        nextYear++;
    }

    int nextMonthDay = 1;
    while (row < 6) { // Always show 6 rows for consistency
        while (col < 7) {
            // Check if this day in the next month has an event
            bool hasEvent = false;
            for (auto event : events) {
                if (event->date.length() >= 10) {
                    int eventYear = event->date.substring(0, 4).toInt();
                    int eventMonth = event->date.substring(5, 7).toInt();
                    int eventDay = event->date.substring(8, 10).toInt();

                    if (eventYear == nextYear && eventMonth == nextMonth && eventDay == nextMonthDay) {
                        hasEvent = true;
                        break;
                    }
                }
            }

            drawPreviousNextMonthDay(nextMonthDay, col, row, x, y + MONTH_HEADER_HEIGHT + 45,
                                   nextMonth, nextYear, hasEvent);
            nextMonthDay++;
            col++;
        }
        col = 0;
        row++;
    }
}

void DisplayManager::drawDayLabels(int x, int y)
{
    // Local constants for old layout compatibility
    const int OLD_OLD_CELL_WIDTH = 54;

    display.setFont(&FONT_EVENT_TITLE); // Using event font for day labels

    for (int i = 0; i < 7; i++) {
        // Adjust day index based on first day of week setting
        int dayIndex = (i + FIRST_DAY_OF_WEEK) % 7;
        int dayX = x + (i * OLD_OLD_CELL_WIDTH) + (OLD_OLD_CELL_WIDTH / 2) - 12;
        display.setCursor(dayX, y + 20); // Moved 20 pixels lower total
        display.print(DAY_NAMES_SHORT[dayIndex]);
    }
}

void DisplayManager::drawCalendarGrid(int x, int y)
{
    // Local constants for old layout compatibility
    const int OLD_CELL_WIDTH = 54;
    const int OLD_CELL_HEIGHT = 50;

    // Draw horizontal lines (6 rows max for any month)
    for (int row = 0; row <= 6; row++) {
        int lineY = y + (row * OLD_CELL_HEIGHT);
        display.drawLine(x, lineY, x + (7 * OLD_CELL_WIDTH), lineY, GxEPD_BLACK);
    }

    // Draw vertical lines (7 columns for days)
    for (int col = 0; col <= 7; col++) {
        int lineX = x + (col * OLD_CELL_WIDTH);
        display.drawLine(lineX, y, lineX, y + (6 * OLD_CELL_HEIGHT), GxEPD_BLACK);
    }
}

void DisplayManager::drawCalendarDay(int day, int col, int row, int x, int y,
    bool hasEvent, bool isToday)
{
    // Local constants for old layout compatibility
    const int OLD_CELL_WIDTH = 54;
    const int OLD_CELL_HEIGHT = 50;

    int cellX = x + (col * OLD_CELL_WIDTH);
    int cellY = y + (row * OLD_CELL_HEIGHT);

    // Apply Floyd-Steinberg dithering for weekend cells (25% gray)
    // Calculate actual day of week considering FIRST_DAY_OF_WEEK setting
    int actualDayOfWeek = (col + FIRST_DAY_OF_WEEK) % 7;
    bool isWeekend = (actualDayOfWeek == 0 || actualDayOfWeek == 6); // Sunday = 0, Saturday = 6
    if (isWeekend && !isToday) {
        // Apply dithering pattern for weekend cells
        drawDitheredRectangle(cellX + 1, cellY + 1, OLD_CELL_WIDTH - 2, OLD_CELL_HEIGHT - 2,
                            GxEPD_WHITE, GxEPD_BLACK, DitherLevel::DITHER_25);
    }

    // Highlight today with a border outline
    if (isToday) {
        // Draw a 2-pixel thick border for today
        display.drawRect(cellX + 1, cellY + 1, OLD_CELL_WIDTH - 2, OLD_CELL_HEIGHT - 2, GxEPD_BLACK);
        display.drawRect(cellX + 2, cellY + 2, OLD_CELL_WIDTH - 4, OLD_CELL_HEIGHT - 4, GxEPD_BLACK);
    }

    // Draw day number centered in cell
    display.setFont(&FONT_CALENDAR_DAY_NUMBERS);
    String dayStr = String(day);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);

    // Center horizontally and vertically
    // Note: y1 is typically negative for the ascent, so we use -y1 for proper centering
    int textX = cellX + (OLD_CELL_WIDTH - w) / 2;
    int textY = cellY + (OLD_CELL_HEIGHT - y1) / 2; // Using -y1 as it represents the ascent

    display.setCursor(textX, textY);
    display.print(dayStr);

    // Draw event indicator (small triangle in top-right corner)
    if (hasEvent) {
        int triangleSize = 8;
        int triangleX = cellX + OLD_CELL_WIDTH - triangleSize - 3;
        int triangleY = cellY + 3;

        // Draw black triangle for all days (since we're using outline for today)
        display.fillTriangle(
            triangleX, triangleY, // Top-left
            triangleX + triangleSize, triangleY, // Top-right
            triangleX + triangleSize, triangleY + triangleSize, // Bottom-right
            GxEPD_BLACK);
    }
}

void DisplayManager::drawEventsList(const std::vector<CalendarEvent*>& events,
    int x, int y, int maxWidth, int maxHeight)
{
    if (events.empty()) {
        drawNoEvents(x, y);
        return;
    }

    // Check if there are any events in the current month
    [[maybe_unused]]
    bool hasCurrentMonthEvents
        = false;
    for (auto event : events) {
        if (event->isToday || event->isTomorrow || event->dayOfMonth > 0) {
            hasCurrentMonthEvents = true;
            break;
        }
    }

    display.setFont(&FONT_EVENT_DATE_HEADER);
    display.setCursor(x, y);

    // if (!hasCurrentMonthEvents && !events.empty()) {
    //     // Show "Next Event" header if no events in current month
    //     display.print("Next Event");
    // } else {
    //     display.print(LOC_EVENTS);
    // }

    // int currentY = y + 30;
    int currentY = y;
    int eventHeight = 55;
    int maxEvents = (maxHeight - 40) / eventHeight;
    int eventsShown = 0;

    // Group events by day
    bool todayShown = false;
    bool tomorrowShown = false;
    bool dayAfterShown = false;

    for (auto event : events) {
        if (eventsShown >= maxEvents)
            break;

        // Add day header if needed
        if (event->isToday && !todayShown) {
#ifdef DISP_TYPE_6C
            display.setTextColor(GxEPD_RED);
#endif
            display.setFont(&FONT_EVENT_DATE_HEADER);
            display.setCursor(x, currentY);
            display.print(LOC_TODAY);
#ifdef DISP_TYPE_6C
            display.setTextColor(GxEPD_BLACK);  // Reset color to black
#endif
            currentY += 20;
            todayShown = true;
        } else if (event->isTomorrow && !tomorrowShown) {
            if (todayShown)
                currentY += 10; // Add spacing
#ifdef DISP_TYPE_6C
            display.setTextColor(COLOR_EVENT_TOMORROW_HEADER);  // Use orange for better visibility
#endif
            display.setFont(&FONT_EVENT_DATE_HEADER);
            display.setCursor(x, currentY);
            display.print(LOC_TOMORROW);
#ifdef DISP_TYPE_6C
            display.setTextColor(GxEPD_BLACK);  // Reset color to black
#endif
            currentY += 20;
            tomorrowShown = true;
        } else if (!event->isToday && !event->isTomorrow && !dayAfterShown) {
            if (todayShown || tomorrowShown)
                currentY += 10; // Add spacing
#ifdef DISP_TYPE_6C
            display.setTextColor(COLOR_EVENT_TOMORROW_HEADER);  // Use orange for all date headers for visibility
#endif
            display.setFont(&FONT_EVENT_DATE_HEADER);
            display.setCursor(x, currentY);

            // Parse and display the date
            if (event->date.length() >= 10) {
                int month = event->date.substring(5, 7).toInt();
                int day = event->date.substring(8, 10).toInt();
                display.print(String(MONTH_NAMES_SHORT[month]) + " " + String(day));
            }
#ifdef DISP_TYPE_6C
            display.setTextColor(GxEPD_BLACK);  // Reset color to black
#endif
            currentY += 20;
            dayAfterShown = true;
        }

        drawEventCompact(event, x + 10, currentY, maxWidth - 20);
        currentY += eventHeight;
        eventsShown++;
    }

    // Show if there are more events
    if (events.size() > eventsShown) {
        display.setFont(&FONT_EVENT_DETAILS);
        display.setCursor(x, currentY + 10);
        display.print("+" + String(events.size() - eventsShown) + " " + LOC_MORE_EVENTS);
    }
}

void DisplayManager::drawEventCompact(const CalendarEvent* event, int x, int y, int maxWidth)
{
#ifdef DISP_TYPE_6C
    // Ensure text color is black for event content
    display.setTextColor(GxEPD_BLACK);
#endif

    // Draw time or "All Day"
    display.setFont(&FONT_EVENT_TIME);
    display.setCursor(x, y);

    if (event->allDay) {
        display.print("--");  // Use dashes instead of "All Day" to save space
    } else {
        display.print(formatTime(event->startTimeStr));
    }

    // Draw event title
    display.setFont(&FONT_EVENT_TITLE);
    display.setCursor(x, y + 35);
    String title = truncateText(event->title, maxWidth);
    display.print(title);

    // Draw location if available
    if (!event->location.isEmpty()) {
        display.setFont(&FONT_EVENT_LOCATION);
        display.setCursor(x, y + 30);
        String location = truncateText(event->location, maxWidth - 20);
        display.print(location);
    }

    // Draw a thin separator line
    display.drawLine(x - 5, y + 40, x + maxWidth - 10, y + 40, GxEPD_BLACK);
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

void DisplayManager::drawStatusBar(bool wifiConnected, int rssi,
    float batteryVoltage, int batteryPercentage,
    int currentDay, int currentMonth, int currentYear, const String& currentTime)
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

    // Center the combined string
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(combinedStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((DISPLAY_WIDTH - w) / 2, y - 8);
    display.print(combinedStr);
}

void DisplayManager::centerText(const String& text, int x, int y, int width, const GFXfont* font)
{
    Serial.println("Centering text: " + text);
    display.setFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(x + (width - w) / 2, y);
    display.print(text);
}

String DisplayManager::formatTime(const String& timeStr)
{
    // Convert from ISO format to readable time
    // Input: 2024-10-14T15:30:00
    // Output: 15:30 (24-hour) or 3:30 PM (12-hour)

    if (timeStr.length() < 16)
        return timeStr;

    int hour = timeStr.substring(11, 13).toInt();
    String minute = timeStr.substring(14, 16);

    if (TIME_FORMAT_24H) {
        // 24-hour format: HH:MM
        String hourStr = (hour < 10) ? "0" + String(hour) : String(hour);
        return hourStr + ":" + minute;
    } else {
        // 12-hour format: H:MM AM/PM
        String ampm = "AM";

        if (hour >= 12) {
            ampm = "PM";
            if (hour > 12)
                hour -= 12;
        }
        if (hour == 0)
            hour = 12;

        return String(hour) + ":" + minute + " " + ampm;
    }
}

String DisplayManager::truncateText(const String& text, int maxWidth)
{
    // Simple truncation - in production you'd measure actual pixel width
    int maxChars = maxWidth / 6; // Approximate character width
    if (text.length() <= maxChars) {
        return text;
    }
    return text.substring(0, maxChars - 3) + "...";
}

MonthCalendar DisplayManager::generateMonthCalendar(int year, int month,
    const std::vector<CalendarEvent*>& events)
{
    MonthCalendar calendar;
    calendar.year = year;
    calendar.month = month;

    // Initialize hasEvent array and event colors
    for (int i = 0; i < 32; i++) {
        calendar.hasEvent[i] = false;
        for (int j = 0; j < MAX_CALENDARS; j++) {
            calendar.eventColors[i][j] = "";
        }
    }

    // Calculate days in month
    int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    calendar.daysInMonth = daysInMonth[month];

    // Check for leap year
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        calendar.daysInMonth = 29;
    }

    // Calculate first day of week for the month
    // Using Zeller's congruence
    int m = month;
    int y = year;
    if (m < 3) {
        m += 12;
        y--;
    }
    int k = y % 100;
    int j = y / 100;
    int h = (1 + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    int dayOfWeek = (h + 6) % 7; // Convert to 0=Sunday

    // Adjust for user's first day of week preference
    calendar.firstDayOfWeek = (dayOfWeek - FIRST_DAY_OF_WEEK + 7) % 7;

    // Set today - will be set by caller since we may not have proper time sync
    calendar.today = 0; // Default to no highlight

    // Mark days with events and collect calendar colors (max 3 calendars per day)
    for (auto event : events) {
        if (event->date.length() >= 10) {
            int eventYear = event->date.substring(0, 4).toInt();
            int eventMonth = event->date.substring(5, 7).toInt();
            int eventDay = event->date.substring(8, 10).toInt();

            if (eventYear == year && eventMonth == month && eventDay >= 1 && eventDay <= 31) {
                calendar.hasEvent[eventDay] = true;

                // Store calendar color for this day (up to 3 different colors)
                if (!event->calendarColor.isEmpty()) {
                    bool colorExists = false;
                    for (int i = 0; i < MAX_CALENDARS; i++) {
                        if (calendar.eventColors[eventDay][i] == event->calendarColor) {
                            colorExists = true;
                            break;
                        }
                    }

                    if (!colorExists) {
                        // Find first empty slot
                        for (int i = 0; i < MAX_CALENDARS; i++) {
                            if (calendar.eventColors[eventDay][i].isEmpty()) {
                                calendar.eventColors[eventDay][i] = event->calendarColor;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return calendar;
}

void DisplayManager::showCalendar(const std::vector<CalendarEvent*>& events,
    const String& currentDate,
    const String& currentTime,
    const WeatherData* weatherData,
    bool wifiConnected,
    int rssi,
    float batteryVoltage,
    int batteryPercentage)
{
    // Get current date info
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    // Use fixed mock date if time not properly set
    int currentDay = timeinfo->tm_mday;
    int currentYear = timeinfo->tm_year + 1900;
    int currentMonth = timeinfo->tm_mon + 1;

    // If date is invalid (not synced yet), use a fixed mock date
    if (currentYear <= 1970 || currentMonth < 1 || currentMonth > 12 || currentDay < 1 || currentDay > 31) {
        currentDay = 16;
        currentMonth = 10;  // October
        currentYear = 2025;
    }

    // Use the modern layout
    showModernCalendar(events, currentDay, currentMonth, currentYear,
        currentTime, weatherData, wifiConnected, rssi, batteryVoltage, batteryPercentage);
}

void DisplayManager::showModernCalendar(const std::vector<CalendarEvent*>& events,
    int currentDay,
    int currentMonth,
    int currentYear,
    const String& currentTime,
    const WeatherData* weatherData,
    bool wifiConnected,
    int rssi,
    float batteryVoltage,
    int batteryPercentage)
{
    // Generate month calendar data
    MonthCalendar monthCal = generateMonthCalendar(currentYear, currentMonth, events);

    // Set today's date if we're displaying the current month
    if (currentYear > 1970) {  // Only if we have valid time
        monthCal.today = currentDay;  // Use the currentDay passed as parameter
    }

    // Format month and year string - handle invalid dates
    String monthYear;
    if (currentYear <= 1970 || currentMonth < 1 || currentMonth > 12) {
        // Time not set yet, show placeholder
        monthYear = "---";
    } else {
        monthYear = String(MONTH_NAMES[currentMonth]) + " " + String(currentYear);
    }

    display.firstPage();
    do {
        // clear();
        fillScreen(GxEPD_WHITE);

        // Draw vertical divider between left and right sides
        drawDivider();

        // LEFT SIDE
        // Draw header with large day number
        drawModernHeader(currentDay, monthYear, currentTime, weatherData);
        // Draw calendar below header
        drawCompactCalendar(monthCal, events);

        // RIGHT SIDE
        // Draw events section at top
        drawEventsSection(events);

        // Draw separator between events and weather with 5px padding
        int separatorY = WEATHER_START_Y - 9;  // Adjusted for moved weather section (was 15, now 9)
        display.drawLine(RIGHT_START_X + 10, separatorY, DISPLAY_WIDTH - 10, separatorY, GxEPD_BLACK);

        // Draw weather section at bottom
        if (weatherData && !weatherData->hourlyForecast.empty()) {
            drawWeatherForecast(*weatherData);
        } else {
            drawWeatherPlaceholder();
        }

        // Draw status bar at the very bottom
        drawStatusBar(wifiConnected, rssi, batteryVoltage, batteryPercentage,
                     currentDay, currentMonth, currentYear, currentTime);

    } while (display.nextPage());
}

void DisplayManager::showMessage(const String& title, const String& message)
{
    display.firstPage();
    do {
        clear();
        display.setFont(&FONT_ERROR_TITLE);
        centerText(title, 0, 100, DISPLAY_WIDTH, &FONT_ERROR_TITLE);

        display.setFont(&FONT_ERROR_MESSAGE);
        centerText(message, 0, 200, DISPLAY_WIDTH, &FONT_ERROR_MESSAGE);
    } while (display.nextPage());
}

void DisplayManager::showError(const String& error)
{
    display.firstPage();
    do {
        clear();
        drawError(error);
    } while (display.nextPage());
}

void DisplayManager::drawError(const String& error)
{
    display.setFont(&FONT_ERROR_TITLE);
    centerText(LOC_ERROR, 0, DISPLAY_HEIGHT / 2 - 40, DISPLAY_WIDTH, &FONT_ERROR_TITLE);

    display.setFont(&FONT_ERROR_MESSAGE);
    centerText(error, 0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH, &FONT_ERROR_MESSAGE);
}

void DisplayManager::drawPreviousNextMonthDay(int day, int col, int row, int x, int y,
                                             int month, int year, bool hasEvent)
{
    // Local constants for old layout compatibility
    const int OLD_CELL_WIDTH = 54;
    const int OLD_CELL_HEIGHT = 50;

    int cellX = x + (col * OLD_CELL_WIDTH);
    int cellY = y + (row * OLD_CELL_HEIGHT);

    // Apply dithering for previous/next month days
    drawDitheredRectangle(cellX + 1, cellY + 1, OLD_CELL_WIDTH - 2, OLD_CELL_HEIGHT - 2,
                        GxEPD_WHITE, GxEPD_BLACK, DitherLevel::DITHER_20);

    // Draw day number centered in cell (gray for previous/next month)
    display.setFont(&FONT_CALENDAR_OUTSIDE_MONTH);
    String dayStr = String(day);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);

    // Center horizontally and vertically
    int textX = cellX + (OLD_CELL_WIDTH - w) / 2;
    int textY = cellY + (OLD_CELL_HEIGHT - y1) / 2; // Using -y1 as it represents the ascent

    display.setCursor(textX, textY);
    display.print(dayStr);

    // Draw event indicator if there's an event for this day
    if (hasEvent) {
        // Draw a small triangle or circle to indicate an event
#ifdef DISP_TYPE_6C
        // On 6-color displays, use a colored indicator
        int circleRadius = 3;
        int circleX = cellX + OLD_CELL_WIDTH / 2;
        int circleY = cellY + OLD_CELL_HEIGHT - 8;
        display.fillCircle(circleX, circleY, circleRadius, COLOR_CALENDAR_OUTSIDE_MONTH);
#else
        // On B/W displays, draw a small triangle
        int triangleSize = 8;
        int triangleX = cellX + OLD_CELL_WIDTH - triangleSize - 3;
        int triangleY = cellY + 3;

        display.fillTriangle(
            triangleX, triangleY, // Top-left
            triangleX + triangleSize, triangleY, // Top-right
            triangleX + triangleSize, triangleY + triangleSize, // Bottom-right
            GxEPD_BLACK);
#endif
    }
}

int DisplayManager::getDaysInMonth(int year, int month)
{
    // Days in each month (0-indexed, 0=January)
    const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    int days = daysInMonth[month];

    // Check for leap year in February
    if (month == 1) { // February
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days = 29;
        }
    }

    return days;
}

void DisplayManager::drawDitheredRectangle(int x, int y, int width, int height,
                                          uint16_t bgColor, uint16_t fgColor, DitherLevel ditherLevel)
{
    // First fill with background color
    if (bgColor != GxEPD_WHITE) {
        display.fillRect(x, y, width, height, bgColor);
    }

    // Apply dithering if not solid or none
    if (ditherLevel != DitherLevel::NONE && ditherLevel != DitherLevel::SOLID) {
        float ditherPercent = static_cast<float>(ditherLevel) / 100.0f;
        applyDithering(x, y, width, height, bgColor, fgColor, ditherPercent);
    } else if (ditherLevel == DitherLevel::SOLID) {
        // Solid fill with foreground color
        display.fillRect(x, y, width, height, fgColor);
    }
}

void DisplayManager::applyDithering(int x, int y, int width, int height,
                                   uint16_t bgColor, uint16_t fgColor, float ditherPercent)
{
    // Ordered dithering using Bayer matrix for consistent e-paper results
    // Using a 4x4 Bayer matrix for smooth gradients
    const uint8_t ditherMatrix[4][4] = {
        { 0, 8, 2, 10 },
        { 12, 4, 14, 6 },
        { 3, 11, 1, 9 },
        { 15, 7, 13, 5 }
    };

    // Apply dithering pattern
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            // Use ordered dithering for more consistent results on e-paper
            int matrixX = dx % 4;
            int matrixY = dy % 4;
            float threshold = ditherMatrix[matrixY][matrixX] / 15.0f;

            // Draw foreground pixel if dither percent is greater than threshold
            if (ditherPercent > threshold) {
                display.drawPixel(x + dx, y + dy, fgColor);
            }
        }
    }
}

void DisplayManager::powerDown()
{
    display.hibernate();
}

void DisplayManager::powerOff()
{
    display.powerOff();
}

void DisplayManager::end()
{
    display.end();
}

void DisplayManager::test()
{
    Serial.println("Testing display...");
    showMessage(LOC_E_PAPER_CALENDAR, LOC_DISPLAY_TEST_SUCCESSFUL);
    delay(2000);
}

// ============================================================================
// Font Metrics Helper Functions
// ============================================================================

int16_t DisplayManager::getFontHeight(const GFXfont* font)
{
    if (!font) {
        // Default font height (usually 8 pixels)
        return 8;
    }

    // Calculate total font height from ascent and descent
    return font->yAdvance;
}

int16_t DisplayManager::getFontBaseline(const GFXfont* font)
{
    if (!font) {
        // Default font baseline
        return 7;
    }

    // The baseline is typically the negative of the minimum y offset
    int16_t minY = 0;
    for (uint8_t i = font->first; i <= font->last; i++) {
        GFXglyph *glyph = &(font->glyph[i - font->first]);
        if (glyph->yOffset < minY) {
            minY = glyph->yOffset;
        }
    }
    return -minY;
}

int16_t DisplayManager::getTextWidth(const String& text, const GFXfont* font)
{
    // Temporarily set font to calculate bounds
    display.setFont(font);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    // Note: Cannot restore old font as GxEPD2 doesn't provide getFont()
    // Caller is responsible for setting font back if needed
    return w;
}

int16_t DisplayManager::calculateYPosition(int16_t baseY, const GFXfont* font, int16_t spacing)
{
    // Calculate next Y position based on font height and spacing
    int16_t fontHeight = getFontHeight(font);
    return baseY + fontHeight + spacing;
}

void DisplayManager::drawTextWithMetrics(const String& text, int16_t x, int16_t y,
                                        const GFXfont* font, bool centerX, bool centerY,
                                        int16_t maxWidth)
{
    display.setFont(font);

    int16_t drawX = x;
    int16_t drawY = y;

    if (centerX || centerY) {
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

        if (centerX) {
            if (maxWidth > 0) {
                drawX = x + (maxWidth - w) / 2;
            } else {
                drawX = x - w / 2;
            }
        }

        if (centerY) {
            // Center vertically using baseline
            int16_t baseline = getFontBaseline(font);
            drawY = y + baseline / 2;
        }
    }

    display.setCursor(drawX, drawY);
    display.print(text);

    // Note: Cannot restore old font as GxEPD2 doesn't provide getFont()
    // Caller is responsible for setting font back if needed
}

// Icon drawing functions - simple geometric representations for e-paper display
void DisplayManager::drawWiFiIcon(int x, int y, int size, bool error)
{
    int barHeight = size / 4;
    int barSpacing = size / 8;

    // Draw WiFi bars
    for (int i = 0; i < 3; i++) {
        int barWidth = size - (i * size / 3);
        int barX = x + (size - barWidth) / 2;
        int barY = y + (i * (barHeight + barSpacing));

        if (i == 0 || !error) {
            display.drawRoundRect(barX, barY, barWidth, barHeight, 2, GxEPD_BLACK);
        }
    }

    // Draw X if error
    if (error) {
        display.drawLine(x + size / 4, y + size / 4, x + 3 * size / 4, y + 3 * size / 4, GxEPD_BLACK);
        display.drawLine(x + 3 * size / 4, y + size / 4, x + size / 4, y + 3 * size / 4, GxEPD_BLACK);
    }
}

void DisplayManager::drawCalendarIcon(int x, int y, int size)
{
    // Draw calendar outline
    display.drawRect(x, y + size / 4, size, 3 * size / 4, GxEPD_BLACK);

    // Draw header
    display.fillRect(x, y + size / 4, size, size / 6, GxEPD_BLACK);

    // Draw date grid
    int cellSize = size / 7;
    for (int i = 1; i < 7; i++) {
        display.drawLine(x + i * cellSize, y + size / 2, x + i * cellSize, y + size, GxEPD_BLACK);
    }

    // Draw hangers
    display.fillRect(x + size / 4, y, size / 12, size / 3, GxEPD_BLACK);
    display.fillRect(x + 2 * size / 3, y, size / 12, size / 3, GxEPD_BLACK);
}

void DisplayManager::drawBatteryIcon(int x, int y, int size)
{
    // Draw battery outline
    display.drawRect(x, y + size / 4, 4 * size / 5, size / 2, GxEPD_BLACK);

    // Draw battery terminal
    display.fillRect(x + 4 * size / 5, y + 3 * size / 8, size / 5, size / 4, GxEPD_BLACK);

    // Draw low battery indicator
    display.fillRect(x + 2, y + size / 4 + 2, size / 5, size / 2 - 4, GxEPD_BLACK);
}

void DisplayManager::drawClockIcon(int x, int y, int size)
{
    // Draw circle
    display.drawCircle(x + size / 2, y + size / 2, size / 2 - 2, GxEPD_BLACK);

    // Draw clock hands
    display.drawLine(x + size / 2, y + size / 2, x + size / 2, y + size / 4, GxEPD_BLACK);
    display.drawLine(x + size / 2, y + size / 2, x + 3 * size / 4, y + size / 2, GxEPD_BLACK);
}

void DisplayManager::drawNetworkIcon(int x, int y, int size)
{
    // Draw globe
    display.drawCircle(x + size / 2, y + size / 2, size / 2 - 2, GxEPD_BLACK);

    // Draw latitude lines
    display.drawLine(x + 2, y + size / 2, x + size - 2, y + size / 2, GxEPD_BLACK);
    display.drawLine(x + size / 4, y + size / 3, x + 3 * size / 4, y + size / 3, GxEPD_BLACK);
    display.drawLine(x + size / 4, y + 2 * size / 3, x + 3 * size / 4, y + 2 * size / 3, GxEPD_BLACK);

    // Draw longitude
    display.drawLine(x + size / 2, y + 2, x + size / 2, y + size - 2, GxEPD_BLACK);
}

void DisplayManager::drawMemoryIcon(int x, int y, int size)
{
    // Draw chip outline
    display.drawRect(x + size / 4, y + size / 4, size / 2, size / 2, GxEPD_BLACK);

    // Draw pins
    for (int i = 0; i < 4; i++) {
        display.drawLine(x + size / 4 - 3, y + size / 4 + i * size / 8 + 3,
            x + size / 4, y + size / 4 + i * size / 8 + 3, GxEPD_BLACK);
        display.drawLine(x + 3 * size / 4, y + size / 4 + i * size / 8 + 3,
            x + 3 * size / 4 + 3, y + size / 4 + i * size / 8 + 3, GxEPD_BLACK);
    }
}

void DisplayManager::drawSettingsIcon(int x, int y, int size)
{
    // Draw gear
    int centerX = x + size / 2;
    int centerY = y + size / 2;
    int radius = size / 3;

    display.drawCircle(centerX, centerY, radius, GxEPD_BLACK);
    display.drawCircle(centerX, centerY, radius / 2, GxEPD_BLACK);

    // Draw teeth
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        int x1 = centerX + cos(angle) * radius;
        int y1 = centerY + sin(angle) * radius;
        int x2 = centerX + cos(angle) * (radius + 4);
        int y2 = centerY + sin(angle) * (radius + 4);
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
    }
}

void DisplayManager::drawUpdateIcon(int x, int y, int size)
{
    // Draw download arrow
    display.drawLine(x + size / 2, y + size / 4, x + size / 2, y + 3 * size / 4, GxEPD_BLACK);
    display.drawLine(x + size / 2, y + 3 * size / 4, x + size / 4, y + size / 2, GxEPD_BLACK);
    display.drawLine(x + size / 2, y + 3 * size / 4, x + 3 * size / 4, y + size / 2, GxEPD_BLACK);

    // Draw base line
    display.drawLine(x + size / 4, y + 3 * size / 4 + 3, x + 3 * size / 4, y + 3 * size / 4 + 3, GxEPD_BLACK);
}

void DisplayManager::drawWarningIcon(int x, int y, int size)
{
    // Use warning_icon bitmap with inverted display
    if (size == 64) {
        display.drawInvertedBitmap(x, y, warning_icon_64x64, 64, 64, GxEPD_BLACK);
    } else if (size == 48) {
        display.drawInvertedBitmap(x, y, warning_icon_48x48, 48, 48, GxEPD_BLACK);
    } else if (size == 32) {
        display.drawInvertedBitmap(x, y, warning_icon_32x32, 32, 32, GxEPD_BLACK);
    } else {
        // For other sizes, draw a simple triangle warning
        display.drawLine(x + size / 2, y + size / 4, x + size / 4, y + 3 * size / 4, GxEPD_BLACK);
        display.drawLine(x + size / 4, y + 3 * size / 4, x + 3 * size / 4, y + 3 * size / 4, GxEPD_BLACK);
        display.drawLine(x + 3 * size / 4, y + 3 * size / 4, x + size / 2, y + size / 4, GxEPD_BLACK);
        // Exclamation mark inside
        display.drawLine(x + size / 2, y + size / 2 - 5, x + size / 2, y + size / 2 + 5, GxEPD_BLACK);
        display.fillCircle(x + size / 2, y + 3 * size / 4 - 5, 2, GxEPD_BLACK);
    }
}

void DisplayManager::drawErrorIcon(int x, int y, int size)
{
    // Use error_icon bitmap with inverted display
    if (size == 64) {
        display.drawInvertedBitmap(x, y, error_icon_64x64, 64, 64, GxEPD_BLACK);
    } else if (size == 48) {
        display.drawInvertedBitmap(x, y, error_icon_48x48, 48, 48, GxEPD_BLACK);
    } else if (size == 32) {
        display.drawInvertedBitmap(x, y, error_icon_32x32, 32, 32, GxEPD_BLACK);
    } else {
        // For other sizes, draw a simple X icon
        display.drawLine(x + size / 4, y + size / 4, x + 3 * size / 4, y + 3 * size / 4, GxEPD_BLACK);
        display.drawLine(x + 3 * size / 4, y + size / 4, x + size / 4, y + 3 * size / 4, GxEPD_BLACK);
        display.drawCircle(x + size / 2, y + size / 2, size / 2 - 2, GxEPD_BLACK);
    }
}

void DisplayManager::drawInfoIcon(int x, int y, int size)
{
    // Draw circle
    display.drawCircle(x + size / 2, y + size / 2, size / 2 - 2, GxEPD_BLACK);

    // Draw "i"
    display.fillCircle(x + size / 2, y + size / 3, 2, GxEPD_BLACK);
    display.drawLine(x + size / 2, y + size / 2 - 2, x + size / 2, y + 3 * size / 4, GxEPD_BLACK);
}

void DisplayManager::drawIcon(ErrorIcon icon, int x, int y, int size)
{
    // Use error_icon for WiFi errors, warning_icon for others
    // Draw with inverted bitmap for better visibility on e-paper

    if (size == 64) {
        switch (icon) {
        case ErrorIcon::WIFI:
            // Use error icon for WiFi
            display.drawInvertedBitmap(x, y, error_icon_64x64, 64, 64, GxEPD_BLACK);
            break;
        case ErrorIcon::CALENDAR:
        case ErrorIcon::BATTERY:
        case ErrorIcon::CLOCK:
        case ErrorIcon::NETWORK:
        case ErrorIcon::MEMORY:
        case ErrorIcon::SETTINGS:
            // Use warning icon for all other errors
            display.drawInvertedBitmap(x, y, warning_icon_64x64, 64, 64, GxEPD_BLACK);
            break;
        default:
            // Default to warning icon
            display.drawInvertedBitmap(x, y, warning_icon_64x64, 64, 64, GxEPD_BLACK);
            break;
        }
    } else if (size == 48) {
        // Use 48x48 icons if available
        switch (icon) {
        case ErrorIcon::WIFI:
            display.drawInvertedBitmap(x, y, error_icon_48x48, 48, 48, GxEPD_BLACK);
            break;
        default:
            display.drawInvertedBitmap(x, y, warning_icon_48x48, 48, 48, GxEPD_BLACK);
            break;
        }
    } else if (size == 32) {
        // Use 32x32 icons if available
        switch (icon) {
        case ErrorIcon::WIFI:
            display.drawInvertedBitmap(x, y, error_icon_32x32, 32, 32, GxEPD_BLACK);
            break;
        default:
            display.drawInvertedBitmap(x, y, warning_icon_32x32, 32, 32, GxEPD_BLACK);
            break;
        }
    } else {
        // For other sizes, fall back to drawing simple icons
        switch (icon) {
        case ErrorIcon::WIFI:
            drawWiFiIcon(x, y, size, true);
            break;
        case ErrorIcon::CALENDAR:
            drawCalendarIcon(x, y, size);
            break;
        case ErrorIcon::BATTERY:
            drawBatteryIcon(x, y, size);
            break;
        case ErrorIcon::CLOCK:
            drawClockIcon(x, y, size);
            break;
        case ErrorIcon::NETWORK:
            drawNetworkIcon(x, y, size);
            break;
        case ErrorIcon::MEMORY:
            drawMemoryIcon(x, y, size);
            break;
        case ErrorIcon::SETTINGS:
            drawSettingsIcon(x, y, size);
            break;
        case ErrorIcon::UPDATE:
            drawUpdateIcon(x, y, size);
            break;
        default:
            drawInfoIcon(x, y, size);
            break;
        }
    }
}

void DisplayManager::showFullScreenError(const ErrorInfo& error)
{
    display.firstPage();
    do {
        clear();

        // Draw large icon at top - use 64x64 for better visibility
        int iconSize = 64;
        int iconX = (DISPLAY_WIDTH - iconSize) / 2;
        int iconY = 50;

        // Draw appropriate icon based on error type
        switch (error.icon) {
        case ErrorIcon::WIFI:
            // Use WiFi X icon for WiFi connection errors
            display.drawInvertedBitmap(iconX, iconY, wifi_x_64x64, 64, 64, GxEPD_BLACK);
            break;
        case ErrorIcon::BATTERY:
            // Use battery alert icon for battery errors
            display.drawInvertedBitmap(iconX, iconY, battery_alert_0deg_64x64, 64, 64, GxEPD_BLACK);
            break;
        case ErrorIcon::MEMORY:
            // Use error icon for memory errors
            display.drawInvertedBitmap(iconX, iconY, error_icon_64x64, 64, 64, GxEPD_BLACK);
            break;
        case ErrorIcon::CALENDAR:
        case ErrorIcon::CLOCK:
        case ErrorIcon::NETWORK:
        case ErrorIcon::SETTINGS:
        case ErrorIcon::UPDATE:
            // Use warning icon for other error types
            display.drawInvertedBitmap(iconX, iconY, warning_icon_64x64, 64, 64, GxEPD_BLACK);
            break;
        default:
            // Default based on error level
            if (error.level == ErrorLevel::WARNING || error.level == ErrorLevel::INFO) {
                display.drawInvertedBitmap(iconX, iconY, warning_icon_64x64, 64, 64, GxEPD_BLACK);
            } else {
                display.drawInvertedBitmap(iconX, iconY, error_icon_64x64, 64, 64, GxEPD_BLACK);
            }
            break;
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
        display.setCursor(20, DISPLAY_HEIGHT - 40);
        display.print(errorCode);

        // Draw retry info if applicable
        if (error.recoverable && error.maxRetries > 0) {
            display.setFont(&FONT_ERROR_DETAILS);
            String retryText = LOC_ERROR_RETRYING;
            retryText += " (" + String(error.retryCount) + "/" + String(error.maxRetries) + ")";
            display.setCursor(20, DISPLAY_HEIGHT - 20);
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
        display.setCursor(DISPLAY_WIDTH - w - 20, DISPLAY_HEIGHT - 20);
        display.print(actionText);

    } while (display.nextPage());
}