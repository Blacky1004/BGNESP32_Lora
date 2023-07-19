#ifndef _MAIN_H
#define _MAIN_H

#include <esp_spi_flash.h> // zum auslesen der ESP32 Attribute
#include <esp_event.h> // wird für den WIFI Eventhandler benötigt
#include <esp32-hal-timer.h> //für die versch. Timer
#include <esp_coexist.h>     // needed for coex version display
#include <esp_wifi.h> //für den zugriff auf init und deinit von WIFI

#include "globals.h" //unsere globalen Deklarationen
#include "cyclic.h"
#include "reset.h"
#include "config_bng.h" //all unsere Konfigurationssachen
#include "irqhandler.h"
#include "i2c.h"
#include "lorawan.h" //LoRaWAN 
#include "wifihandler.h"
#include "timekeeper.h"
#include "boot.h"
#include "webserver.h"
#include "button.h"

#endif
