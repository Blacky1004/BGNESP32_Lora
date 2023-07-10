#ifndef _WIFIHANDLER_H
#define _WIFIHANDLER_H

#include "globals.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "config_bng.h"
#include "irqhandler.h"
#include <list>


extern Ticker wificycler;
extern char wifi_event_msg[255];
extern TaskHandle_t WiFiTask;
extern String wifiWebList;
typedef struct {
    public:
    int id;
    String ssid;
    int8_t rssi;
    wifi_auth_mode_t encrytionType;
} wifi_network_t;

extern std::list<wifi_network_t> myWiFiList;

int wifi_init();
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void setWiFiIRQ(void);
void wifi_loop();
#endif