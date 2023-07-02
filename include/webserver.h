#include "globals.h"
#include <SPIFFS.h>
#include "ESPAsyncWebServer.h"
#include "wifihandler.h"

String htmlProcessor(const String& var);
void webserver_init(void);