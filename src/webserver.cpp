#include "webserver.h"
#include <ArduinoJson.h>

AsyncWebServer server(80);
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
    while (systemCfg.wifi_ready == false)
    {
        delay(20);
    }
    
    systemCfg.myip = systemCfg.wifi_mode == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    ESP_LOGI(TAG,"Starte Webserver auf http://%s ...", systemCfg.myip);

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
        AsyncJsonResponse * response = new AsyncJsonResponse();
        //DynamicJsonDocument doc(1024);
        JsonObject root = response->getRoot();
        // JsonArray wl = root.createNestedArray("wifis");
        // std::map<byte, std::string> wlist;
        // int n = WiFi.scanNetworks();
        // if(n > 0){
        //     for(byte i = 0; i < n; i++) {
        //         JsonObject o = wl.createNestedObject();
        //         o["id"] = i;
        //         o["ssid"] = WiFi.SSID(i);
        //         wl.add(o);
        //     }
        // }
        root["wifis"] = wifiWebList;
        root["mode"] = systemCfg.wifi_mode;
        root["status"] = systemCfg.actual_wifi_status;
        root["code"] = 200;
        
        //String json = "";
        //serializeJson(doc, json);
        //request->send(200, "application/json", json);
        //json= String();
        response->setLength();
        request->send(response);
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
        doc["lcycle"] = cfg.sendcycle;
        doc["devid"] = String(LMIC.devaddr, HEX);
        doc["lwaitings"] = systemCfg.lora_waitings;
        doc["lpayload"] = systemCfg.last_payload;
        doc["rparams"] = systemCfg.radioParams; 

        String json = "";
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        json= String();
    });
    server.on("/get_sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
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
        request->send(200, "application/json", json);
        json= String();
    });
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
        else if(data["ssid"].as<uint8_t>() == -1) {
            cfg.wifi_mode = WIFI_AP;
            cfg.wifi_bssid = 0;
            if(cfg.wifi_password != data["ssidpasw"].as<String>().c_str()) {
                strcpy(cfg.wifi_password, data["ssidpasw"].as<String>().c_str());
                strcpy(cfg.wifi_ssid , systemCfg.hostname);
                cfg.wifi_enabled = false;
                
                response = "{\"code\": 200, \"message\":\"Es wurde keine SSID übergeben.\", \"ip\": \"192.168.4.1\"}";
                request->send(200, "application/json", response);
                saveConfig(false);
                delay(5000);
                do_reset(false);
            } else {
                response = "{\"code\": 303, \"message\": \"Es wurden keine Änderungen erkannt.\"}";
                request->send(200, "application/json", response);
            }
        } else {
            cfg.wifi_mode = WIFI_STA;
            bool found = false;
            
            if(found){
                saveConfig(false);
                response = "{\"code\": 200, \"message\":\"Es wurde keine SSID übergeben.\"}";
                request->send(200, "application/json", response);
                do_reset(false);
            } else {
                response = "{\"code\": 404, \"message\":\"Es wurde Kein Netzwerk mit dieser SSID gefunden.\"}";
                request->send(200, "application/json", response);
            }
        }
    });
    server.addHandler(check_wifi_handler);
    #pragma endregion

    server.begin();
    ESP_LOGI(TAG, "Webserver erfolgreich gestartet.");
}