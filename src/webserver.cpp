#include "webserver.h"
#include <ArduinoJson.h>

AsyncWebServer server(80);
String htmlProcessor(const String& var){
    if(var == "CHIPID") {
        ESP_LOGD(TAG, "VAR CHIPID = '%s'", systemCfg.hostname);
        return String(systemCfg.hostname);
    }
    return String();
}

void webserver_init() {
    while (systemCfg.wifi_ready == false)
    {
        delay(20);
    }
    
    ESP_LOGI(TAG,"Starte Webserver auf http://%s ...", cfg.wifi_mode == WIFI_AP ? WiFi.softAPIP().toString().c_str() : WiFi.localIP().toString().c_str());

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

    #pragma region Ajaxabfragen
    server.on("/get_wifi_list", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        JsonArray wl = doc.createNestedArray("wifis");
        for(wifi_network_t w: myWiFiList){
            JsonObject o = wl.createNestedObject();
            o["id"] = w.id;
            o["ssid"] = w.ssid;
            o["bssid"] = w.bssid;
            o["rssi"] = w.rssi;
            o["enc_type"] = w.encrytionType;
            wl.add(o);
        }
        doc["mode"] = systemCfg.wifi_mode;
        doc["status"] = systemCfg.actual_wifi_status;
        doc["code"] = 200;
        String json = "";
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        json= String();
    });
    server.on("/get_lora_info", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        doc["status"] = systemCfg.lora_status;
        
        #if (USE_OTAA)
            JsonArray devkey = doc.createNestedArray("deveui");
            JsonArray appkey = doc.createNestedArray("appeui");
            JsonArray apskey = doc.createNestedArray("appkey");
            doc["mode"] = "OTAA";
            for(u1_t d: DEVEUI) {
                devkey.add(d);
            }
            for(u1_t a: APPEUI) {
                appkey.add(a);
            }
            for(u1_t k: APPKEY) {
                apskey.add(k);
            }
            if(systemCfg.lora_status == LORA_JOINED) {
                doc["netid"] = LMIC.netid & 0x001FFFFF;
                doc["devaddr"] = String(LMIC.devaddr, HEX);
            }
        #else
            doc["mode"] = "ABP";
        #endif

    });
    #pragma endregion
    server.begin();
    ESP_LOGI(TAG, "Webserver erfolgreich gestartet.");
}