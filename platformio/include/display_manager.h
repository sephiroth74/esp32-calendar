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

// Enum for dither percentage levels
enum class DitherLevel {
    NONE = 0, // No dithering (solid fill)
    DITHER_10 = 10, // 10% dithering (very light)
    DITHER_20 = 20, // 20% dithering (light gray)
    DITHER_25 = 25, // 25% dithering
    DITHER_30 = 30, // 30% dithering
    DITHER_40 = 40, // 40% dithering
    DITHER_50 = 50, // 50% dithering (medium gray)
    DITHER_60 = 60, // 60% dithering
    DITHER_70 = 70, // 70% dithering
    DITHER_75 = 75, // 75% dithering (dark gray)
    SOLID = 100 // Solid fill (100%)
};

struct MonthCalendar {
    int year;
    int month;
    int daysInMonth;
    int firstDayOfWeek; // 0=Sunday, 1=Monday, etc.
    int today;
    bool hasEvent[32]; // Track which days have events (1-31)
    bool hasHoliday[32]; // Track which days are holidays (1-31)
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
    static const int SPLIT_X = 400; // Split screen at middle

    // Left side - Header and Calendar
    static const int LEFT_WIDTH = 400; // Left half of screen
    static const int HEADER_HEIGHT = 120; // Header with large day number and month/year
    static const int CALENDAR_START_Y = HEADER_HEIGHT + 20; // Moved down 10 pixels (was +10)
    static const int CALENDAR_HEIGHT = 360; // Adjusted for moved calendar

    // Right side - Events and Weather
    static const int RIGHT_WIDTH = 400; // Right half of screen
    static const int RIGHT_START_X = SPLIT_X; // Start of right side
    static const int EVENTS_HEIGHT = 340; // More space for events
    static const int WEATHER_START_Y = DISPLAY_HEIGHT - 100; // Bottom position above status bar (moved down 30px total)
    static const int WEATHER_HEIGHT = 100; // Compact weather section

    // Calendar grid dimensions for left side
    static const int CALENDAR_MARGIN = 20; // Margins on sides
    static const int CELL_WIDTH = 50; // (380/7) ≈ 50 pixels per day
    static const int CELL_HEIGHT = 45; // Height of calendar cells
    static const int DAY_LABEL_HEIGHT = 25; // Height for day labels

    // Helper methods
    void drawModernHeader(int currentDay, const String& monthYear, const String& currentTime, const WeatherData* weatherData = nullptr);
    void drawCompactCalendar(const MonthCalendar& calendar,
        const std::vector<CalendarEvent*>& events);
    void drawPreviousNextMonthDay(int day, int col, int row, int x, int y,
        int month, int year, bool hasEvent = false);
    void drawDitheredRectangle(int x, int y, int width, int height,
        uint16_t bgColor, uint16_t fgColor, DitherLevel ditherLevel);
    void applyDithering(int x, int y, int width, int height,
        uint16_t bgColor, uint16_t fgColor, float ditherPercent);
    int getDaysInMonth(int year, int month);
    String formatEventDate(const String& eventDate, int currentYear, int currentMonth, int currentDay);
    void drawEventsSection(const std::vector<CalendarEvent*>& events);
    void drawWeatherPlaceholder();
    void drawWeatherForecast(const WeatherData& weatherData);
    void drawDivider();
    void drawNoEvents(int x, int y);
    void drawError(const String& error);
    void drawStatusBar(bool wifiConnected, int rssi, float batteryVoltage, int batteryPercentage,
        int currentDay, int currentMonth, int currentYear, const String& currentTime, bool isStale = false);
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

    // Font metrics helper functions
    int16_t getFontHeight(const GFXfont* font);
    int16_t getFontBaseline(const GFXfont* font);
    int16_t getTextWidth(const String& text, const GFXfont* font);
    int16_t calculateYPosition(int16_t baseY, const GFXfont* font, int16_t spacing = 0);
    void drawTextWithMetrics(const String& text, int16_t x, int16_t y, const GFXfont* font,
        bool centerX = false, bool centerY = false, int16_t maxWidth = 0);

public:
    /**
     * @brief Construct a new Display Manager object
     */
    DisplayManager();

    /**
     * @brief Initialize the e-paper display
     *
     * Configures SPI pins, initializes the display hardware, and sets up
     * the display for rendering. Must be called before any other display operations.
     */
    void init();

    /**
     * @brief Clear the display to white
     *
     * Clears the internal framebuffer. Call refresh() to update the physical display.
     */
    void clear();

    /**
     * @brief Display the current framebuffer on screen
     *
     * Convenience method that calls refresh(). Updates the physical e-paper display
     * with the current framebuffer contents.
     */
    void displayScreen();

    /**
     * @brief Move to next page in paged rendering mode
     * @return true if there are more pages to render
     */
    bool nextPage();

    /**
     * @brief Get number of pages for paged rendering
     * @return Number of pages (1 for non-paged displays)
     */
    uint16_t pages();

    /**
     * @brief Get height of each page in pixels
     * @return Page height in pixels
     */
    uint16_t pageHeight();

    /**
     * @brief Set display rotation
     * @param rotation Rotation value (0=0°, 1=90°, 2=180°, 3=270°)
     */
    void setRotation(uint8_t rotation);

    /**
     * @brief Set rendering window to full display
     *
     * Configures the display to render to the entire screen.
     * Used for full refreshes as opposed to partial updates.
     */
    void setFullWindow();

    /**
     * @brief Fill entire screen with specified color
     * @param color Color value (GxEPD_BLACK, GxEPD_WHITE, etc.)
     */
    void fillScreen(uint16_t color);

    /**
     * @brief Set cursor position for text rendering
     * @param x X coordinate in pixels
     * @param y Y coordinate in pixels (baseline position)
     */
    void setCursor(int16_t x, int16_t y);

    /**
     * @brief Set font for text rendering
     * @param f Pointer to GFXfont structure
     */
    void setFont(const GFXfont* f);

    /**
     * @brief Set text color for rendering
     * @param c Color value (GxEPD_BLACK, GxEPD_WHITE, etc.)
     */
    void setTextColor(uint16_t c);

    /** @brief Print String to display */
    size_t print(const String& s);
    /** @brief Print C-string to display */
    size_t print(const char str[]);
    /** @brief Print single character to display */
    size_t print(char c);
    /** @brief Print any Printable object to display */
    size_t print(const Printable& x);

    /**
     * @brief Get bounding box for text string
     * @param string Text string to measure
     * @param x X position for text
     * @param y Y position for text
     * @param x1 Output: left edge of bounding box
     * @param y1 Output: top edge of bounding box
     * @param w Output: width of bounding box
     * @param h Output: height of bounding box
     */
    void getTextBounds(const char* string, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    void getTextBounds(const __FlashStringHelper* s, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    void getTextBounds(const String& str, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

    /**
     * @brief Start first page for paged rendering
     *
     * Begins the firstPage/nextPage rendering loop. Use in conjunction with
     * nextPage() for displays that require paged rendering.
     */
    void firstPage();

    /**
     * @brief Fill rectangle with specified color
     * @param x X coordinate of top-left corner
     * @param y Y coordinate of top-left corner
     * @param w Width in pixels
     * @param h Height in pixels
     * @param color Fill color
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    /**
     * @brief Draw inverted bitmap (black pixels become color)
     * @param x X coordinate of top-left corner
     * @param y Y coordinate of top-left corner
     * @param bitmap Pointer to bitmap data
     * @param w Width in pixels
     * @param h Height in pixels
     * @param color Color for bitmap pixels
     */
    void drawInvertedBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);

    /** @brief Check if display supports color */
    bool hasColor();
    /** @brief Check if display supports partial updates */
    bool hasPartialUpdate();
    /** @brief Check if display supports fast partial updates */
    bool hasFastPartialUpdate();

    /** @brief Get display width in pixels */
    int16_t width(void);
    /** @brief Get display height in pixels */
    int16_t height(void);

    /**
     * @brief Refresh the physical display
     * @param partial_update_mode If true, use partial update (faster but may leave ghosting)
     */
    void refresh(bool partial_update_mode = false);

    /**
     * @brief Display calendar in legacy list-based layout (deprecated)
     *
     * Legacy method for showing calendar events in simple list format.
     * Kept for backward compatibility - use showModernCalendar() instead.
     *
     * @param events Vector of calendar events to display
     * @param currentDate Current date string
     * @param currentTime Current time string
     * @param weatherData Optional weather data for display
     * @param wifiConnected WiFi connection status
     * @param rssi WiFi signal strength in dBm
     * @param batteryVoltage Battery voltage
     * @param batteryPercentage Battery percentage (0-100)
     * @param isStale true if showing cached/stale data
     * @deprecated Use showModernCalendar() for modern split-screen layout
     */
    void showCalendar(const std::vector<CalendarEvent*>& events,
        const String& currentDate,
        const String& currentTime,
        const WeatherData* weatherData = nullptr,
        bool wifiConnected = true,
        int rssi = 0,
        float batteryVoltage = 0.0,
        int batteryPercentage = 0,
        bool isStale = false);

    /**
     * @brief Display calendar in modern split-screen layout
     *
     * Renders calendar in modern two-column layout:
     * - Left side: Month calendar grid with event indicators
     * - Right side: Today/tomorrow weather and event list
     *
     * Features:
     * - Large day number and month/year in header
     * - Calendar grid with colored event dots
     * - Side-by-side today/tomorrow weather with icons
     * - Event list with date headers
     * - Status bar with WiFi, battery, and time
     *
     * @param events Vector of calendar events to display
     * @param currentDay Day of month (1-31)
     * @param currentMonth Month (1-12)
     * @param currentYear Full year (e.g., 2025)
     * @param currentTime Time string (e.g., "14:30")
     * @param weatherData Optional weather data for display
     * @param wifiConnected WiFi connection status
     * @param rssi WiFi signal strength in dBm (-100 to 0)
     * @param batteryVoltage Battery voltage (e.g., 4.2)
     * @param batteryPercentage Battery percentage (0-100)
     * @param isStale true if showing cached/stale data
     */
    void showModernCalendar(const std::vector<CalendarEvent*>& events,
        int currentDay,
        int currentMonth,
        int currentYear,
        const String& currentTime,
        const WeatherData* weatherData = nullptr,
        bool wifiConnected = true,
        int rssi = 0,
        float batteryVoltage = 0.0,
        int batteryPercentage = 0,
        bool isStale = false);

    /**
     * @brief Display simple message with title
     * @param title Message title
     * @param message Message text
     */
    void showMessage(const String& title, const String& message);

    /**
     * @brief Display simple error message
     * @param error Error text to display
     */
    void showError(const String& error);

    /**
     * @brief Display full-screen error with icon
     *
     * Shows comprehensive error information with appropriate icon,
     * error level indicator, and detailed message. Used for critical
     * errors that prevent normal operation.
     *
     * @param error ErrorInfo structure with code, message, level, icon, and details
     */
    void showFullScreenError(const ErrorInfo& error);

    /**
     * @brief Power down the display (low power mode)
     *
     * Puts display into low power mode while maintaining display content.
     * Display can be quickly woken up.
     */
    void powerDown();

    /**
     * @brief Power off the display completely
     *
     * Completely powers off the display. Requires full re-initialization
     * to use again. Use for deep sleep scenarios.
     */
    void powerOff();

    /**
     * @brief End display operations and clean up
     */
    void end();

    /**
     * @brief Run display test pattern
     *
     * Displays test pattern to verify display is working correctly.
     * Used for hardware debugging and initial setup verification.
     */
    void test();
};

#endif // DISPLAY_MANAGER_H