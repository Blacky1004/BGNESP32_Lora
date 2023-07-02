#include "webserver.h"
AsyncWebServer server(80);
String htmlProcessor(const String& var){
    if(var == "CHIPID") {
        ESP_LOGD(TAG, "VAR CHIPID = '%s'", systemCfg.hostname);
        return String(systemCfg.hostname);
    }
    return String();
}

void webserver_init() {
    ESP_LOGI(TAG,"Starte Webserver auf http://%s ...", WiFi.localIP().toString().c_str());

    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
		ESP_LOGE(TAG,"SPIFFS Mountingfehler, Webserver wird nicht gestartet!");
        return;
	}

    server.serveStatic("/", SPIFFS,"/");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String indexFile = "/index_ap.html";
        if(cfg.wifi_mode == WIFI_STA && WiFi.status() == WL_CONNECTED){
            indexFile = "/index_default.html";
        }

        request->send(SPIFFS, indexFile, String(), false, htmlProcessor);
    });

    #pragma region Contenthandling
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/styles.css", "text/css");
	});
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/favicon.ico", "image/png");
	});
    server.on("/chart.js", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/chart.js", "text/javascript");
	});
    server.on("/toastr.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/toastr.min.css", "text/css");
	});
    server.on("/toastr.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/toastr.min.js", "text/javascript");
	});
    server.on("/jquery.js", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/jquery.js", "text/javascript");
	});
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript");
	});
    server.on("/system.js", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/system.js", "text/javascript", false, htmlProcessor);
	});
	server.on("/logo-buergernetzgeragreiz.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/logo-buergernetzgeragreiz.png", "image/png");
	});
	server.on("/nav_bg.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/configuration.png", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/configuration.png", "image/png");
    });
    #pragma endregion

    server.begin();
    ESP_LOGI(TAG, "Webserver erfolgreich gestartet.");
}