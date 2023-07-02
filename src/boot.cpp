#include "boot.h"
#include "reset.h"

static hw_timer_t *wdTimer = NULL;

static TaskHandle_t RestartHandle;

void IRAM_ATTR watchdog() { xTaskResumeFromISR(RestartHandle); }

void start_boot_menu(void) {
    RTC_runmode = RUNMODE_NORMAL;

    // setup restart handle task for resetting ESP32, which is callable from ISR
  // (because esp_restart() from ISR would trigger the ESP32 task watchdog)
  xTaskCreate(
      [](void *p) {
        vTaskSuspend(NULL); // wait for task resume call by watchdog
        esp_restart();
      },
      "Restart", configMINIMAL_STACK_SIZE, NULL, (3 | portPRIVILEGE_BIT),
      &RestartHandle);

  // setup watchdog, based on esp32 timer2 interrupt
  wdTimer = timerBegin(0, 80, true);              // timer 0, div 80, countup
  timerAttachInterrupt(wdTimer, &watchdog, false); // callback for device reset
  timerAlarmWrite(wdTimer, BOOTDELAY * 1000000, false); // set time in us
  timerAlarmEnable(wdTimer);                            // enable watchdog
}