#include "main.h"

char clientId[20] = {0}; //eindeutige ClientID 

void setup() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    #if(VERBOSE) 
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    #else
    esp_log_level_set("*", ESP_LOG_NONE);
    #endif


    #if(VERBOSE) 
    
    #endif
}

void loop() {

}