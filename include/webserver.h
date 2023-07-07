#include "globals.h"
#include <SPIFFS.h>
#include "ESPAsyncWebServer.h"
#include <AsyncJson.h>
#include "wifihandler.h"

String htmlProcessor(const String& var);
void webserver_init(void);
