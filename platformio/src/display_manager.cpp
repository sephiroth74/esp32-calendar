#include "display_manager.h"
#include "localization.h"
#include "version.h"
#include "debug_config.h"
#include "string_utils.h"
#include <SPI.h>
#include <assets/fonts.h>
#include <assets/icons/icons.h>

DisplayManager::DisplayManager()
    : display(GxEPD2_DRIVER_CLASS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY))
{
}

void DisplayManager::init()
{
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

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    display.fillRect(x, y, w, h, color);
}

void DisplayManager::drawInvertedBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
    display.drawInvertedBitmap(x, y, bitmap, w, h, color);
}

bool DisplayManager::hasColor()
{
    return display.epd2.hasColor;
}

bool DisplayManager::hasPartialUpdate()
{
    return display.epd2.hasPartialUpdate;
}

bool DisplayManager::hasFastPartialUpdate()
{
    return display.epd2.hasFastPartialUpdate;
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

void DisplayManager::drawDivider()
{
    // Draw thin vertical line between left and right sides
    // Stop before the bottom status bar area
    display.drawLine(SPLIT_X, 0, SPLIT_X, DISPLAY_HEIGHT - 40, GxEPD_BLACK);
}

void DisplayManager::centerText(const String& text, int x, int y, int width, const GFXfont* font)
{
    DEBUG_VERBOSE_PRINTLN("Centering text: " + text);
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

void DisplayManager::showCalendar(const std::vector<CalendarEvent*>& events,
    const String& currentDate,
    const String& currentTime,
    const WeatherData* weatherData,
    bool wifiConnected,
    int rssi,
    float batteryVoltage,
    int batteryPercentage,
    bool isStale)
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
        currentTime, weatherData, wifiConnected, rssi, batteryVoltage, batteryPercentage, isStale);
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
    int batteryPercentage,
    bool isStale)
{
#ifdef DEBUG_DISPLAY
    Serial.println("[DisplayManager] showModernCalendar started");
    Serial.println("[DisplayManager] Events count: " + String(events.size()));
#endif

    // Generate month calendar data
    MonthCalendar monthCal = generateMonthCalendar(currentYear, currentMonth, events);

#ifdef DEBUG_DISPLAY
    Serial.println("[DisplayManager] Month calendar generated");
#endif

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

#ifdef DEBUG_DISPLAY
    Serial.println("[DisplayManager] Calling firstPage()...");
#endif

    display.firstPage();

#ifdef DEBUG_DISPLAY
    Serial.println("[DisplayManager] Starting rendering loop...");
#endif

    do {
#ifdef DEBUG_DISPLAY
        Serial.println("[DisplayManager] Rendering page...");
#endif
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
        if (weatherData && !weatherData->dailyForecast.empty()) {
            drawWeatherForecast(*weatherData);
        } else {
            drawWeatherPlaceholder();
        }

        // Draw status bar at the very bottom
        drawStatusBar(wifiConnected, rssi, batteryVoltage, batteryPercentage,
                     currentDay, currentMonth, currentYear, currentTime, isStale);

    } while (display.nextPage());

#ifdef DEBUG_DISPLAY
    Serial.println("[DisplayManager] showModernCalendar completed");
#endif
}

void DisplayManager::showMessage(const String& title, const String& message)
{
    display.firstPage();
    do {
        fillScreen(GxEPD_WHITE);
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
        fillScreen(GxEPD_WHITE);
        drawError(error);
    } while (display.nextPage());
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