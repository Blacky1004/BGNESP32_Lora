#include "globals.h"
#include "config_bng.h"

#define DEVCONFIG "loracfg"

Preferences nvram;

const char esp_default_config[] PROGMEM = R"rawliteral(
{
	"wifi": {
		"ssid":"",
		"password":"",
        "enabled": true
	},
	"location":{
		"latitude": 0,
		"longitude": 0,
		"height": 0
	},
	"lora":{
		"enabled": true,		
        "newskey":null,
        "appskey":null,
        "devid": "",
        "devaddr":"260B19DF",
        "interval": 60
	},
	"data_setup":{
		"preferred":"lora_and_wlan"
	}
}	
)rawliteral";

bool loadConfig(void) {
	String file_content = readConfigFile(SPIFFS);
	int config_file_size = file_content.length();
	if(config_file_size > 1024){
		ESP_LOGE(TAG, "Datei zu groß! Max. 1024Kb.");
		auto error = deserializeJson(config, esp_default_config);
		if(error) {
			ESP_LOGE(TAG, "Fehler beim laden der Werkseinstellungen!");
			return false;
		}
		return true;
	}

	auto error = deserializeJson(config, file_content);
	if(error){
		ESP_LOGE(TAG,"Fehler beim lesen der Konfig!");
		auto error = deserializeJson(config, esp_default_config);
		if(error){
            ESP_LOGE(TAG, "config -> Fehler beim laden der Werkseinstellungen!");
			return false;
		}
	}
	return true;
}

String readConfigFile(fs::FS &fs) {
	ESP_LOGI(TAG,"lese Konfigurationsdatei... ");
	File file = fs.open("/config.json");
	if(!file || file.isDirectory()){
		ESP_LOGW(TAG,"(noch) nicht [nicht vorhanden]");	
        ESP_LOGI(TAG, "erzeuge neue Datei...");
        	
        writeConfigFile(fs, String(esp_default_config));
		return String(esp_default_config);
	}
    else {
        ESP_LOGI(TAG,"[OK] Eingelesen");
    }

	String fileText = "";
	while (file.available())
	{
		fileText = file.readString();
	}
	file.close();    
	return fileText;
}
void writeConfigFile(fs::FS &fs, String content) {

}