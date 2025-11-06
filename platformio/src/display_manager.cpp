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

    // Set rotation based on orientation
#if DISPLAY_ORIENTATION == PORTRAIT
    display.setRotation(1); // 90 degrees for portrait (480x800 becomes 800x480 rotated)
    DEBUG_INFO_PRINTLN("Display initialized in PORTRAIT mode!");
#else
    display.setRotation(0); // No rotation for landscape
    DEBUG_INFO_PRINTLN("Display initialized in LANDSCAPE mode!");
#endif
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

#if DISPLAY_ORIENTATION == LANDSCAPE
void DisplayManager::drawDivider()
{
    // Draw thin vertical line between left and right sides
    // Stop before the bottom status bar area
    display.drawLine(SPLIT_X, 0, SPLIT_X, DISPLAY_HEIGHT - 40, GxEPD_BLACK);
}
#endif

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

    // Use the modern layout (now extracts date/time internally)
    showModernCalendar(events, now, weatherData, wifiConnected, rssi, batteryVoltage, batteryPercentage, isStale);
}

void DisplayManager::showModernCalendar(const std::vector<CalendarEvent*>& events,
    time_t now,
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

    // Extract date/time components from time_t
    struct tm* timeinfo = localtime(&now);
    int currentDay = timeinfo->tm_mday;
    int currentMonth = timeinfo->tm_mon + 1;
    int currentYear = timeinfo->tm_year + 1900;

    // Generate month calendar data
    MonthCalendar monthCal = generateMonthCalendar(currentYear, currentMonth, events);

    // Set today's date if we're displaying the current month
    if (currentYear > 1970) {
        monthCal.today = currentDay;
    }

    // Format month and year string - handle invalid dates
    String monthYear;
    if (currentYear <= 1970 || currentMonth < 1 || currentMonth > 12) {
        monthYear = "---";
    } else {
        monthYear = String(MONTH_NAMES[currentMonth]) + " " + String(currentYear);
    }

    display.firstPage();

    do {
        fillScreen(GxEPD_WHITE);

#if DISPLAY_ORIENTATION == LANDSCAPE
        // LANDSCAPE LAYOUT: Vertical split with calendar on left, events on right

        // Draw vertical divider
        drawDivider();

        // LEFT SIDE: Header and Calendar
        drawLandscapeHeader(now, weatherData);
        drawLandscapeCalendar(monthCal, events);

        // RIGHT SIDE: Events at top, weather at bottom
        drawLandscapeEvents(events);

        int separatorY = WEATHER_START_Y - 9;
        display.drawLine(RIGHT_START_X + 10, separatorY, DISPLAY_WIDTH - 10, separatorY, GxEPD_BLACK);

        if (weatherData && !weatherData->dailyForecast.empty()) {
            drawLandscapeWeather(*weatherData);
        } else {
            drawLandscapeWeatherPlaceholder();
        }

        // Status bar at bottom
        drawLandscapeStatusBar(wifiConnected, rssi, batteryVoltage, batteryPercentage, now, isStale);

#elif DISPLAY_ORIENTATION == PORTRAIT
        // PORTRAIT LAYOUT: Calendar on top, events below with weather on left

        // TOP: Header and Calendar
        drawPortraitHeader(now, weatherData);
        drawPortraitCalendar(monthCal, events);

        // Horizontal divider
        // int dividerY = EVENTS_START_Y - 10;
        // display.drawLine(10, dividerY, DISPLAY_WIDTH - 10, dividerY, GxEPD_BLACK);

        // BOTTOM: Weather on left, Events on right
        drawPortraitEventsWithWeather(events, weatherData);

        // Status bar at bottom
        drawPortraitStatusBar(wifiConnected, rssi, batteryVoltage, batteryPercentage, now, isStale);
#endif

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