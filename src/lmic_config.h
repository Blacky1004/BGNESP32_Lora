// workaround for arduino-espressif32 v2.0.0 (see isse #714 @ MCCI_LMIC)
#define hal_init LMICHAL_init
#define CFG_eu868 1 // Europe (high band)
#if !(defined(CFG_sx1272_radio) || defined(CFG_sx1276_radio))
#define CFG_sx1276_radio 1
#endif

#define LMIC_ENABLE_DeviceTimeReq 1
//#define LMIC_DEBUG_LEVEL 1
//#define LMIC_PRINTF_TO Serial
#define LMIC_FAILURE_TO Serial
#define DISABLE_PING

#define USE_ORIGINAL_AES
//#define USE_IDEETRON_AES