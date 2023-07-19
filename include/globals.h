#ifndef _GLOBALS_H
#define _GLOBALS_H

// The mother of all embedded development...
#include <Arduino.h>
#include <lmic.h>
#include <WiFi.h>
#include "Version.h"

#ifdef HAS_RTC
#include <RtcUtility.h>
#include <RtcDateTime.h>
#endif
#include <Ticker.h>

// std::set for unified array functions
#include <set>
#include <array>
#include <algorithm>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define KONFIG_MODE 0
#define WLAN_MODE 2


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define FORMAT_SPIFFS_IF_FAILED true
#define DEFAULT_AP_START "BNG" 

#define _bit(b) (1U << (b))
#define _bitl(b) (1UL << (b))

// bits in payloadmask for filtering payload data
#define COUNT_DATA _bit(0)
#define RESERVED_DATA _bit(1)
#define MEMS_DATA _bit(2)
#define GPS_DATA _bit(3)
#define SENSOR1_DATA _bit(4)
#define SENSOR2_DATA _bit(5)
#define SENSOR3_DATA _bit(6)
#define BATT_DATA _bit(7)

#define LMIC_EVENTMSG_LEN 17
// pseudo system halt function, useful to prevent writeloops to NVRAM
#ifndef _ASSERT
#define _ASSERT(cond)                                                          \
  if ((cond) == 0) {                                                           \
    ESP_LOGE(TAG, "FEHLER in %s:%d", __FILE__, __LINE__);                     \
    for (;;)                                                                   \
      ;                                                                        \
  }
#endif

#define _seconds() millis() / 1000.0

extern float	pm25Datas[];
extern float	pm10Datas[];
extern float	tempDatas[];
extern float humdatas[];
enum payloadSendType {LORA_ONLY, LORA_PREFERABLY, WLAN_ONLY};
enum loraStatus {LORA_OFF, LORA_INIT, LORA_INITIALIZED,LORA_JOINING, LORA_JOINED, LORA_JOINWAIT};
typedef struct __attribute__((packed)) {
    char version[10];
    char wifi_ssid[32];
    char wifi_password[255];
    uint32_t chipid;
    u_int8_t *wifi_bssid;
    wifi_mode_t wifi_mode;
    bool wifi_enabled;
    bool lora_is_abp;
    u1_t deveui[8];
    u1_t appeui[8];
    u1_t appkey[16];
    u1_t netid;
    u1_t nwkskey[16];
    u4_t devaddr;
    payloadSendType sendtype;
    uint8_t loradr;
    uint8_t txpower; 
    uint8_t adrmode;
    uint8_t screensaver;
    uint8_t screenon;
    uint8_t countermode;
    int16_t rssilimit;
    uint16_t sendcycle;
    uint16_t sleepcycle;
    uint16_t wakesync;
    uint8_t payloadmask;
    char model[50];
    uint8_t revision;
    uint8_t cores;
    uint32_t flashsize;
    uint32_t cpuspeed;
} systemConfig_t;

typedef struct {
  uint8_t MessageSize;
  uint8_t MessagePort;
  uint8_t Message[PAYLOAD_BUFFER_SIZE];
} MessageBuffer_t;


typedef struct {
    int32_t latitude;
    int32_t longitude;
    uint8_t satellites;
    uint16_t hdop;
    int16_t altitude;
} gpsStatus_t;

typedef struct {
    float iaq;             // IAQ signal
    uint8_t iaq_accuracy;  // accuracy of IAQ signal
    float temperature;     // temperature signal
    float humidity;        // humidity signal
    float pressure;        // pressure signal
    float raw_temperature; // raw temperature signal
    float raw_humidity;    // raw humidity signal
    float gas;             // raw gas sensor signal
} bmeStatus_t;

typedef struct {
  float pm10;
  float pm25;
} sdsStatus_t;

typedef struct {  
  char wifi_ssid[32];
  wifi_mode_t wifi_mode;
  wl_status_t actual_wifi_status;
  char hostname[32];
  bool wifi_ready;
  loraStatus lora_status;
  bool gps_enabled;
  bool gps_time_valid;
  bool gps_latlng_valid;
  bool bme_valid;
  bool sds_valid;
  float temp;
  float hum;
  float press;
  float pm10;
  float pm25;
  double lat;
  double lon;
  double altitude;
  uint32_t sats;
  String myip;
  uint32_t heap;
  uint32_t freeheap;
  uint8_t lora_waitings;
  char radioParams[40];
  time_t last_payload;
  time_t actual_time;
} systemvars_t;

#endif