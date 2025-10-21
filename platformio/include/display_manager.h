#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "config.h"
#include "error_manager.h"
#include "weather_client.h"
#include <vector>
#include <ArduinoJson.h>
#include "calendar_event.h"

#define MAX_DISPLAY_BUFFER_SIZE 65536ul

#ifdef DISP_TYPE_BW
#include <GxEPD2_BW.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_750_GDEY075T7
#define MAX_HEIGHT(EPD)                                        \
    (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) \
            ? EPD::HEIGHT                                      \
            : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

#elif defined(DISP_TYPE_6C)
#include <GxEPD2_7C.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_7C
#define GxEPD2_DRIVER_CLASS GxEPD2_730c_GDEP073E01
#define MAX_HEIGHT(EPD)                                          \
    (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) \
            ? EPD::HEIGHT                                        \
            : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#endif // DISP_TYPE

// Forward declaration - CalendarEvent is now defined in calendar_event.h
class CalendarEvent;

struct MonthCalendar {
    int year;
    int month;
    int daysInMonth;
    int firstDayOfWeek; // 0=Sunday, 1=Monday, etc.
    int today;
    bool hasEvent[32]; // Track which days have events (1-31)
    String eventColors[32][MAX_CALENDARS]; // Store calendar colors per day (max MAX_CALENDARS)
};

class DisplayManager {
// Make internal display and methods accessible in debug mode
#ifdef DEBUG_DISPLAY
public:
#else
private:
#endif
    GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display;

#ifdef DEBUG_DISPLAY
public:
#else
private:
#endif
    // Vertical split layout constants
    static const int SPLIT_X = 400;                // Split screen at middle

    // Left side - Header and Calendar
    static const int LEFT_WIDTH = 400;             // Left half of screen
    static const int HEADER_HEIGHT = 120;          // Header with large day number and month/year
    static const int CALENDAR_START_Y = HEADER_HEIGHT + 20;  // Moved down 10 pixels (was +10)
    static const int CALENDAR_HEIGHT = 360;        // Adjusted for moved calendar

    // Right side - Events and Weather
    static const int RIGHT_WIDTH = 400;            // Right half of screen
    static const int RIGHT_START_X = SPLIT_X;      // Start of right side
    static const int EVENTS_HEIGHT = 340;          // More space for events
    static const int WEATHER_START_Y = DISPLAY_HEIGHT - 150; // Bottom position above status bar (moved up 10px)
    static const int WEATHER_HEIGHT = 100;         // Compact weather section

    // Calendar grid dimensions for left side
    static const int CALENDAR_MARGIN = 20;         // Margins on sides
    static const int CELL_WIDTH = 50;              // (380/7) ≈ 50 pixels per day
    static const int CELL_HEIGHT = 45;             // Height of calendar cells
    static const int DAY_LABEL_HEIGHT = 25;        // Height for day labels

    // Helper methods
    void drawHeader(const String& currentDate, const String& currentTime);
    void drawModernHeader(int currentDay, const String& monthYear, const String& currentTime);
    void drawMonthCalendar(const MonthCalendar& calendar, int x, int y);
    void drawCompactCalendar(const MonthCalendar& calendar);
    void drawCalendarGrid(int x, int y);
    void drawCalendarDay(int day, int col, int row, int x, int y, bool hasEvent, bool isToday);
    void drawPreviousNextMonthDay(int day, int col, int row, int x, int y);
    void drawDayLabels(int x, int y);
    void applyFloydSteinbergDithering(int x, int y, int width, int height, float grayLevel);
    int getDaysInMonth(int year, int month);
    void drawEventsList(const std::vector<CalendarEvent*>& events, int x, int y, int maxWidth, int maxHeight);
    String formatEventDate(const String& eventDate, int currentYear, int currentMonth, int currentDay);
    void drawEventsSection(const std::vector<CalendarEvent*>& events);
    void drawEventCompact(const CalendarEvent* event, int x, int y, int maxWidth);
    void drawWeatherPlaceholder();
    void drawWeatherForecast(const WeatherData& weatherData);
    void drawDivider();
    void drawNoEvents(int x, int y);
    void drawError(const String& error);
    void drawStatusBar(bool wifiConnected, int rssi, float batteryVoltage, int batteryPercentage,
                      int currentDay, int currentMonth, int currentYear, const String& currentTime);
    void centerText(const String& text, int x, int y, int width, const GFXfont* font);
    String formatTime(const String& timeStr);
    String truncateText(const String& text, int maxWidth);
    MonthCalendar generateMonthCalendar(int year, int month, const std::vector<CalendarEvent*>& events);

    // Error icon drawing functions
    void drawWiFiIcon(int x, int y, int size, bool error = false);
    void drawCalendarIcon(int x, int y, int size);
    void drawBatteryIcon(int x, int y, int size);
    void drawClockIcon(int x, int y, int size);
    void drawNetworkIcon(int x, int y, int size);
    void drawMemoryIcon(int x, int y, int size);
    void drawSettingsIcon(int x, int y, int size);
    void drawUpdateIcon(int x, int y, int size);
    void drawWarningIcon(int x, int y, int size);
    void drawErrorIcon(int x, int y, int size);
    void drawInfoIcon(int x, int y, int size);
    void drawIcon(ErrorIcon icon, int x, int y, int size);

public:
    DisplayManager();
    void init();
    void clear();
    void showCalendar(const std::vector<CalendarEvent*>& events,
                     const String& currentDate,
                     const String& currentTime,
                     const WeatherData* weatherData = nullptr,
                     bool wifiConnected = true,
                     int rssi = 0,
                     float batteryVoltage = 0.0,
                     int batteryPercentage = 0);
    void showModernCalendar(const std::vector<CalendarEvent*>& events,
                           int currentDay,
                           int currentMonth,
                           int currentYear,
                           const String& currentTime,
                           const WeatherData* weatherData = nullptr,
                           bool wifiConnected = true,
                           int rssi = 0,
                           float batteryVoltage = 0.0,
                           int batteryPercentage = 0);
    void showMessage(const String& title, const String& message);
    void showError(const String& error);
    void showFullScreenError(const ErrorInfo& error);
    void powerDown();
    void test();
};

#endif // DISPLAY_MANAGER_H