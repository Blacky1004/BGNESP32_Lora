#ifndef _WIFIHANDLER_H
#define _WIFIHANDLER_H

#include "globals.h"
#include "config_bng.h"
#include "irqhandler.h"
#include <list>


extern Ticker wificycler;

extern TaskHandle_t WiFiTask;
typedef struct {
    public:
    int id;
    String ssid;
    uint8_t* bssid;
    int8_t rssi;
    wifi_auth_mode_t encrytionType;
} wifi_network_t;

extern std::list<wifi_network_t> myWiFiList;

int wifi_init();
void setWiFiIRQ(void);
void wifi_loop();
#endif