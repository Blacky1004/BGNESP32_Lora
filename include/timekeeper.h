#ifndef _timekeeper_H
#define _timekeeper_H

#include "globals.h"

#include "esp_sntp.h"

#define HAS_LORA_TIME ((HAS_LORA) && ((TIME_SYNC_LORASERVER) || (TIME_SYNC_LORAWAN)))
#define HAS_TIME (TIME_SYNC_INTERVAL) && (HAS_LORA_TIME || HAS_GPS)

enum timesource_t { _gps, _rtc, _lora, _unsynced, _set };


extern timesource_t timeSource;

extern DRAM_ATTR bool TimePulseTick; // 1sec pps flag set by GPS or RTC
#ifdef GPS_INT
extern DRAM_ATTR unsigned long lastPPS;
#endif
#ifdef RTC_INT
extern DRAM_ATTR unsigned long lastRTCpulse;
#endif


bool timeIsValid(time_t const t);

time_t compileTime(void);
#endif