#ifndef LANG_IT_H
#define LANG_IT_H

// Italian language strings
// This file is included at compile time based on DISPLAY_LANGUAGE setting

// Month names
#define LOC_JANUARY "Gennaio"
#define LOC_FEBRUARY "Febbraio"
#define LOC_MARCH "Marzo"
#define LOC_APRIL "Aprile"
#define LOC_MAY "Maggio"
#define LOC_JUNE "Giugno"
#define LOC_JULY "Luglio"
#define LOC_AUGUST "Agosto"
#define LOC_SEPTEMBER "Settembre"
#define LOC_OCTOBER "Ottobre"
#define LOC_NOVEMBER "Novembre"
#define LOC_DECEMBER "Dicembre"

// Short month names
#define LOC_JAN "Gen"
#define LOC_FEB "Feb"
#define LOC_MAR "Mar"
#define LOC_APR "Apr"
#define LOC_MAY_SHORT "Mag"
#define LOC_JUN "Giu"
#define LOC_JUL "Lug"
#define LOC_AUG "Ago"
#define LOC_SEP "Set"
#define LOC_OCT "Ott"
#define LOC_NOV "Nov"
#define LOC_DEC "Dic"

// Day names
#define LOC_SUNDAY "Domenica"
#define LOC_MONDAY "Lunedi"
#define LOC_TUESDAY "Martedi"
#define LOC_WEDNESDAY "Mercoledi"
#define LOC_THURSDAY "Giovedi"
#define LOC_FRIDAY "Venerdi"
#define LOC_SATURDAY "Sabato"

// Short day names
#define LOC_SUN "Dom"
#define LOC_MON "Lun"
#define LOC_TUE "Mar"
#define LOC_WED "Mer"
#define LOC_THU "Gio"
#define LOC_FRI "Ven"
#define LOC_SAT "Sab"

// Event display
#define LOC_EVENTS "Eventi"
#define LOC_NO_EVENTS "Nessun Evento"
#define LOC_ENJOY_FREE_DAY "Goditi la tua giornata libera!"
#define LOC_TODAY "Oggi"
#define LOC_TOMORROW "Domani"
#define LOC_UPCOMING "Prossimi"
#define LOC_ALL_DAY "Tutto il giorno"
#define LOC_MORE_EVENTS "altri"
#define LOC_WEATHER_FORECAST "Previsioni Meteo"
#define LOC_WEATHER_COMING_SOON "Dati meteo in arrivo..."
#define LOC_NO_WEATHER_DATA "Dati meteo non disponibili"
#define LOC_TEMPERATURE "Temperatura"
#define LOC_HUMIDITY "Umidità"

// System messages
#define LOC_CALENDAR_DISPLAY "Calendario"
#define LOC_STARTING_UP "Avvio in corso..."
#define LOC_CONNECTING "Connessione"
#define LOC_WIFI_PREFIX "WiFi: "
#define LOC_SYNCING_TIME "Sincronizzazione Ora"
#define LOC_NTP_SERVER "Server NTP"
#define LOC_UPDATING "Aggiornamento"
#define LOC_FETCHING_CALENDAR "Recupero calendario..."
#define LOC_ERROR "Errore"
#define LOC_WIFI_CONNECTION_FAILED "Connessione WiFi fallita"
#define LOC_WIFI_RECONNECTION_FAILED "Riconnessione WiFi fallita"
#define LOC_FAILED_TO_FETCH_CALENDAR "Recupero calendario fallito"
#define LOC_DISPLAY_TEST_SUCCESSFUL "Test Display Riuscito!"
#define LOC_E_PAPER_CALENDAR "Calendario E-Paper"

// Status
#define LOC_UPDATED "Aggiornato: "

// Error messages (using English as fallback for now)
#define LOC_ERROR_WIFI_CONNECTION_FAILED "Connessione WiFi fallita"
#define LOC_ERROR_WIFI_DISCONNECTED "WiFi disconnesso"
#define LOC_ERROR_WIFI_WEAK_SIGNAL "Segnale WiFi troppo debole"
#define LOC_ERROR_WIFI_SSID_NOT_FOUND "Rete WiFi non trovata"
#define LOC_ERROR_WIFI_WRONG_PASSWORD "Password WiFi errata"
#define LOC_ERROR_WIFI_DHCP_FAILED "Impossibile ottenere indirizzo IP"
#define LOC_ERROR_WIFI_RECONNECTION_FAILED "Riconnessione WiFi fallita"

#define LOC_ERROR_CALENDAR_FETCH_FAILED "Recupero calendario fallito"
#define LOC_ERROR_CALENDAR_PARSE_ERROR "Dati calendario non validi"
#define LOC_ERROR_CALENDAR_AUTH_FAILED "Autenticazione calendario fallita"
#define LOC_ERROR_CALENDAR_URL_INVALID "URL calendario non valido"
#define LOC_ERROR_CALENDAR_TIMEOUT "Timeout richiesta calendario"
#define LOC_ERROR_CALENDAR_NO_EVENTS "Nessun evento trovato"
#define LOC_ERROR_CALENDAR_TOO_MANY_EVENTS "Troppi eventi da visualizzare"

#define LOC_ERROR_NTP_SYNC_FAILED "Sincronizzazione ora fallita"
#define LOC_ERROR_NTP_SERVER_UNREACHABLE "Server ora non raggiungibile"
#define LOC_ERROR_TIME_NOT_SET "Ora non impostata"
#define LOC_ERROR_TIMEZONE_ERROR "Errore configurazione fuso orario"

#define LOC_ERROR_DISPLAY_INIT_FAILED "Inizializzazione display fallita"
#define LOC_ERROR_DISPLAY_UPDATE_FAILED "Aggiornamento display fallito"
#define LOC_ERROR_DISPLAY_BUSY_TIMEOUT "Timeout display occupato"

#define LOC_ERROR_BATTERY_LOW "Batteria scarica"
#define LOC_ERROR_BATTERY_CRITICAL "Batteria critica - spegnimento"
#define LOC_ERROR_BATTERY_MONITOR_FAILED "Monitor batteria fallito"

#define LOC_ERROR_MEMORY_LOW "Memoria insufficiente"
#define LOC_ERROR_MEMORY_ALLOCATION_FAILED "Allocazione memoria fallita"

#define LOC_ERROR_NETWORK_TIMEOUT "Timeout rete"
#define LOC_ERROR_NETWORK_DNS_FAILED "Ricerca DNS fallita"
#define LOC_ERROR_NETWORK_SSL_FAILED "Connessione SSL fallita"
#define LOC_ERROR_HTTP_ERROR "Richiesta HTTP fallita"

#define LOC_ERROR_CONFIG_MISSING "Configurazione mancante"
#define LOC_ERROR_CONFIG_INVALID "Configurazione non valida"
#define LOC_ERROR_CONFIG_WIFI_NOT_SET "WiFi non configurato"
#define LOC_ERROR_CONFIG_CALENDAR_NOT_SET "Calendario non configurato"

#define LOC_ERROR_OTA_UPDATE_AVAILABLE "Aggiornamento disponibile"
#define LOC_ERROR_OTA_UPDATE_FAILED "Aggiornamento fallito"
#define LOC_ERROR_OTA_DOWNLOAD_FAILED "Download fallito"
#define LOC_ERROR_OTA_VERIFICATION_FAILED "Verifica aggiornamento fallita"

#define LOC_ERROR_SYSTEM_RESTART_REQUIRED "Riavvio richiesto"
#define LOC_ERROR_SYSTEM_UNKNOWN_ERROR "Errore sconosciuto"

// Error levels
#define LOC_ERROR_LEVEL_INFO "Info"
#define LOC_ERROR_LEVEL_WARNING "Avviso"
#define LOC_ERROR_LEVEL_ERROR "Errore"
#define LOC_ERROR_LEVEL_CRITICAL "Errore Critico"

// Error actions
#define LOC_ERROR_RETRYING "Riprovo..."
#define LOC_ERROR_PLEASE_WAIT "Attendere prego..."
#define LOC_ERROR_CHECK_SETTINGS "Controlla impostazioni"
#define LOC_ERROR_RESTART_DEVICE "Riavvia dispositivo"

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

#endif // LANG_IT_H