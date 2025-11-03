#include "display_manager.h"
#include "localization.h"
#include "debug_config.h"
#include "string_utils.h"
#include <assets/fonts.h>

// ============================================================================
// Calendar Drawing Functions
// ============================================================================

// Helper method: Draw day labels (M T W T F S S)
void DisplayManager::drawCalendarDayLabels(int startX, int startY, int cellWidth) {
    display.setFont(&FONT_CALENDAR_DAY_LABELS);

#ifdef DISP_TYPE_6C
    display.setTextColor(COLOR_CALENDAR_DAY_LABELS);
#endif

    for (int i = 0; i < 7; i++) {
        int dayIndex = (i + FIRST_DAY_OF_WEEK) % 7;
        String shortDay = String(DAY_NAMES_SHORT[dayIndex]).substring(0, 1);

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(shortDay, 0, 0, &x1, &y1, &w, &h);
        int x = startX + (i * cellWidth) + (cellWidth - w) / 2;
        display.setCursor(x, startY + 15);
        display.print(shortDay);
    }

#ifdef DISP_TYPE_6C
    display.setTextColor(GxEPD_BLACK);
#endif
}

// Helper method: Draw previous month days
void DisplayManager::drawCalendarPrevMonthDays(int startX, int startY, int cellWidth,
                                                const MonthCalendar& calendar,
                                                const std::vector<CalendarEvent*>& events,
                                                int& row, int& col) {
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

        display.setFont(&FONT_CALENDAR_OUTSIDE_MONTH);
        String dayStr = String(prevMonthDay);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

#ifdef DISP_TYPE_6C
        display.setTextColor(COLOR_CALENDAR_OUTSIDE_MONTH);
        display.print(dayStr);
        display.setTextColor(GxEPD_BLACK);
#else
        display.print(dayStr);
#endif

        // Check for events
        bool hasEvent = false;
        for (auto event : events) {
            if (event->date.length() >= 10) {
                int eventYear = event->date.substring(0, 4).toInt();
                int eventMonth = event->date.substring(5, 7).toInt();
                int eventDay = event->date.substring(8, 10).toInt();

                if (eventYear == prevYear && eventMonth == (prevMonth + 1) && eventDay == prevMonthDay) {
                    hasEvent = true;
                    break;
                }
            }
        }

        if (hasEvent) {
#ifdef DISP_TYPE_6C
            display.fillCircle(x + cellWidth / 2, y + 32, 3, COLOR_CALENDAR_OUTSIDE_MONTH);
#else
            display.fillCircle(x + cellWidth / 2, y + 32, 3, GxEPD_BLACK);
#endif
        }

        prevMonthDay++;
    }
}

// Helper method: Draw current month days
void DisplayManager::drawCalendarCurrentMonthDays(int startX, int startY, int cellWidth,
                                                   const MonthCalendar& calendar,
                                                   int& row, int& col) {
    display.setFont(&FONT_CALENDAR_DAY_NUMBERS);
    int currentDay = 1;
    col = calendar.firstDayOfWeek;

    while (currentDay <= calendar.daysInMonth) {
        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        // Highlight weekends and holidays
        bool isWeekend = (FIRST_DAY_OF_WEEK == 0) ? (col == 0 || col == 6) : (col == 5 || col == 6);
        bool isHoliday = calendar.hasHoliday[currentDay];

        if (isWeekend || isHoliday) {
#ifdef DISP_TYPE_6C
            drawDitheredRectangle(x, y, cellWidth, CELL_HEIGHT, GxEPD_WHITE, COLOR_CALENDAR_WEEKEND_BG,
                                static_cast<DitherLevel>(DITHER_CALENDAR_WEEKEND));
#else
            drawDitheredRectangle(x, y, cellWidth, CELL_HEIGHT, GxEPD_WHITE, GxEPD_BLACK, DitherLevel::DITHER_10);
#endif
        }

        // Highlight today with border
        bool isToday = (currentDay == calendar.today);
        if (isToday) {
#ifdef DISP_TYPE_6C
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, COLOR_CALENDAR_TODAY_BORDER);
            display.drawRect(x + 2, y + 2, cellWidth - 4, CELL_HEIGHT - 4, COLOR_CALENDAR_TODAY_BORDER);
#else
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, GxEPD_BLACK);
            display.drawRect(x + 2, y + 2, cellWidth - 4, CELL_HEIGHT - 4, GxEPD_BLACK);
#endif
        }

        // Draw day number
        String dayStr = String(currentDay);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

#ifdef DISP_TYPE_6C
        if (isToday) {
            display.setTextColor(COLOR_CALENDAR_TODAY_TEXT);
            display.print(dayStr);
            display.setTextColor(GxEPD_BLACK);
        } else {
            display.print(dayStr);
        }
#else
        display.print(dayStr);
#endif

        // Draw event indicators
        if (calendar.hasEvent[currentDay]) {
#ifdef DISP_TYPE_6C
            int circleCount = 0;
            for (int i = 0; i < MAX_CALENDARS; i++) {
                if (!calendar.eventColors[currentDay][i].isEmpty()) circleCount++;
            }

            if (circleCount > 0) {
                int circleRadius = 3;
                int circleSpacing = 8;
                int totalWidth = (circleCount * circleRadius * 2) + ((circleCount - 1) * circleSpacing);
                int circleStartX = x + cellWidth / 2 - totalWidth / 2 + circleRadius;

                for (int i = 0; i < circleCount; i++) {
                    String colorStr = calendar.eventColors[currentDay][i];
                    uint16_t circleColor = GxEPD_BLACK;

                    if (colorStr == "red") circleColor = GxEPD_RED;
                    else if (colorStr == "orange") circleColor = GxEPD_ORANGE;
                    else if (colorStr == "yellow") circleColor = GxEPD_YELLOW;
                    else if (colorStr == "green") circleColor = GxEPD_GREEN;

                    display.fillCircle(circleStartX + (i * (circleRadius * 2 + circleSpacing)), y + 32, circleRadius, circleColor);
                }
            }
#else
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
}

// Helper method: Draw next month days
void DisplayManager::drawCalendarNextMonthDays(int startX, int startY, int cellWidth,
                                                const MonthCalendar& calendar,
                                                const std::vector<CalendarEvent*>& events,
                                                int& row, int& col) {
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
            if (row >= 6) break;
        }

        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        display.setFont(&FONT_CALENDAR_OUTSIDE_MONTH);
        String dayStr = String(nextMonthDay);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

#ifdef DISP_TYPE_6C
        display.setTextColor(COLOR_CALENDAR_OUTSIDE_MONTH);
        display.print(dayStr);
        display.setTextColor(GxEPD_BLACK);
#else
        display.print(dayStr);
#endif

        // Check for events
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

        if (hasEvent) {
#ifdef DISP_TYPE_6C
            display.fillCircle(x + cellWidth / 2, y + 32, 3, COLOR_CALENDAR_OUTSIDE_MONTH);
#else
            display.fillCircle(x + cellWidth / 2, y + 32, 3, GxEPD_BLACK);
#endif
        }

        nextMonthDay++;
        col++;
    }
}

// Main function: Draw compact calendar (refactored)
void DisplayManager::drawCompactCalendar(const MonthCalendar& calendar,
                                        const std::vector<CalendarEvent*>& events) {
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

MonthCalendar DisplayManager::generateMonthCalendar(int year, int month,
    const std::vector<CalendarEvent*>& events)
{
    MonthCalendar calendar;
    calendar.year = year;
    calendar.month = month;

    // Initialize hasEvent array, hasHoliday array, and event colors
    for (int i = 0; i < 32; i++) {
        calendar.hasEvent[i] = false;
        calendar.hasHoliday[i] = false;
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

                // Mark as holiday if this is a holiday event
                if (event->isHoliday) {
                    calendar.hasHoliday[eventDay] = true;
                }

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
