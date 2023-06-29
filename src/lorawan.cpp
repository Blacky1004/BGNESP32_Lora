#if(HAS_LORA)
#include "lorawan.h"
#if CLOCK_ERROR_PROCENTAGE > 7
#warning CLOCK_ERROR_PROCENTAGE value in lmic_config.h is too high; values > 7 will cause side effects
#endif

#if (TIME_SYNC_LORAWAN)
#ifndef LMIC_EMABLE_DeviceTimeReq
#define LMIC_EMABLE_DeviceTimeReq 1
#endif
#endif

static QueueHandle_t LoraSendQueue;
TaskHandle_t lmicTask = NULL, lorasendTask = NULL;
char lmic_event_msg[LMIC_EVENTMSG_LEN];

class MyHalConfig_t : public Arduino_LMIC::HalConfiguration_t {
public:
  MyHalConfig_t(){};

  // set SPI pins to board configuration, pins may come from pins_arduino.h
  void begin(void) override {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  }

  // void end(void) override

  // ostime_t setModuleActive(bool state) override
};

static MyHalConfig_t myHalConfig{};

static const lmic_pinmap myPinmap = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST == NOT_A_PIN ? LMIC_UNUSED_PIN : LORA_RST,
    .dio = {LORA_IRQ, LORA_DIO1,
            LORA_DIO2 == NOT_A_PIN ? LMIC_UNUSED_PIN : LORA_DIO2},
    .rxtx_rx_active = LMIC_UNUSED_PIN,
    .rssi_cal = 10,
    .spi_freq = 8000000, // 8MHz
    .pConfig = &myHalConfig
};
void lora_setupNetwork(bool preJoin) {
    if(preJoin) {
    #if CFG_LMIC_US_like
    // in the US, with TTN, it saves join time if we start on subband 1
    // (channels 8-15). This will get overridden after the join by
    // parameters from the network. If working with other networks or in
    // other regions, this will need to be changed.
    LMIC_selectSubBand(1);
#elif CFG_LMIC_EU_like
    // settings for TheThingsNetwork
    // Enable link check validation
    LMIC_setLinkCheckMode(1);
#endif
    } else{
        // set data rate adaptation according to saved setting
     //LMIC_setAdrMode(cfg.adrmode);
    }
}
#endif //HAS_LORA