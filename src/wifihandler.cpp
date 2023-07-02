#include "wifihandler.h"
#include <WiFi.h>
#include "esp_random.h"
#include <map>

TaskHandle_t WiFiTask;
std::map<int, std::string>  wifiNetworkList;
Ticker wificycler;
uint32_t chipId = 0;
String mySSID = "";
void setWiFiIRQ() { xTaskNotify(irqHandlerTask, WIFI_IRQ, eSetBits);}

void load_WiFiNetwork() {
    int n = WiFi.scanNetworks();
    if(n == 0) {
        ESP_LOGW(TAG, "Keine Netzwerke gefunden!");
        cfg.wifi_mode = WIFI_AP;
    } else {
        wifiNetworkList.clear();

        for (int i = 0; i < n; i++)
        {
            wifiNetworkList.insert({i, WiFi.SSID(i).c_str()});
        }        
    }
}

bool check_wifi() {
    int c = 0;
    while (c < 20)
    {
        if(WiFi.status() == WL_CONNECTED){
            return true;
        }

        delay(500);            
        c++;
    }
    return false;
}
int startAP() {
    WiFi.mode(WIFI_AP);
    
    if(WiFi.softAP(mySSID,"")) {
        ESP_LOGI(TAG,"AccessPoint mit der SSID %s gestartet", mySSID);
        return 1;
    } else {
        ESP_LOGE(TAG, "Fehler beim starten des AccessPoints!");
        return 0;
    }
}
int wifi_init() {
    int status = 0;
    for(int i=0; i< 17; i=i+8) {
		chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
	mySSID = std::move("BNG-" + String(chipId));
    load_WiFiNetwork();
    if(!cfg.wifi_enabled) {
        ESP_LOGW(TAG, "WiFi ist nicht aktiviert.");
        return 0;
    }
    if(cfg.wifi_mode == WIFI_STA) {
        //Stations modus
        if(cfg.wifi_ssid == NULL || sizeof(cfg.wifi_ssid) < 1) {
           status = startAP();
        } else {
            WiFi.mode(WIFI_STA);
            WiFi.hostname(mySSID);
            strcpy(systemCfg.hostname, WiFi.getHostname());
            ESP_LOGI(TAG,"Verbinde zu WLAN %s und Passwort '%s'", cfg.wifi_ssid, cfg.wifi_password);
            status = WiFi.begin(cfg.wifi_ssid, cfg.wifi_password);
            if(check_wifi()){
                ESP_LOGI(TAG,"Verbunden mit WLAN '%s'", WiFi.SSID());
                ESP_LOGI(TAG, "Mein Hostname: '%s'", WiFi.getHostname());
                ESP_LOGI(TAG, "lokale IP: %s", WiFi.localIP().toString());
            } else {
                ESP_LOGW(TAG, "Konnte keine Verbindung zu SSID '%s' aufbauen, starte AccessPoint.", WiFi.SSID());
                status = startAP();
            }
        }
    } else {
       status = startAP();
    }
    if(status) {
        wificycler.attach(WIFICYCLE, setWiFiIRQ);
    }
    return 1;
}

void wifi_loop() {
    systemCfg.actual_wifi_status = WiFi.status();
    load_WiFiNetwork();
}

