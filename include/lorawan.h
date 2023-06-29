#if (HAS_LORA)

#ifndef _LORAWAN_H
#define _LORAWAN_H

#include "globals.h"

#include <driver/rtc_io.h>

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <arduino_lmic_hal_boards.h>
#include "loraconf.h"


extern TaskHandle_t lmicTask, lorasendTask;
extern char lmic_event_msg[LMIC_EVENTMSG_LEN];

esp_err_t lmic_init(void);
void lora_setupNetwork(bool preJoin);
void SaveLMICToRTC(uint32_t deepsleep_sec);
void LoadLMICFromRTC();
void lmictask(void *pvParameters);
void gen_lora_devui(uint8_t *pdevui);
void RevBytes(unsigned char *b, size_t c);

#endif //_LORAWAN_H
#endif //HAS_LORA