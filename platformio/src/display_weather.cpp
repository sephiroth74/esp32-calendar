#include "display_manager.h"
#include "localization.h"
#include "debug_config.h"
#include "weather_client.h"
#include <assets/fonts.h>
#include <assets/icons/icons.h>

// ============================================================================
// Header and Weather Drawing Functions
// ============================================================================

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
            // Draw sunrise icon (24x24)
            display.drawInvertedBitmap(10, 8, wi_sunrise_24x24, 24, 24, GxEPD_BLACK);
            // Draw sunrise time next to icon
            display.setCursor(38, 22);  // Position text next to icon (adjusted for 24px icon)
            display.print(sunriseTime);
        }

        // Draw sunset at top-right corner with icon
        if (today.sunset.length() >= 16) {
            String sunsetTime = today.sunset.substring(11, 16);  // Extract HH:MM
            int16_t textWidth = getTextWidth(sunsetTime, &FONT_SUNRISE_SUNSET);
            // Calculate position for right alignment
            int iconX = LEFT_WIDTH - textWidth - 38;  // Account for 24px icon and text width
            // Draw sunset icon (24x24)
            display.drawInvertedBitmap(iconX, 8, wi_sunset_24x24, 24, 24, GxEPD_BLACK);
            // Draw sunset time next to icon
            display.setCursor(iconX + 28, 22);  // Position text next to icon (adjusted for 24px icon)
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

void DisplayManager::drawWeatherPlaceholder()
{
    // Draw placeholder for weather forecast in bottom right
    int x = RIGHT_START_X + 20;
    int y = WEATHER_START_Y + 6;  // Move down 6 pixels

    // Display placeholder weather icon (48x48)
#ifdef DISP_TYPE_6C
    // Use red for weather icon on 6-color displays
    display.drawInvertedBitmap(x + 10, y - 14, wi_na_48x48, 48, 48, COLOR_WEATHER_ICON);
#else
    display.drawInvertedBitmap(x + 10, y - 14, wi_na_48x48, 48, 48, GxEPD_BLACK);
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
    // Check if we have daily forecast data
    if (weatherData.dailyForecast.empty()) {
        display.setFont(&FONT_WEATHER_MESSAGE);
        int x = RIGHT_START_X + 20;
        int y = WEATHER_START_Y + 50;
        display.setCursor(x, y);
        display.print(LOC_NO_WEATHER_DATA);
        return;
    }

    // Weather section dimensions
    int sectionWidth = RIGHT_WIDTH - 40;  // Total width with margins
    int halfWidth = sectionWidth / 2;     // Width for each day
    int startX = RIGHT_START_X + 20;
    int startY = WEATHER_START_Y + 10;

    WeatherClient tempClient(nullptr);

    // Draw today's weather (left side)
    if (weatherData.dailyForecast.size() > 0) {
        const WeatherDay& today = weatherData.dailyForecast[0];
        int x = startX;
        int y = startY;

        // Weather icon (64x64)
        const uint8_t* todayIcon = tempClient.getWeatherIconBitmap(today.weatherCode, true, 64);
        if (todayIcon) {
#ifdef DISP_TYPE_6C
            display.drawInvertedBitmap(x, y, todayIcon, 64, 64, COLOR_WEATHER_ICON);
#else
            display.drawInvertedBitmap(x, y, todayIcon, 64, 64, GxEPD_BLACK);
#endif
        }

        // Label "Today" to the right of icon (moved 4px left)
        display.setFont(&FONT_WEATHER_LABEL);
        display.setCursor(x + 70, y + 15);
        display.print(LOC_TODAY);

        // Rain percentage below label (moved 4px left)
        display.setFont(&FONT_WEATHER_RAIN);
        display.setCursor(x + 70, y + 35);
        display.print(String(today.precipitationProbability) + "% " + LOC_RAIN);

        // Min/Max temperature below rain percentage (moved 4px left)
        display.setFont(&FONT_WEATHER_TEMP_MAIN);
        String tempRange = String(int(today.tempMin)) + "\260 / " + String(int(today.tempMax)) + "\260";
        display.setCursor(x + 70, y + 55);
        display.print(tempRange);
    }

    // Draw tomorrow's weather (right side)
    if (weatherData.dailyForecast.size() > 1) {
        const WeatherDay& tomorrow = weatherData.dailyForecast[1];
        int x = startX + halfWidth;
        int y = startY;

        // Weather icon (64x64)
        const uint8_t* tomorrowIcon = tempClient.getWeatherIconBitmap(tomorrow.weatherCode, true, 64);
        if (tomorrowIcon) {
#ifdef DISP_TYPE_6C
            display.drawInvertedBitmap(x, y, tomorrowIcon, 64, 64, COLOR_WEATHER_ICON);
#else
            display.drawInvertedBitmap(x, y, tomorrowIcon, 64, 64, GxEPD_BLACK);
#endif
        }

        // Label "Tomorrow" to the right of icon (moved 4px left)
        display.setFont(&FONT_WEATHER_LABEL);
        display.setCursor(x + 70, y + 15);
        display.print(LOC_TOMORROW);

        // Rain percentage below label (moved 4px left)
        display.setFont(&FONT_WEATHER_RAIN);
        display.setCursor(x + 70, y + 35);
        display.print(String(tomorrow.precipitationProbability) + "% " + LOC_RAIN);

        // Min/Max temperature below rain percentage (moved 4px left)
        display.setFont(&FONT_WEATHER_TEMP_MAIN);
        String tempRange = String(int(tomorrow.tempMin)) + "\260 / " + String(int(tomorrow.tempMax)) + "\260";
        display.setCursor(x + 70, y + 55);
        display.print(tempRange);
    }
}
