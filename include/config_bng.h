#ifndef _CONFIG_BNG_H
#define _CONFIG_BNG_H

#include "globals.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Preferences.h>

DynamicJsonDocument config(1024);

String readConfigFile(fs::FS &fs);
void writeConfigFile(fs::FS &fs, String content);
bool loadConfig(void);
bool saveConfig(bool erase);

#endif
