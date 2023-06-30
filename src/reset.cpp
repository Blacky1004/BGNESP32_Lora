#include "globals.h"
#include "reset.h"
#include <rom/rtc.h>

#define uS_TO_S_FACTOR 1000000ULL

RTC_NOINIT_ATTR runmode_t RTC_runmode;
RTC_NOINIT_ATTR uint32_t RTC_restarts;

RTC_DATA_ATTR struct timeval sleep_start_time;
RTC_DATA_ATTR int64_t RTC_millis = 0;

struct timeval sleep_stop_time;

void reset_rtc_vars(void) {
  RTC_runmode = RUNMODE_POWERCYCLE;
  RTC_restarts = 0;
}


#if (HAS_TIME)
void adjust_wakeup(uint32_t *wakeuptime) {
    if ((timeSource == _unsynced) || (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS)) {
        ESP_LOGI(TAG, "Syncwakeup: Ungültige Zeitangabe für die Synchronisation!");
        return;
    }

    time_t now;
    time(&now);

    uint16_t shift_sec = 3600 - (now + *wakeuptime) % 3600;

    if(shift_sec < SYNCWAKEUP) {
        *wakeuptime += shift_sec;
        ESP_LOGI(TAG, "SyncWakeUp: WakeUp um %hu Sek. verschoben.", shift_sec);
    } else if(shift_sec >= (3600 - SYNCWAKEUP)) {
        *wakeuptime = 3600 - shift_sec;
        ESP_LOGI(TAG, "SyncWakeUp: WakeUp um %hu Sek. vorgezogen.");
    } else 
        ESP_LOGI(TAG, "SyncWakeUp: WakeUp bleibt unverändert.");
}
#endif

void do_reset(bool warmstart) {
    if(warmstart) {
        ESP_LOGI(TAG,"Starte System neu (WARMUP)");        
    } else {
        #if (HAS_LORA) 
        if(RTC_runmode == RUNMODE_NORMAL){
            LMIC_shutdown();
        }
        #endif
        reset_rtc_vars();
        ESP_LOGI(TAG, "Starte System neu (COLDUP)");
    }
    esp_restart();
}

void do_after_reset(void) {
    struct timeval sleep_stop_time;
    int64_t sleep_time_ms;

    loadConfig();

    #ifdef TIME_SYNC_TIMEZONE
    setenv("TZ", TIME_SYNC_TIMEZONE, 1);
    tzset();
    ESP_LOGD(TAG, "setze Zeitzone auf %s", TIME_SYNC_TIMEZONE);
    #endif

    switch (rtc_get_reset_reason(0))
    {
        case RESET_REASON_CHIP_POWER_ON:
        case RESET_REASON_SYS_BROWN_OUT:
            reset_rtc_vars();
        break;
        case RESET_REASON_CPU0_SW:        
            RTC_restarts++;
        break;
        case RESET_REASON_CORE_DEEP_SLEEP:
            gettimeofday(&sleep_stop_time, NULL);
            sleep_time_ms = ((int64_t)sleep_stop_time.tv_sec * 1000000L +
                     (int64_t)sleep_stop_time.tv_usec -
                     (int64_t)sleep_start_time.tv_sec * 1000000L -
                     (int64_t)sleep_start_time.tv_usec) /
                    1000LL;
            RTC_millis += sleep_time_ms;
            ESP_LOGI(TAG,"Verbrachte Zeit im Tiefschlaf: %llu ms", sleep_time_ms);
            timeSource = timeIsValid(sleep_stop_time.tv_sec) ? _set : _unsynced;
            if(RTC_runmode == RUNMODE_SLEEP)
                RTC_runmode = RUNMODE_WAKEUP;
        break;
    default:
        RTC_runmode = RUNMODE_POWERCYCLE;
        RTC_restarts++;
        break;
    }
}