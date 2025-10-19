#include "weather_client.h"
#include <assets/icons/icons.h>

static const char OPEN_WEATHER_REST_URL[] = "https://api.open-meteo.com/v1/forecast?latitude={LOC_LATITUDE}&longitude={LOC_LONGITUDE}&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset&hourly=temperature_2m,weather_code,apparent_temperature,precipitation_probability&current=apparent_temperature,is_day,weather_code,precipitation&timezone=auto&forecast_days=3";

WeatherClient::WeatherClient(WiFiClientSecure* wifiClient) : client(wifiClient) {
    if (client) {
        client->setInsecure(); // Skip certificate validation for simplicity
    }
    // Initialize with default location from config
    latitude = LOC_LATITUDE;
    longitude = LOC_LONGITUDE;
}

WeatherClient::~WeatherClient() {
    // Client is managed externally, don't delete
}

void WeatherClient::setLocation(float lat, float lon) {
    latitude = lat;
    longitude = lon;
    Serial.println("Weather location set to: " + String(latitude, 6) + ", " + String(longitude, 6));
}

String WeatherClient::buildWeatherUrl() {
    String url = String(OPEN_WEATHER_REST_URL);

    // Replace latitude and longitude placeholders with runtime values
    url.replace("{LOC_LATITUDE}", String(latitude, 6));
    url.replace("{LOC_LONGITUDE}", String(longitude, 6));

    return url;
}

bool WeatherClient::fetchWeather(WeatherData& data) {
    if (!client) {
        Serial.println("Weather client not initialized");
        return false;
    }

    HTTPClient http;
    String url = buildWeatherUrl();

    Serial.println("Fetching weather from: " + url);

    http.begin(*client, url);
    http.setTimeout(10000); // 10 second timeout

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Weather data received, parsing...");

        bool success = parseWeatherData(payload, data);
        http.end();
        return success;
    } else {
        Serial.println("Weather fetch failed, HTTP code: " + String(httpCode));
        if (httpCode > 0) {
            Serial.println("Error: " + http.getString());
        }
        http.end();
        return false;
    }
}

bool WeatherClient::parseWeatherData(const String& jsonData, WeatherData& data) {
    // Use PSRAM if available for large JSON document
    DynamicJsonDocument doc(16384);

    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        Serial.println("Failed to parse weather JSON: " + String(error.c_str()));
        return false;
    }

    // Parse current weather
    if (doc.containsKey("current")) {
        JsonObject current = doc["current"];
        data.currentTemp = current["apparent_temperature"];
        data.currentWeatherCode = current["weather_code"];
        data.isDay = current["is_day"] == 1;
    }

    // Parse hourly forecast (7 items starting at 6am with 3-hour intervals)
    data.hourlyForecast.clear();
    if (doc.containsKey("hourly")) {
        JsonObject hourly = doc["hourly"];
        JsonArray times = hourly["time"];
        JsonArray temps = hourly["temperature_2m"];
        JsonArray codes = hourly["weather_code"];
        JsonArray precip = hourly["precipitation_probability"];

        // Get current hour
        time_t now;
        time(&now);
        struct tm* timeinfo = localtime(&now);
        int currentHour = timeinfo->tm_hour;

        // Start at 6am for the forecast
        int startHour = 6;

        // Find today's data starting from startHour
        int todayStartIdx = -1;
        String todayDate = "";

        for (int i = 0; i < times.size(); i++) {
            String timeStr = times[i].as<String>();
            String date = timeStr.substring(0, 10);
            int hour = timeStr.substring(11, 13).toInt();

            // Get today's date from first entry
            if (i == 0 || todayDate.isEmpty()) {
                if (hour <= currentHour) {
                    todayDate = date;
                }
            }

            // Find first entry matching our start hour on today's date
            if (date == todayDate && hour == startHour) {
                todayStartIdx = i;
                break;
            }
        }

        // If we couldn't find today's start hour, use current hour
        if (todayStartIdx < 0) {
            for (int i = 0; i < times.size(); i++) {
                String timeStr = times[i].as<String>();
                int hour = timeStr.substring(11, 13).toInt();
                if (hour >= currentHour) {
                    todayStartIdx = i;
                    break;
                }
            }
        }

        if (todayStartIdx >= 0) {
            // Get 7 data points at 3-hour intervals starting from 6am
            // This gives us: 6, 9, 12, 15, 18, 21, 0 (next day)
            int added = 0;
            for (int i = 0; i < 21 && added < 7; i++) {
                int idx = todayStartIdx + (i * 3); // 3-hour intervals

                if (idx >= times.size()) break;

                WeatherHour hour;
                hour.time = times[idx].as<String>();
                hour.temperature = temps[idx];
                hour.weatherCode = codes[idx];
                hour.precipitationProbability = precip[idx];

                // Determine if it's day or night based on hour
                int h = hour.time.substring(11, 13).toInt();
                hour.isDay = (h >= 6 && h < 18);

                data.hourlyForecast.push_back(hour);
                added++;
            }
        }
    }

    // Parse daily forecast
    data.dailyForecast.clear();
    if (doc.containsKey("daily")) {
        JsonObject daily = doc["daily"];
        JsonArray dates = daily["time"];
        JsonArray codes = daily["weather_code"];
        JsonArray maxTemps = daily["temperature_2m_max"];
        JsonArray minTemps = daily["temperature_2m_min"];
        JsonArray sunrises = daily["sunrise"];
        JsonArray sunsets = daily["sunset"];

        for (int i = 0; i < dates.size() && i < 3; i++) {
            WeatherDay day;
            day.date = dates[i].as<String>();
            day.weatherCode = codes[i];
            day.tempMax = maxTemps[i];
            day.tempMin = minTemps[i];
            day.sunrise = sunrises[i].as<String>();
            day.sunset = sunsets[i].as<String>();

            data.dailyForecast.push_back(day);
        }
    }

    Serial.println("Weather data parsed successfully");
    Serial.println("Current temp: " + String(data.currentTemp) + "°C");
    Serial.println("Hourly forecast items: " + String(data.hourlyForecast.size()));
    Serial.println("Daily forecast items: " + String(data.dailyForecast.size()));

    return true;
}

// Macro to select icon based on size
#define GET_WEATHER_ICON(name, size) \
    ((size) == 16 ? name##_16x16 : \
     (size) == 24 ? name##_24x24 : \
     (size) == 48 ? name##_48x48 : \
     (size) == 64 ? name##_64x64 : \
     (size) == 96 ? name##_96x96 : \
     (size) == 128 ? name##_128x128 : \
     (size) == 160 ? name##_160x160 : \
     (size) == 196 ? name##_196x196 : \
     name##_32x32)

const uint8_t* WeatherClient::getWeatherIconBitmap(int weatherCode, bool isDay, int size) {
    // Return appropriate icon based on weather code
    switch (weatherCode) {
        case 0: // Clear sky
            return isDay ? GET_WEATHER_ICON(wi_day_sunny, size)
                         : GET_WEATHER_ICON(wi_night_clear, size);

        case 1: // Mainly clear
        case 2: // Partly cloudy
            return isDay ? GET_WEATHER_ICON(wi_day_cloudy, size)
                         : GET_WEATHER_ICON(wi_night_alt_cloudy, size);

        case 3: // Overcast
            return GET_WEATHER_ICON(wi_cloudy, size);

        case 45: // Fog
        case 48: // Depositing rime fog
            return GET_WEATHER_ICON(wi_fog, size);

        case 51: // Light drizzle
        case 53: // Moderate drizzle
        case 55: // Dense drizzle
            return GET_WEATHER_ICON(wi_sprinkle, size);

        case 56: // Light freezing drizzle
        case 57: // Dense freezing drizzle
            return GET_WEATHER_ICON(wi_sleet, size);

        case 61: // Slight rain
        case 63: // Moderate rain
            return isDay ? GET_WEATHER_ICON(wi_day_rain, size)
                         : GET_WEATHER_ICON(wi_night_alt_rain, size);

        case 65: // Heavy rain
            return GET_WEATHER_ICON(wi_rain, size);

        case 66: // Light freezing rain
        case 67: // Heavy freezing rain
            return GET_WEATHER_ICON(wi_sleet, size);

        case 71: // Slight snow
        case 73: // Moderate snow
        case 75: // Heavy snow
            return GET_WEATHER_ICON(wi_snow, size);

        case 77: // Snow grains
            return GET_WEATHER_ICON(wi_snow, size);

        case 80: // Slight rain showers
        case 81: // Moderate rain showers
            return isDay ? GET_WEATHER_ICON(wi_day_showers, size)
                         : GET_WEATHER_ICON(wi_night_alt_showers, size);

        case 82: // Violent rain showers
            return GET_WEATHER_ICON(wi_rain, size);

        case 85: // Slight snow showers
        case 86: // Heavy snow showers
            return GET_WEATHER_ICON(wi_snow, size);

        case 95: // Thunderstorm
            return GET_WEATHER_ICON(wi_thunderstorm, size);

        case 96: // Thunderstorm with slight hail
        case 99: // Thunderstorm with heavy hail
            return GET_WEATHER_ICON(wi_thunderstorm, size);

        default:
            return GET_WEATHER_ICON(wi_na, size); // Not available icon
    }
}

#undef GET_WEATHER_ICON

String WeatherClient::getWeatherDescription(int weatherCode) {
    switch (weatherCode) {
        case 0: return "Clear sky";
        case 1: return "Mainly clear";
        case 2: return "Partly cloudy";
        case 3: return "Overcast";
        case 45: return "Fog";
        case 48: return "Rime fog";
        case 51: return "Light drizzle";
        case 53: return "Moderate drizzle";
        case 55: return "Dense drizzle";
        case 56: return "Light freezing drizzle";
        case 57: return "Dense freezing drizzle";
        case 61: return "Slight rain";
        case 63: return "Moderate rain";
        case 65: return "Heavy rain";
        case 66: return "Light freezing rain";
        case 67: return "Heavy freezing rain";
        case 71: return "Slight snow";
        case 73: return "Moderate snow";
        case 75: return "Heavy snow";
        case 77: return "Snow grains";
        case 80: return "Slight rain showers";
        case 81: return "Moderate rain showers";
        case 82: return "Violent rain showers";
        case 85: return "Slight snow showers";
        case 86: return "Heavy snow showers";
        case 95: return "Thunderstorm";
        case 96: return "Thunderstorm with hail";
        case 99: return "Heavy thunderstorm";
        default: return "Unknown";
    }
}