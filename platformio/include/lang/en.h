#ifndef LANG_EN_H
#define LANG_EN_H

// English language strings
// This file is included at compile time based on DISPLAY_LANGUAGE setting

// Month names
#define LOC_JANUARY "January"
#define LOC_FEBRUARY "February"
#define LOC_MARCH "March"
#define LOC_APRIL "April"
#define LOC_MAY "May"
#define LOC_JUNE "June"
#define LOC_JULY "July"
#define LOC_AUGUST "August"
#define LOC_SEPTEMBER "September"
#define LOC_OCTOBER "October"
#define LOC_NOVEMBER "November"
#define LOC_DECEMBER "December"

// Short month names
#define LOC_JAN "Jan"
#define LOC_FEB "Feb"
#define LOC_MAR "Mar"
#define LOC_APR "Apr"
#define LOC_MAY_SHORT "May"
#define LOC_JUN "Jun"
#define LOC_JUL "Jul"
#define LOC_AUG "Aug"
#define LOC_SEP "Sep"
#define LOC_OCT "Oct"
#define LOC_NOV "Nov"
#define LOC_DEC "Dec"

// Day names
#define LOC_SUNDAY "Sunday"
#define LOC_MONDAY "Monday"
#define LOC_TUESDAY "Tuesday"
#define LOC_WEDNESDAY "Wednesday"
#define LOC_THURSDAY "Thursday"
#define LOC_FRIDAY "Friday"
#define LOC_SATURDAY "Saturday"

// Short day names
#define LOC_SUN "Sun"
#define LOC_MON "Mon"
#define LOC_TUE "Tue"
#define LOC_WED "Wed"
#define LOC_THU "Thu"
#define LOC_FRI "Fri"
#define LOC_SAT "Sat"

// Event display
#define LOC_EVENTS "Events"
#define LOC_NO_EVENTS "No Events"
#define LOC_ENJOY_FREE_DAY "Enjoy your free day!"
#define LOC_TODAY "Today"
#define LOC_TOMORROW "Tomorrow"
#define LOC_UPCOMING "Upcoming"
#define LOC_ALL_DAY "All Day"
#define LOC_MORE_EVENTS "more"
#define LOC_WEATHER_FORECAST "Weather Forecast"
#define LOC_WEATHER_COMING_SOON "Weather data coming soon..."
#define LOC_NO_WEATHER_DATA "No weather data available"
#define LOC_TEMPERATURE "Temperature"
#define LOC_HUMIDITY "Humidity"
#define LOC_RAIN "rain"

// System messages
#define LOC_CALENDAR_DISPLAY "Calendar Display"
#define LOC_STARTING_UP "Starting up..."
#define LOC_CONNECTING "Connecting"
#define LOC_WIFI_PREFIX "WiFi: "
#define LOC_SYNCING_TIME "Syncing Time"
#define LOC_NTP_SERVER "NTP Server"
#define LOC_UPDATING "Updating"
#define LOC_FETCHING_CALENDAR "Fetching calendar..."
#define LOC_ERROR "Error"
#define LOC_WIFI_CONNECTION_FAILED "WiFi connection failed"
#define LOC_WIFI_RECONNECTION_FAILED "WiFi reconnection failed"
#define LOC_FAILED_TO_FETCH_CALENDAR "Failed to fetch calendar"
#define LOC_DISPLAY_TEST_SUCCESSFUL "Display Test Successful!"
#define LOC_E_PAPER_CALENDAR "E-Paper Calendar"

// Status
#define LOC_UPDATED "Updated: "

// Error messages
#define LOC_ERROR_WIFI_CONNECTION_FAILED "WiFi connection failed"
#define LOC_ERROR_WIFI_DISCONNECTED "WiFi disconnected"
#define LOC_ERROR_WIFI_WEAK_SIGNAL "WiFi signal too weak"
#define LOC_ERROR_WIFI_SSID_NOT_FOUND "WiFi network not found"
#define LOC_ERROR_WIFI_WRONG_PASSWORD "Wrong WiFi password"
#define LOC_ERROR_WIFI_DHCP_FAILED "Failed to obtain IP address"
#define LOC_ERROR_WIFI_RECONNECTION_FAILED "WiFi reconnection failed"

#define LOC_ERROR_CALENDAR_FETCH_FAILED "Failed to fetch calendar"
#define LOC_ERROR_CALENDAR_PARSE_ERROR "Calendar data invalid"
#define LOC_ERROR_CALENDAR_AUTH_FAILED "Calendar authentication failed"
#define LOC_ERROR_CALENDAR_URL_INVALID "Invalid calendar URL"
#define LOC_ERROR_CALENDAR_TIMEOUT "Calendar request timeout"
#define LOC_ERROR_CALENDAR_NO_EVENTS "No calendar events found"
#define LOC_ERROR_CALENDAR_TOO_MANY_EVENTS "Too many events to display"

#define LOC_ERROR_NTP_SYNC_FAILED "Time sync failed"
#define LOC_ERROR_NTP_SERVER_UNREACHABLE "Time server unreachable"
#define LOC_ERROR_TIME_NOT_SET "Time not set"
#define LOC_ERROR_TIMEZONE_ERROR "Timezone configuration error"

#define LOC_ERROR_DISPLAY_INIT_FAILED "Display initialization failed"
#define LOC_ERROR_DISPLAY_UPDATE_FAILED "Display update failed"
#define LOC_ERROR_DISPLAY_BUSY_TIMEOUT "Display busy timeout"

#define LOC_ERROR_BATTERY_LOW "Battery low"
#define LOC_ERROR_BATTERY_CRITICAL "Battery critical - shutting down"
#define LOC_ERROR_BATTERY_MONITOR_FAILED "Battery monitor failed"

#define LOC_ERROR_MEMORY_LOW "Memory low"
#define LOC_ERROR_MEMORY_ALLOCATION_FAILED "Memory allocation failed"

#define LOC_ERROR_NETWORK_TIMEOUT "Network timeout"
#define LOC_ERROR_NETWORK_DNS_FAILED "DNS lookup failed"
#define LOC_ERROR_NETWORK_SSL_FAILED "SSL connection failed"
#define LOC_ERROR_HTTP_ERROR "HTTP request failed"

#define LOC_ERROR_CONFIG_MISSING "Configuration missing"
#define LOC_ERROR_CONFIG_INVALID "Invalid configuration"
#define LOC_ERROR_CONFIG_WIFI_NOT_SET "WiFi not configured"
#define LOC_ERROR_CONFIG_CALENDAR_NOT_SET "Calendar not configured"

#define LOC_ERROR_OTA_UPDATE_AVAILABLE "Update available"
#define LOC_ERROR_OTA_UPDATE_FAILED "Update failed"
#define LOC_ERROR_OTA_DOWNLOAD_FAILED "Download failed"
#define LOC_ERROR_OTA_VERIFICATION_FAILED "Update verification failed"

#define LOC_ERROR_SYSTEM_RESTART_REQUIRED "Restart required"
#define LOC_ERROR_SYSTEM_UNKNOWN_ERROR "Unknown error"

// Error levels
#define LOC_ERROR_LEVEL_INFO "Info"
#define LOC_ERROR_LEVEL_WARNING "Warning"
#define LOC_ERROR_LEVEL_ERROR "Error"
#define LOC_ERROR_LEVEL_CRITICAL "Critical Error"

// Error actions
#define LOC_ERROR_RETRYING "Retrying..."
#define LOC_ERROR_PLEASE_WAIT "Please wait..."
#define LOC_ERROR_CHECK_SETTINGS "Check settings"
#define LOC_ERROR_RESTART_DEVICE "Restart device"

// Arrays for convenient access
[[maybe_unused]]
static const char* MONTH_NAMES[] = {
    "", LOC_JANUARY, LOC_FEBRUARY, LOC_MARCH, LOC_APRIL, LOC_MAY, LOC_JUNE,
    LOC_JULY, LOC_AUGUST, LOC_SEPTEMBER, LOC_OCTOBER, LOC_NOVEMBER, LOC_DECEMBER
};

[[maybe_unused]]
static const char* MONTH_NAMES_SHORT[] = {
    "", LOC_JAN, LOC_FEB, LOC_MAR, LOC_APR, LOC_MAY_SHORT, LOC_JUN,
    LOC_JUL, LOC_AUG, LOC_SEP, LOC_OCT, LOC_NOV, LOC_DEC
};

[[maybe_unused]]
static const char* DAY_NAMES[] = {
    LOC_SUNDAY, LOC_MONDAY, LOC_TUESDAY, LOC_WEDNESDAY,
    LOC_THURSDAY, LOC_FRIDAY, LOC_SATURDAY
};

[[maybe_unused]]
static const char* DAY_NAMES_SHORT[] = {
    LOC_SUN, LOC_MON, LOC_TUE, LOC_WED, LOC_THU, LOC_FRI, LOC_SAT
};

#endif // LANG_EN_H