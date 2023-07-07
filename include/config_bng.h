#ifndef _CONFIG_BNG_H
#define _CONFIG_BNG_H

#include "globals.h"
#include "reset.h"
#include <Preferences.h>

extern systemConfig_t cfg;
extern systemvars_t systemCfg;

void saveConfig(bool erase);
void loadConfig(void);
void eraseConfig(void);
int version_compare(const String v1, const String v2);
void sort_chart_data(String a_name, float newValue);

#endif
