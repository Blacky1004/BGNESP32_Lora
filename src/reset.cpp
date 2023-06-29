#include "globals.h"
#include "reset.h"

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