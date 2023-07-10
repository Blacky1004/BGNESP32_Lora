#include "wifihandler.h"
#include "esp_random.h"
#include <map>
#include<list>

TaskHandle_t WiFiTask;
char wifi_event_msg[255];
std::list<wifi_network_t> myWiFiList;
Ticker wificycler;
uint32_t chipId = 0;
String mySSID = "";
void setWiFiIRQ() { xTaskNotify(irqHandlerTask, WIFI_IRQ, eSetBits);}
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    snprintf(wifi_event_msg, 255, "%02X", info.wifi_ap_staipassigned.ip.addr);
    ESP_LOGI(TAG, "Verbindungsanfrage von %s erhalten", wifi_event_msg);
}
void load_WiFiNetwork() {
    int n = WiFi.scanNetworks();
    if(n == 0) {
        ESP_LOGW(TAG, "Keine Netzwerke gefunden!");
        cfg.wifi_mode = WIFI_AP;
        //myWiFiList.clear();
    } else {
        myWiFiList.clear();
        for (int i = 0; i < n; i++)
        {
            wifi_network_t w;
            w.id = i;
            w.encrytionType = WiFi.encryptionType(i);
            w.rssi = WiFi.RSSI(i);
            w.ssid = WiFi.SSID(i);
            w.bssid = WiFi.BSSID(i);
            myWiFiList.push_back(w);  
        }        
    }
}

bool check_wifi() {
    int c = 0;
    //WiFi.begin(cfg.wifi_ssid, cfg.wifi_password);
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
    cfg.wifi_mode = WIFI_AP;
    strcpy(systemCfg.hostname, mySSID.c_str());
    if(WiFi.softAP(mySSID,"")) {
        ESP_LOGI(TAG,"AccessPoint mit der SSID %s gestartet", mySSID);
        //WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_AP_STAIPASSIGNED);
        systemCfg.wifi_ready = true;
        systemCfg.myip = WiFi.softAPIP().toString();
        strcpy(systemCfg.wifi_ssid, mySSID.c_str());
        systemCfg.wifi_mode = WIFI_AP;
        return 1;
    } else {
        ESP_LOGE(TAG, "Fehler beim starten des AccessPoints!");
        systemCfg.wifi_ready = false;
        return 0;
    }
}
int wifi_init() {
    int status = 0;
    for(int i=0; i< 17; i=i+8) {
		chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
    cfg.chipid = chipId;
	mySSID = std::move("BNG-" + String(chipId));
    load_WiFiNetwork();
    if(!cfg.wifi_enabled) {
        ESP_LOGW(TAG, "WiFi ist nicht aktiviert.");
        systemCfg.wifi_ready = false;
        return 0;
    }
    if(cfg.wifi_mode == WIFI_STA) {
        //Stations modus
        if(cfg.wifi_ssid == NULL || sizeof(cfg.wifi_ssid) < 1) {
           status = startAP();      
                
        } else {
            try
            {
                WiFi.mode(WIFI_STA);
                WiFi.hostname(mySSID);
                strcpy(systemCfg.hostname, WiFi.getHostname());
                ESP_LOGI(TAG,"Verbinde zu WLAN %s und Passwort '%s'", cfg.wifi_ssid, cfg.wifi_password);
                status = WiFi.begin(cfg.wifi_ssid, cfg.wifi_password);
                if(check_wifi()){
                    ESP_LOGI(TAG,"Verbunden mit WLAN '%s'", WiFi.SSID());
                    ESP_LOGI(TAG, "Mein Hostname: '%s'", WiFi.getHostname());
                    ESP_LOGI(TAG, "lokale IP: %s", WiFi.localIP().toString());
                    systemCfg.wifi_ready = true;
                } else {
                    ESP_LOGW(TAG, "Konnte keine Verbindung zu SSID '%s' aufbauen, starte AccessPoint.", cfg.wifi_ssid);
                    status = startAP();
                }    /* code */
            }
            catch(const std::exception& e)
            {
                ESP_LOGE(TAG, "FEHLER %s", e.what() );
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
    systemCfg.heap = ESP.getHeapSize();
    systemCfg.freeheap = ESP.getFreeHeap();

    load_WiFiNetwork();
}

