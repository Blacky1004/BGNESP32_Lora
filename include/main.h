#ifndef _MAIN_H
#define _MAIN_H

#include <esp_spi_flash.h> // zum auslesen der ESP32 Attribute
#include <esp_event.h> // wird für den WIFI Eventhandler benötigt
#include <esp32-hal-timer.h> //für die versch. Timer
#include <esp_wifi.h> //für den zugriff auf init und deinit von WIFI

#include "globals.h" //unsere globalen Deklarationen
#include "reset.h"
#include "config_bng.h" //all unsere Konfigurationssachen
#include "lorawan.h" //LoRaWAN 

#endif
