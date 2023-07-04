#ifndef _GPSREAD_H
#define _GPSREAD_H

#if (HAS_GPS)

#include <TinyGPS++.h> // library for parsing NMEA data
#include "timekeeper.h"
#include <SoftwareSerial.h>

#ifndef GPS_BAUDRATE
#define GPS_BAUDRATE 9600UL
#endif

extern TinyGPSPlus gps; // Make TinyGPS++ instance globally availabe
extern TaskHandle_t GpsTask;

int gps_init(void);
int gps_config();
bool gps_hasfix();
void gps_storelocation(gpsStatus_t *gps_store);
void gps_loop(void *pvParameters);
time_t get_gpstime(uint16_t *msec);

#endif

#endif