#ifndef _RESET_H
#define _RESET_H

#include <driver/rtc_io.h>
#include <soc/reset_reasons.h>

#include "lorawan.h"

void reset_rtc_vars(void);
void do_reset(bool warmstart);
void do_after_reset(void);
void enter_deepsleep(uint32_t wakeup_sec, const gpio_num_t wakeup_gpio);
uint64_t uptime(void);

enum runmode_t {
    RUNMODE_POWERCYCLE,
    RUNMODE_NORMAL,
    RUNMODE_WAKEUP,
    RUNMODE_UPDATE,
    RUNMODE_SLEEP,
    RUNMODE_MAINTENANCE
};

extern RTC_NOINIT_ATTR runmode_t RTC_runmode;
extern RTC_NOINIT_ATTR uint32_t RTC_restarts;
#endif