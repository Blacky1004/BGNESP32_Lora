#ifndef _CONFIG_BNG_H
#define _CONFIG_BNG_H

#include "globals.h"
#include "reset.h"
#include <Preferences.h>
#include <vector>
extern systemConfig_t cfg;
extern systemvars_t systemCfg;
extern std::vector<resturls_t> urlList;
void saveConfig(bool erase);
void loadConfig(void);
void eraseConfig(void);
void loadRestUrls(void);
void saveRestUrls(void);
uint16_t insertRestUrl(String url, String apikey, bool can_delete);
int version_compare(const String v1, const String v2);
void sort_chart_data(String a_name, float newValue);

#endif
