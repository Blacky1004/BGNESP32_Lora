#include "timekeeper.h"

// G = GPS / R = RTC / L = LORA / ? = unsynced / * = set
const char timeSetSymbols[] = {'G', 'R', 'L', '?', '*'};

DRAM_ATTR bool TimePulseTick = false;
#ifdef GPS_INT
    DRAM_ATTR unsigned long lastPPS = millis();
#endif
#ifdef RTC_INT
    DRAM_ATTR unsigned long lastRTCpulse = millis();
#endif

timesource_t timeSource = _unsynced;



bool timeIsValid(time_t const t) {
  // is t a time in the past? we use compile time to guess
  // compile time is some local time, but we do not know it's time zone
  // thus, we go 1 full day back to be sure to catch a time in the past
  return (t > (compileTime() - 86400));
}

time_t compileTime(void) {
    char s_month[5];
    int year;
    struct tm t = {0};
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    static time_t secs = -1;

    if (secs == -1) {
    // determine date
    sscanf(__DATE__, "%4s %d %d", s_month, &t.tm_mday, &year);
    t.tm_mon = (strstr(month_names, s_month) - month_names) / 3;
    t.tm_year = year - 1900;
    // determine time
    sscanf(__TIME__, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);

    // convert to secs local time
    secs = mktime(&t);
    }

    return secs;
}