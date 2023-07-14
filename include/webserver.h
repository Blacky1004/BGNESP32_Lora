#include "globals.h"
#include <SPIFFS.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "wifihandler.h"
#include <map>

#define CACHE_HEADER "max-age=86400"
#define CORS_HEADER "Access-Control-Allow-Origin"

#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_PLAIN "text/plain"
#define CONTENT_TYPE_HTML "text/html"
#define CAPTIVE_PORTAL
#define PANIC(...) abort()

String htmlProcessor(const String& var);
void webserver_init(void);
