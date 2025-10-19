#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"

struct WeatherHour {
    String time;        // ISO format time
    float temperature;  // Temperature in Celsius
    int weatherCode;    // WMO weather code
    int precipitationProbability; // Precipitation probability in %
    bool isDay;        // Day or night (for icon selection)
};

struct WeatherDay {
    String date;       // ISO format date
    int weatherCode;   // Daily weather code
    float tempMax;     // Maximum temperature
    float tempMin;     // Minimum temperature
    String sunrise;    // Sunrise time
    String sunset;     // Sunset time
};

struct WeatherData {
    // Current weather
    float currentTemp;
    int currentWeatherCode;
    bool isDay;

    // Hourly forecast (7 items at 3-hour intervals)
    std::vector<WeatherHour> hourlyForecast;

    // Daily forecast (next 3 days)
    std::vector<WeatherDay> dailyForecast;
};

class WeatherClient {
private:
    WiFiClientSecure* client;
    float latitude;
    float longitude;

    // Map weather code to icon name
    const char* getWeatherIcon(int weatherCode, bool isDay = true);

    // Parse JSON response
    bool parseWeatherData(const String& jsonData, WeatherData& data);

    // Build URL with coordinates
    String buildWeatherUrl();

public:
    WeatherClient(WiFiClientSecure* wifiClient);
    ~WeatherClient();

    // Set location for weather fetching
    void setLocation(float lat, float lon);

    // Fetch weather data from API
    bool fetchWeather(WeatherData& data);

    // Get weather icon for display (supports 16x16, 24x24, 32x32 sizes)
    const uint8_t* getWeatherIconBitmap(int weatherCode, bool isDay = true, int size = 32);

    // Get weather description
    String getWeatherDescription(int weatherCode);
};

#endif // WEATHER_CLIENT_H