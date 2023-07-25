#include "globals.h"
#include "config_bng.h"
#include <SPIFFS.h>
#include <vector>
#define DEVCONFIG "bngcfg"

Preferences nvram;

systemConfig_t cfg;
systemvars_t systemCfg;
std::vector<resturls_t> urlList;

static const uint8_t cfgMagicBytes[] = {0x0A, 0x04, 0x73, 0x08, 0x04};
static const size_t cfgLen = sizeof(cfg), cfgLen2 = sizeof(cfgMagicBytes);
static uint8_t buffer[cfgLen + cfgLen2];
float pm10Datas[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float pm25Datas[]= {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float tempDatas[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float humdatas[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static void defaultConfig(systemConfig_t *myConfig) {
	strncpy(myConfig->version, VERSION, sizeof(myConfig->version) - 1);
	
	auto appeui = std::initializer_list<u1_t>({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
	std::copy(appeui.begin(), appeui.end(), myConfig->appeui);
	
	auto appkey = std::initializer_list<u1_t>({0xA1, 0xF5, 0x4F, 0x2A, 0xE6, 0xD6, 0xD6, 0x64, 0xFE, 0xBF, 0x67, 0x9A, 0x5B, 0x3C, 0x34, 0x61});
	std::copy(appkey.begin(), appkey.end(), myConfig->appkey);

	auto deveui = std::initializer_list<u1_t>({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
	std::copy(deveui.begin(), deveui.end(), myConfig->deveui);

	auto nwkey = std::initializer_list<u1_t>({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
	std::copy(nwkey.begin(), nwkey.end(), myConfig->nwkskey);

	myConfig->lora_is_abp = false;
	myConfig->sendtype = LORA_ONLY,
	myConfig->wifi_enabled = true;
	myConfig->loradr = LORADRDEFAULT;
	myConfig->txpower = LORATXPOWDEFAULT;
	myConfig->adrmode = 1;
	myConfig->screensaver = 0;
	myConfig->screenon = 1;
	myConfig->countermode = COUNTERMODE;
	myConfig->rssilimit = RSSILIMIT;
	myConfig->sendcycle = SENDCYCLE;
	myConfig->sleepcycle = SLEEPCYCLE;
	myConfig->wakesync = SYNCWAKEUP;
	myConfig->payloadmask = PAYLOADMASK;
	myConfig->wifi_mode = WIFI_STA;
	myConfig->wifi_enabled = true;
	myConfig->cpuspeed = ESP.getCpuFreqMHz();
	myConfig->wakeup_manual = false;
	myConfig->homecycle = HOMECYCLE;
	myConfig->payloadqueue = SEND_QUEUE_SIZE;
	myConfig->latitude = 0.0;
	myConfig->longitude = 0.0;
	
	strcpy(myConfig->wifi_ssid, "");
	strcpy(myConfig->wifi_password, "");	
}

static void migrateConfig(void) {
	eraseConfig();
}

void saveConfig(bool erase) {
	ESP_LOGI(TAG, "Speichere Einstellungen in den NVRAM");
	nvram.begin(DEVCONFIG, false);
	if(erase) {
		ESP_LOGI(TAG, "setze Gerät in die Werkseinstellungen");
		nvram.clear();
		defaultConfig(&cfg);
	}

	//Kopieren der Konfig 'cfg' zum ByteArray incl. dem MagicByte
	memcpy(buffer, &cfg, cfgLen);
	memcpy(buffer + cfgLen, &cfgMagicBytes, cfgLen2);

	//abspeichern im NVRAM
	if(nvram.putBytes(DEVCONFIG, buffer, cfgLen + cfgLen2)){
		ESP_LOGI(TAG, "Geräteeinstellungen gespeichert.");
	} else {
		ESP_LOGE(TAG, "NVRAM FEHLER, Geräteeinstellungen nicht gespeichert!");
	}

	nvram.end();
}

void loadConfig(void) {
	int readBytes = 0;
	ESP_LOGI(TAG, "lade Geräteeinstellungen vom NVRAM...");

	if (nvram.begin(DEVCONFIG, true)) {
		//laden der Geräteeinstellungen vom NVRAM und kopieren in ein byteArray
		 readBytes = nvram.getBytes(DEVCONFIG, buffer, cfgLen + cfgLen2);
		 nvram.end();

		 if(readBytes != cfgLen + cfgLen2) {
			ESP_LOGE(TAG, "Ungültige Konfiguration gefunden.");
			migrateConfig();
		 }
	} else {
		ESP_LOGI(TAG, "NVRAM initialisiert, Gerät startet mit Werkseinstellungen.");
		eraseConfig();
	}

	//prüfe die Gültigkeit der Konfiguration Anhand des MagicBytes
	if(memcmp(buffer + cfgLen, &cfgMagicBytes, cfgLen2) != 0){
		ESP_LOGE(TAG, "Konfigurationsdaten ungültig/korrupt!");
		eraseConfig();
	}

	//laden der Konfiguration in die Konfigstruktur
	memcpy(&cfg, buffer, cfgLen);
	ESP_LOGI(TAG, "Konfiguration v%s geladen", cfg.version);

	//prüfen ob die eingelesene Version mit der aktuellen Firmware übereinstimmt
	switch (version_compare(VERSION, cfg.version))
	{
	case  -1:
		ESP_LOGE(TAG, "Die Konfiguration v%s ist mit der aktuellen Firmware v%s nicht kompatibel!", cfg.version, VERSION);
		eraseConfig();
		break;
	case 1:
		ESP_LOGW(TAG, "Das Gerät wurde aktualisiert. , versuche die Konfiguration zu migrieren.");
		migrateConfig();
		break;
	default: //Die KOnfiguration ist aktuell
		break;
	}
}

void loadRestUrls(void) {
	DynamicJsonDocument doc(2048);
	File jsonFile = SPIFFS.open("/rest_clients.json", "r");
	if(jsonFile){
		deserializeJson(doc, jsonFile);
		if(doc.containsKey("clients")){
			urlList.clear();
			JsonArray jarr = doc["clients"].as<JsonArray>();
			for(JsonObject o: jarr) {
				resturls_t t = {.can_delete = o["can_delete"]};
				strcpy(t.api_key,  o["apikey"]);
				t.id = o["id"];
				strcpy(t.url, o["url"]);
				urlList.push_back(t);
			}
		}
	}
}

void saveRestUrls(void) {
	DynamicJsonDocument doc(2048);
	JsonArray arr = doc.createNestedArray("clients");
	uint16_t i = 0;
	for(resturls_t item : urlList){
		JsonObject o = arr.createNestedObject();
		o["id"] = i++;
		o["url"] = item.url;
		o["apikey"] = item.api_key;
		o["can_delete"] = item.can_delete;
	}
	String json = "";
	File jsonFile = SPIFFS.open("/rest_clients.json", "w");
	serializeJson(doc, jsonFile);
	jsonFile.close();
}

uint16_t insertRestUrl(String url, String apiKey ,bool can_delete) {
	uint16_t nextId = sizeof(urlList) / sizeof(resturls_t);
	
	resturls_t newEntry = {.can_delete = can_delete};
	newEntry.id = nextId;
	strcpy(newEntry.api_key,  apiKey.c_str());
	strcpy(newEntry.url, url.c_str());
	urlList.push_back(newEntry);
	saveRestUrls();
	return newEntry.id;
}

bool comp(char s1, char s2) { return (tolower(s1) < tolower(s2));}

int version_compare(const String v1, const String v2) {
	if(v1 == v2)
		return 0;

	const char *a1 = v1.c_str(), *a2 = v2.c_str();

	if (std::lexicographical_compare(a1, a1 + strlen(a1), a2, a2 + strlen(a2),
									comp))
	return -1;
	else
	return 1;	
}

void eraseConfig(void) {
	reset_rtc_vars();
	saveConfig(true);
}

void sort_chart_data(String a_name, float newValue) {

	if(a_name == "tempDatas") {
		for(int i = 0; i < 9; i++){
        	tempDatas[i] = tempDatas[i+1];
    	}
    	tempDatas[9] = newValue;
	}
	else if(a_name ==  "pm25Datas") {
		for(int i = 0; i < 9; i++){
        	pm25Datas[i] = pm25Datas[i+1];
    	}
    	pm25Datas[9] = newValue;
	}
	else if(a_name ==  "pm10Datas") {
		for(int i = 0; i < 9; i++){
        	pm10Datas[i] = pm10Datas[i+1];
    	}
    	pm10Datas[9] = newValue;
	}
	else if(a_name == "humdatas") {
		for(int i = 0; i < 9; i++){
        	humdatas[i] = humdatas[i+1];
    	}
    	humdatas[9] = newValue;
	}
}