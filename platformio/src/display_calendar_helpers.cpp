#include "display_manager.h"
#include "localization.h"
#include "debug_config.h"
#include "string_utils.h"
#include <assets/fonts.h>

// ============================================================================
// Shared Calendar Helper Functions (used by both orientations)
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

    for (int day = 1; day <= calendar.daysInMonth; day++) {
        int x = startX + (col * cellWidth);
        int y = startY + (row * CELL_HEIGHT);

        // Check if this is today
        bool isToday = (day == calendar.today);

        // Check if weekend (Saturday or Sunday)
        int dayOfWeek = (col + FIRST_DAY_OF_WEEK) % 7;
        bool isWeekend = (dayOfWeek == 0 || dayOfWeek == 6);  // Sunday or Saturday

        // Draw weekend background with dithering
        if (isWeekend && !isToday) {
#ifdef DISP_TYPE_6C
            drawDitheredRectangle(x, y, cellWidth, CELL_HEIGHT,
                                GxEPD_WHITE, COLOR_CALENDAR_WEEKEND_BG, static_cast<DitherLevel>(DITHER_CALENDAR_WEEKEND));
#endif
        }

        // Draw today's cell with red border (2px thick)
        if (isToday) {
#ifdef DISP_TYPE_6C
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, COLOR_CALENDAR_TODAY_BORDER);
            display.drawRect(x + 2, y + 2, cellWidth - 4, CELL_HEIGHT - 4, COLOR_CALENDAR_TODAY_BORDER);
#else
            // Black border for B&W displays
            display.drawRect(x + 1, y + 1, cellWidth - 2, CELL_HEIGHT - 2, GxEPD_BLACK);
#endif
        }

        // Draw day number
        String dayStr = String(day);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(x + (cellWidth - w) / 2, y + 20);

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

        // Draw event indicator if this day has events
        if (calendar.hasEvent[day]) {
#ifdef DISP_TYPE_6C
            // Use first calendar's color if available
            uint16_t dotColor = GxEPD_BLACK;
            if (!calendar.eventColors[day][0].isEmpty()) {
                String colorStr = calendar.eventColors[day][0];
                if (colorStr == "red") dotColor = GxEPD_RED;
                else if (colorStr == "orange") dotColor = GxEPD_ORANGE;
                else if (colorStr == "yellow") dotColor = GxEPD_YELLOW;
                else if (colorStr == "green") dotColor = GxEPD_GREEN;
            }
            display.fillCircle(x + cellWidth / 2, y + 32, 3, dotColor);
#else
            display.fillCircle(x + cellWidth / 2, y + 32, 3, GxEPD_BLACK);
#endif
        }

        // Move to next cell
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
    while (col < 7) {
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

    // Calculate first day of week for the month using Zeller's congruence
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

    // Mark days with events and collect calendar colors
    for (auto event : events) {
        if (event->date.length() >= 10) {
            int eventYear = event->date.substring(0, 4).toInt();
            int eventMonth = event->date.substring(5, 7).toInt();
            int eventDay = event->date.substring(8, 10).toInt();

            if (eventYear == year && eventMonth == month && eventDay >= 1 && eventDay <= 31) {
                calendar.hasEvent[eventDay] = true;

                // Store calendar color (supports up to MAX_CALENDARS per day)
                for (int i = 0; i < MAX_CALENDARS; i++) {
                    if (calendar.eventColors[eventDay][i].isEmpty()) {
                        calendar.eventColors[eventDay][i] = event->calendarColor;
                        break;
                    }
                }
            }
        }
    }

    return calendar;
}

int DisplayManager::getDaysInMonth(int year, int month)
{
    const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int days = daysInMonth[month];

    // Check for leap year in February
    if (month == 1) { // February (0-indexed)
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days = 29;
        }
    }

    return days;
}
