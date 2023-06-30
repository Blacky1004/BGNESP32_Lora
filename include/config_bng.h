#ifndef _CONFIG_BNG_H
#define _CONFIG_BNG_H

#include "globals.h"
#include "reset.h"
#include <Preferences.h>

extern systemConfig_t cfg;

void saveConfig(bool erase);
void loadConfig(void);
void eraseConfig(void);
int version_compare(const String v1, const String v2);

#endif
