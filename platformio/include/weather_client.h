#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <vector>

/**
 * @brief Weather forecast data for a single day
 *
 * Contains all weather information for one day including temperature range,
 * weather conditions, sunrise/sunset times, and precipitation probability.
 * Used by Open-Meteo weather API integration.
 */
struct WeatherDay {
    String date;                  ///< ISO format date (YYYY-MM-DD)
    int weatherCode;              ///< WMO weather code (0-99)
    float tempMax;                ///< Maximum temperature in degrees Celsius
    float tempMin;                ///< Minimum temperature in degrees Celsius
    String sunrise;               ///< Sunrise time in ISO format (YYYY-MM-DDTHH:MM)
    String sunset;                ///< Sunset time in ISO format (YYYY-MM-DDTHH:MM)
    int precipitationProbability; ///< Max precipitation probability for the day (0-100%)
};

/**
 * @brief Complete weather data structure
 *
 * Contains current weather conditions and daily forecast for the next 3 days.
 * Populated by WeatherClient::fetchWeather() from Open-Meteo API.
 */
struct WeatherData {
    // Current weather
    float currentTemp;      ///< Current temperature in degrees Celsius
    int currentWeatherCode; ///< Current WMO weather code
    bool isDay;             ///< True if currently daytime (for icon selection)

    // Daily forecast (next 3 days)
    std::vector<WeatherDay> dailyForecast; ///< Vector of daily forecasts (today + 2 days)
};

/**
 * @brief Client for fetching weather data from Open-Meteo API
 *
 * WeatherClient handles communication with the Open-Meteo weather service,
 * parsing weather data, and providing weather icons for display. It supports
 * fetching current weather conditions and daily forecasts for up to 3 days.
 *
 * Features:
 * - Current weather conditions (temperature, weather code, day/night)
 * - Daily forecast with min/max temps, precipitation probability
 * - Sunrise and sunset times
 * - WMO weather code mapping to descriptive icons
 * - Multiple icon sizes (16x16, 24x24, 32x32, 48x48, 64x64)
 * - Day/night icon variants
 *
 * API: Uses Open-Meteo free weather API (no API key required)
 * Endpoint: https://api.open-meteo.com/v1/forecast
 */
class WeatherClient {
  private:
    WiFiClientSecure* client; ///< Secure WiFi client for HTTPS requests
    float latitude;           ///< Location latitude for weather queries
    float longitude;          ///< Location longitude for weather queries

    /**
     * @brief Map WMO weather code to icon name
     *
     * Converts numeric weather codes (0-99) to descriptive icon names
     * like "sunny", "cloudy", "rainy", etc. Supports day/night variants.
     *
     * @param weatherCode WMO weather code (0-99)
     * @param isDay true for day icons, false for night icons
     * @return Icon name string (e.g., "wi_day_sunny", "wi_night_cloudy")
     */
    const char* getWeatherIcon(int weatherCode, bool isDay = true);

    /**
     * @brief Parse JSON response from Open-Meteo API
     *
     * Parses the JSON response and populates WeatherData structure with
     * current conditions and daily forecasts.
     *
     * @param jsonData Raw JSON string from API response
     * @param data Output WeatherData structure to populate
     * @return true if parsing succeeded
     */
    bool parseWeatherData(const String& jsonData, WeatherData& data);

    /**
     * @brief Build complete API URL with coordinates
     *
     * Constructs the full Open-Meteo API URL including latitude, longitude,
     * timezone, current weather fields, and daily forecast parameters.
     *
     * @return Complete API URL string
     */
    String buildWeatherUrl();

  public:
    /**
     * @brief Construct a new Weather Client object
     * @param wifiClient Pointer to WiFiClientSecure for HTTPS requests
     */
    WeatherClient(WiFiClientSecure* wifiClient);

    /**
     * @brief Destroy the Weather Client object
     */
    ~WeatherClient();

    /**
     * @brief Set geographic location for weather queries
     * @param lat Latitude in decimal degrees
     * @param lon Longitude in decimal degrees
     */
    void setLocation(float lat, float lon);

    /**
     * @brief Fetch weather data from Open-Meteo API
     *
     * Makes HTTPS request to Open-Meteo API and parses the response.
     * Populates the provided WeatherData structure with current conditions
     * and 3-day forecast.
     *
     * @param data Output WeatherData structure to populate
     * @return true if fetch and parse succeeded
     */
    bool fetchWeather(WeatherData& data);

    /**
     * @brief Get weather icon bitmap for display
     *
     * Returns pointer to bitmap data for the specified weather code and size.
     * Supports day/night variants for appropriate icons (sun/moon, etc.).
     *
     * @param weatherCode WMO weather code (0-99)
     * @param isDay true for day icons, false for night icons
     * @param size Icon size in pixels (16, 24, 32, 48, or 64)
     * @return Pointer to icon bitmap data or nullptr if not found
     */
    const uint8_t* getWeatherIconBitmap(int weatherCode, bool isDay = true, int size = 32);

    /**
     * @brief Get human-readable weather description
     *
     * Converts WMO weather code to localized text description.
     *
     * @param weatherCode WMO weather code (0-99)
     * @return Weather description string (e.g., "Sunny", "Partly Cloudy", "Rain")
     */
    String getWeatherDescription(int weatherCode);
};

#endif // WEATHER_CLIENT_H