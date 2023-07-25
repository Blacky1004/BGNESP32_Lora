#if (HAS_GPS)

#include "globals.h"
#include "gpsread.h"


TinyGPSPlus gps;
TaskHandle_t GpsTask;
//HardwareSerial GPS_Serial(1); // use UART #1
SoftwareSerial ss(GPS_SERIAL_RX, GPS_SERIAL_TX);
// Ublox UBX packet data

// UBX CFG-PRT packet
byte CFG_PRT[] = {
    0xB5,                       // sync char 1
    0x62,                       // sync char 2
    0x06,                       // class
    0x00,                       // id
    0x14,                       // length
    0x00,                       // .
    0x01,                       // portID (UART 1)
    0x00,                       // reserved
    0x00,                       // txReady
    0x00,                       // .
    0b11010000,                 // UART mode: 8N1
    0b00001000,                 // .
    0x00,                       // .
    0x00,                       // .
    (byte)GPS_BAUDRATE,         // baudrate
    (byte)(GPS_BAUDRATE >> 8),  // .
    (byte)(GPS_BAUDRATE >> 16), // .
    (byte)(GPS_BAUDRATE >> 24), // .
    0b00000011,                 // input protocols: NMEA + UBX
    0b00000000,                 // .
    0b00000010,                 // output protocols: NMEA
    0x00000000,                 // .
    0x00,                       // reserved
    0x00,                       // .
    0x00,                       // .
    0x00                        // .
};

// Array of two bytes for CFG-MSG packets payload.
byte CFG_MSG_CID[][2] = {{0xF0, 0x01}, {0xF0, 0x02}, {0xF0, 0x03}, {0xF0, 0x05},
                         {0xF0, 0x06}, {0xF0, 0x07}, {0xF0, 0x08}, {0xF0, 0x09},
                         {0xF0, 0x0A}, {0xF0, 0x0E}, {0xF1, 0x00}, {0xF1, 0x03},
                         {0xF1, 0x04}, {0xF1, 0x05}, {0xF1, 0x06}};

// UBX CFG-MSG packet
byte CFG_MSG[] = {
    0xB5, // sync char 1
    0x62, // sync char 2
    0x06, // class
    0x01, // id
    0x03, // length
    0x00, // .
    0x00, // payload (first byte from messages array element)
    0x00, // payload (second byte from messages array element)
    0x00  // payload (zero to disable message)
};

// UBX CFG-CFG packet
byte CFG_CFG[] = {
    0xB5,       // sync char 1
    0x62,       // sync char 2
    0x06,       // class
    0x09,       // id
    0x0D,       // length
    0x00,       // .
    0b00011111, // clearmask
    0b00000110, // .
    0x00,       // .
    0x00,       // .
    0x00,       // savemask
    0x00,       // .
    0x00,       // .
    0x00,       // .
    0b00011111, // loadmask
    0b00000110, // .
    0x00,       // .
    0x00,       // .
    0b00010001  // devicemask
};

// initialize and configure GPS
int gps_init(void) {
  ESP_LOGI(TAG, "Opening serial GPS");
  systemCfg.gps_enabled = true;
  ss.begin(GPS_BAUDRATE);
  
  return 1;
} // gps_init()

// store current GPS location data in struct
void gps_storelocation(gpsStatus_t *gps_store) {
  if (gps.location.isUpdated() && gps.location.isValid() &&
      (gps.location.age() < 1500)) {
    gps_store->latitude = (int32_t)(gps.location.lat() * 1e6);
    gps_store->longitude = (int32_t)(gps.location.lng() * 1e6);
    gps_store->satellites = (uint8_t)gps.satellites.value();
    gps_store->hdop = (uint16_t)gps.hdop.value();
    gps_store->altitude = (int16_t)gps.altitude.meters();
    if(cfg.latitude <= 0 || cfg.longitude <= 0) 
    {
      cfg.latitude = gps.location.lat();
      cfg.longitude = gps.location.lng();
      saveConfig(false);
    }
  } else {
    ESP_LOGE(TAG, "Keine gültigen GPS Daten erhalten!");
    systemCfg.gps_latlng_valid = false;;
  }

}

bool gps_hasfix() {
  // adapted from source:
  // https://github.com/hottimuc/Lora-TTNMapper-T-Beam/blob/master/fromV08/gps.cpp
  return (gps.location.isValid() && gps.location.age() < 4000 &&
          gps.hdop.isValid() && gps.hdop.value() <= 600 &&
          gps.hdop.age() < 4000 && gps.altitude.isValid() &&
          gps.altitude.age() < 4000);
}

// function to poll UTC time from GPS NMEA data; note: this is costly
time_t get_gpstime(uint16_t *msec = 0) {
  const uint16_t txDelay =
      70U * 1000 / (GPS_BAUDRATE / 9); // serial tx of 70 NMEA chars

  // did we get a current date & time?
  if (gps.time.age() < 1000) {
    // convert tinygps time format to struct tm format
    struct tm gps_tm = {0};
    gps_tm.tm_sec = gps.time.second();
    gps_tm.tm_min = gps.time.minute();
    gps_tm.tm_hour = gps.time.hour();
    gps_tm.tm_mday = gps.date.day();
    gps_tm.tm_mon = gps.date.month() - 1;    // 1-12 -> 0-11
    gps_tm.tm_year = gps.date.year() - 1900; // 2000+ -> years since 1900

    // convert UTC tm to time_t epoch
    gps_tm.tm_isdst = 0; // UTC has no DST
    time_t t = mkgmtime(&gps_tm);
    ESP_LOGD(TAG, "Aktuelle Zeit: %02d.%02d.%02d %02d:%02d:%02d",gps.date.day(), gps.date.month(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
#ifdef GPS_INT
    // if we have a recent GPS PPS pulse, sync on top of next second
    uint16_t ppsDiff = millis() - lastPPS;
    if (ppsDiff < 1000)
      *msec = ppsDiff;
    else {
      ESP_LOGD(TAG, "no PPS from GPS");
      return 0;
    }
#else
    // best guess for sync on top of next second
    *msec = gps.time.centisecond() * 10U + txDelay;
#endif
    systemCfg.gps_time_valid = true;
    return t;
  }

  ESP_LOGD(TAG, "no valid GPS time");
  systemCfg.gps_time_valid = false;
  return 0;
} // get_gpstime()

// GPS serial feed FreeRTos Task
void gps_loop(void *pvParameters) {
  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  // feed GPS decoder with serial NMEA data from GPS device
  while (1) {
    while (cfg.payloadmask & GPS_DATA) {
      while (ss.available())
        gps.encode(ss.read());
      if (millis() > 5000 && gps.charsProcessed() < 10){
        ESP_LOGE(TAG, "Keine Verbindung zum GPS Modul, bitte Verbindungen prüfen!");
      }
      delay(5);

    }
    delay(1000);
  } // infinite while loop
} // gps_loop()

#endif // HAS_GPS
