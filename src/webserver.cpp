#include "webserver.h"
#include <ArduinoJson.h>

AsyncWebServer server(80);

#ifdef CAPTIVE_PORTAL
class CaptiveRequesthandler : public AsyncWebHandler {
    public:
    CaptiveRequesthandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
        if(request->host() != WiFi.softAPIP().toString())
            return true;
        else
            return false;
    }

    void handleRequest(AsyncWebServerRequest * request) {
        ESP_LOGD(TAG, "captive-request to %s", request->url().c_str());
        String location = "http://" + WiFi.softAPIP().toString();
        if(request->host() == String(systemCfg.hostname) + ".local")
            location += request->url();
        request->redirect(location);
    }
};
#endif

void handleNotFound(AsyncWebServerRequest *request){
    ESP_LOGE(TAG, "NOT FOUND %s", request->url().c_str());
    request->send(404, F(CONTENT_TYPE_PLAIN), F("Nicht gefunden!"));
}

void handleGetWifiList(AsyncWebServerRequest * request) {
    String json = "{\"wifis\": [";

    int n = WiFi.scanComplete();
    ESP_LOGD(TAG, "scanning wifi (%d)", n);

    if (n == WIFI_SCAN_FAILED){
    WiFi.scanNetworks(true);
    }
    else if (n > 0) { // scan finished
    for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        json += "{";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
        // json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
        // json += ",\"channel\":"+String(WiFi.channel(i));
        json += ",\"secure\":" + String(WiFi.encryptionType(i));
        json += "}";
    }
    // save scan result memory
    WiFi.scanDelete();
    // WiFi.scanNetworks(true);
    }
    json += "]}";

    AsyncWebServerResponse *response = request->beginResponse(200, F(CONTENT_TYPE_JSON), json);
    response->addHeader(F(CORS_HEADER), "*");
    request->send(response);    
}
void handleLoraInfo(AsyncWebServerRequest * request) {
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
    doc["lcycle"] = cfg.sendcycle;
    doc["devid"] = String(LMIC.devaddr, HEX);
    doc["lwaitings"] = systemCfg.lora_waitings;
    doc["lpayload"] = systemCfg.last_payload;
    doc["rparams"] = systemCfg.radioParams; 

    String json = "";
    serializeJson(doc, json);
    AsyncWebServerResponse *response = request->beginResponse(200, F(CONTENT_TYPE_JSON), json);
    response->addHeader(F(CORS_HEADER), "*");
    request->send(response); 
}
void handleBackupConfig(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    doc["adrmode"] = cfg.adrmode;
    doc["appeui"] = cfg.appeui;
    doc["appkey"] = cfg.appkey;
    doc["devaddr"] = cfg.devaddr;
    doc["deveui"] = cfg.deveui;
    doc["lora_is_abp"] = cfg.lora_is_abp;
    doc["loradr"] = cfg.loradr;
    doc["nwkskey"] = cfg.nwkskey;
    doc["payloadmask"] = cfg.payloadmask;
    doc["rssilimit"] = cfg.rssilimit;
    doc["screenon"] = cfg.screenon;
    doc["screensaver"] = cfg.screensaver;
    doc["sendcycle"] = cfg.sendcycle;
    doc["sendtype"] = cfg.sendtype;
    doc["sleepcycle"] = cfg.sleepcycle;
    doc["txpower"] = cfg.txpower;
    doc["version"] = cfg.version;
    doc["wakesync"] = cfg.wakesync;
    doc["wifi_enabled"] = cfg.wifi_enabled;
    doc["wifi_mode"] = cfg.wifi_mode;
    doc["wifi_password"] = cfg.wifi_password;
    doc["wifi_ssid"] = cfg.wifi_ssid;
    String json = "";
    serializeJson(doc, json);
    File backup = SPIFFS.open("/backup.json", FILE_WRITE);
    if(!backup)
        request->send(500, "text/plain", "Fehler beim erstellen des Backups!");
    else{
        backup.print(json);
        backup.close();
        request->send(SPIFFS, "/backup.json", "application/json", true);
    }
}
void handleSensorList(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    
    JsonObject ocfg = doc.createNestedObject("cfg");
    ocfg["lcycle"] = cfg.sendcycle;
    ocfg["devid"] = String(LMIC.devaddr, HEX);
    ocfg["rev"] = cfg.revision;
    ocfg["flash"] = cfg.flashsize / 1024 / 1024;
    ocfg["version"] =  String(cfg.version);
    ocfg["speed"] = ESP.getCpuFreqMHz();
    ocfg["heap"] = systemCfg.heap;
    ocfg["freeheap"] = systemCfg.freeheap;
    ocfg["fdate"] = compileTime();
    JsonObject wlan = doc.createNestedObject("wlan");
    wlan["mode"] = systemCfg.wifi_mode == WIFI_STA ? "WLAN Station" : "AccessPoint";
    wlan["ssid"] = systemCfg.wifi_ssid;
    wlan["ip"] = systemCfg.myip;
    JsonObject gps = doc.createNestedObject("gps");
    gps["enabled"] = systemCfg.gps_enabled;
    gps["valid"] = systemCfg.gps_latlng_valid;
    if(systemCfg.gps_latlng_valid) {
        gps["lat"] = systemCfg.lat;
        gps["lng"] = systemCfg.lon;
        gps["sat"] = systemCfg.sats;
        gps["alt"] = systemCfg.altitude;
    }
    JsonObject sds = doc.createNestedObject("sds");
    sds["valid"] = systemCfg.sds_valid;
    if(systemCfg.sds_valid) {
        sds["pm10"] = systemCfg.pm10;
        sds["pm25"] = systemCfg.pm25;
    }
    JsonObject bme = doc.createNestedObject("bme");
    bme["valid"] = systemCfg.bme_valid;
    bme["temp"] = systemCfg.temp;
    bme["hum"] = systemCfg.hum;
    bme["press"] = systemCfg.press;
    String json = "";
    serializeJson(doc, json);

    AsyncWebServerResponse *response = request->beginResponse(200, F(CONTENT_TYPE_JSON), json);
    response->addHeader(F(CORS_HEADER), "*");
    request->send(response);    
}
String htmlProcessor(const String& var){
    if(var == "CHIPID") {
        ESP_LOGD(TAG, "VAR CHIPID = '%s'", systemCfg.hostname);
        return String(systemCfg.hostname);
    }
    if(var == "CHIPMODEL") {
        return String(cfg.model);
    }
    if(var == "CHIPREV") {
        return String(cfg.revision);
    }
    if(var == "CORES") {
        return String(cfg.cores);
    }
    if(var == "MHZ") {
        return String(cfg.cpuspeed);
    }
    if(var == "RAWCHIPID") {
        return String(cfg.chipid);
    }
    if(var =="DEVID") {
        return String(LMIC.devaddr, HEX);
    }
    if(var == "INTERVAL") {
        return String(cfg.sendcycle);
    }
    return String();
}

void webserver_init() {
    // while (systemCfg.wifi_ready == false)
    // {
    //     delay(20);
    // }
    
    systemCfg.myip = systemCfg.wifi_mode == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    ESP_LOGI(TAG,"Starte Webserver auf http://%s ...", systemCfg.myip);

    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
		ESP_LOGE(TAG,"SPIFFS Mountingfehler, Webserver wird nicht gestartet!");
        return;
	}
    server.onNotFound(handleNotFound);
    #ifdef CAPTIVE_PORTAL
        server.addHandler(new CaptiveRequesthandler()).setFilter(ON_AP_FILTER);
    #endif
    server.serveStatic("/", SPIFFS,"/");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String indexFile = "/index_ap.html";
        if(cfg.wifi_mode == WIFI_STA && WiFi.status() == WL_CONNECTED){
            indexFile = "/index_default.html";
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, indexFile, String(), false, htmlProcessor);
        request->send(response);
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
    server.on("/wifi_err.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/wifi_0.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/wifi_1.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/wifi_2.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/wifi_3.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/wifi_4.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/wifi_ap.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    server.on("/configuration.png", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/configuration.png", "image/png");
    });
    #pragma endregion

    #pragma region Ajaxabfragen
    server.on("/get_wifi_list", HTTP_GET, handleGetWifiList);
    server.on("/get_lora_info", HTTP_GET, handleLoraInfo);
    server.on("/get_sensors", HTTP_GET, handleSensorList);
    server.on("/get_chartdatas", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument doc(1024);
        JsonArray pm10 = doc.createNestedArray("pm10");
        for(int i= 0; i < 10; i++) {
            pm10.add(pm10Datas[i]);
        }
        JsonArray pm25 = doc.createNestedArray("pm25");
        for(int i= 0; i < 10; i++) {
            pm25.add(pm25Datas[i]);
        }
        JsonArray tmp = doc.createNestedArray("tmp");
        for(int i= 0; i < 10; i++) {
            tmp.add(tempDatas[i]);
        }
        JsonArray hum = doc.createNestedArray("hum");
        for(int i= 0; i < 10; i++) {
            hum.add(humdatas[i]);
        }
        String json = "";
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        json= String();
    });
    server.on("/backup_config", HTTP_GET, handleBackupConfig);

    AsyncCallbackJsonWebHandler *check_wifi_handler = new AsyncCallbackJsonWebHandler("/save_wifi", [](AsyncWebServerRequest *request, JsonVariant &json){
        StaticJsonDocument<200> data;
        if (json.is<JsonArray>())
        {
            data = json.as<JsonArray>();
        }
        else if (json.is<JsonObject>())
        {
            data = json.as<JsonObject>();
        }
        String response;
        if((!data.containsKey("ssid") || data["ssid"] == ""  || data["ssid"] == "null") && data["enabled"] == true) {
            response = "{\"code\": 400, \"message\":\"Es wurde keine SSID übergeben.\"}";
            request->send(400, "application/json", response);
        } 
        else if(data["ssid"].as<String>() == "-1") {
            cfg.wifi_mode = WIFI_AP;
            cfg.wifi_bssid = 0;
            if(cfg.wifi_password != data["pasw"].as<String>().c_str()) {
                strcpy(cfg.wifi_password, data["pasw"].as<String>().c_str());
                strcpy(cfg.wifi_ssid , systemCfg.hostname);
                cfg.wifi_enabled = false;
                
                response = "{\"code\": 200, \"message\":\"Es wurde keine SSID übergeben.\", \"ip\": \"192.168.4.1\"}";
                request->send(200, "application/json", response);
                saveConfig(false);
                //delay(5000);
                do_reset(false);
            } else {
                response = "{\"code\": 303, \"message\": \"Es wurden keine Änderungen erkannt.\"}";
                request->send(200, "application/json", response);
            }
        } else {
            cfg.wifi_mode = WIFI_STA;
            bool found = false;
            strcpy(cfg.wifi_ssid, data["ssid"].as<String>().c_str());
            strcpy(cfg.wifi_password, data["pasw"].as<String>().c_str());
            saveConfig(false);
            response = "{\"code\": 200, \"message\":\"Es wurde Die WLAN Einstellung gespeichert..\"}";
            request->send(200, "application/json", response);
            do_reset(false);
        }
    });
    server.addHandler(check_wifi_handler);
    #pragma endregion

    server.begin();
    ESP_LOGI(TAG, "Webserver erfolgreich gestartet.");
}