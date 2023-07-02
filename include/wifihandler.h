#ifndef _WIFIHANDLER_H
#define _WIFIHANDLER_H

#include "globals.h"
#include "config_bng.h"
#include "irqhandler.h"
#include <map>


extern Ticker wificycler;
extern std::map<int, std::string>  wifiNetworkList;
extern TaskHandle_t WiFiTask;


int wifi_init();
void setWiFiIRQ(void);
void wifi_loop();
#endif