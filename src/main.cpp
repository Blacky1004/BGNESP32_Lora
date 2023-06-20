#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "ESPAsyncWebServer.h"
#include <AsyncJson.h>
#include <ArduinoJson.h>

#include <AsyncTCP.h>
#include <lmic.h>
#include <hal\hal.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "DHTesp.h"
#include "SDS011.h"

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define KONFIG_MODE 0
#define WLAN_MODE 2
#define LORA_MODE 4

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define FORMAT_SPIFFS_IF_FAILED true

#define DEFAULT_AP_START "BNG" 

AsyncWebServer server(80);

//Unsere SSID im AP Mode = BGN-CHIPID
String mySSID ="";

uint32_t chipId = 0;
bool loraAvailable = false;
uint8_t aktualKonfigMode = KONFIG_MODE;

//LoRa TTN Device-Daten
// static u1_t NWKSKEY[16];
// static u1_t APPSKEY[16];
// static u4_t DEVADDR;
static const PROGMEM u1_t NWKSKEY[16] = { 0x22, 0xE2, 0xAD, 0x04, 0x62, 0xD6, 0x22, 0x56, 0xFE, 0x24, 0x3E, 0xDD, 0x7C, 0x4E, 0x26, 0x42 };

// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x6B, 0x50, 0x6C, 0x93, 0x9F, 0xAF, 0xF6, 0x24, 0xB0, 0xFC, 0x7D, 0x82, 0xD6, 0x66, 0x97, 0x8C};
// LoRaWAN end-device address (DevAddr)
static const u4_t DEVADDR = 0x260B19DF; // <-- Change this address for every node!

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

//Daten für die Website
float pm25Datas[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float pm10Datas[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float tempDatas[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
int countPos = 0;

static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;

const lmic_pinmap lmic_pins = { // Pins des TTGO ESP32 LoRa Board
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {26, 33, LMIC_UNUSED_PIN},
};
//LCD Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//-> Defaultkonfiguration
const char esp_default_config[] PROGMEM = R"rawliteral(
{
	"wifi": {
		"ssid":"",
		"password":"",
        "enabled": false
	},
	"location":{
		"latitude": 0,
		"longitude": 0,
		"height": 0
	},
	"lora":{
		"enabled": false,
		"c0": true,
		"c1": true,
		"c2": true,
		"c3": true,
		"c4": true,
		"c5": true,
		"c6": true,
		"c7": true,
        "newskey":[],
        "appskey":[],
        "devid":"260B19DF"
	},
	"data_setup":{
		"preferred":"lora_and_wlan"
	}
}	
)rawliteral";
const char esp_default_lora_payload[] PROGMEM = R"rawliteral(
    {
        "devid": 0,
        "pm25": 0.0,
        "pm10": 0.0,
        "temp": 0,
        "hum": 0,
        "hpa": 0,
        "location":{
            "height": 0,
            "latitude": 0.0,
            "longitude": 0.0
        }
    }
)rawliteral";
bool loraInitialized = false;
StaticJsonDocument<512> config;
StaticJsonDocument<255> jsonPayload;
//<- Ende Defaultkonfiguration

//-> Flashdaten
const unsigned char lcdlogo [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x0f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x0f, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfd, 0x01, 0xff, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x3f, 0xff, 0xc0, 0x1f, 0xf8, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0xff, 0xff, 0xf0, 0xff, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x1f, 0xff, 0x7e, 0x9f, 0xf7, 0xe7, 0xb6, 0x40, 0x00, 0x08, 0x00, 0x00, 0x3f, 0xff, 0xbf, 0xf3, 
	0xff, 0xef, 0xbb, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfc, 0xd9, 0xef, 0xbb, 0x00, 
	0x00, 0x7f, 0xc0, 0x00, 0xff, 0xff, 0xa0, 0x0f, 0xf9, 0xef, 0x7b, 0x40, 0x00, 0xff, 0xe0, 0x00, 
	0xff, 0xff, 0xc0, 0x07, 0x7b, 0xef, 0x7b, 0x00, 0x01, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xe0, 0x0f, 
	0x7b, 0xde, 0x76, 0x80, 0x01, 0xff, 0xf0, 0x01, 0xff, 0xff, 0xd0, 0x3b, 0x7b, 0xde, 0xee, 0x00, 
	0x03, 0xff, 0xe0, 0x03, 0xff, 0xff, 0xe0, 0x63, 0x7b, 0x9d, 0xfd, 0x00, 0x03, 0xff, 0xf0, 0x0e, 
	0xff, 0xff, 0x21, 0xc3, 0xf7, 0xbd, 0xfc, 0x00, 0x01, 0xff, 0xf0, 0x3f, 0xaf, 0x5a, 0xc3, 0x02, 
	0xf7, 0x7b, 0xba, 0x00, 0x01, 0xff, 0xc0, 0x7f, 0x29, 0x6f, 0x46, 0x06, 0xef, 0x77, 0xf0, 0x00, 
	0x01, 0xff, 0xe0, 0xfc, 0x04, 0x00, 0x1c, 0x07, 0xee, 0xf7, 0x68, 0x00, 0x00, 0xff, 0x83, 0xf0, 
	0x00, 0x00, 0x30, 0x0f, 0xdd, 0xee, 0xd0, 0x00, 0x01, 0x7f, 0x97, 0xe0, 0x60, 0x00, 0x60, 0x0f, 
	0xfd, 0xdd, 0xa0, 0x00, 0x00, 0xff, 0x4f, 0x83, 0xf0, 0x00, 0x60, 0x1f, 0xbb, 0xbd, 0x40, 0x00, 
	0x01, 0xff, 0xbf, 0x0f, 0xfc, 0x01, 0x80, 0x37, 0x77, 0x36, 0x00, 0x00, 0x03, 0xff, 0xfe, 0x0f, 
	0xfe, 0x01, 0x80, 0x6e, 0xef, 0x7d, 0x00, 0x00, 0x03, 0xff, 0xfc, 0x1f, 0xff, 0x03, 0x00, 0x7d, 
	0xde, 0xf8, 0x00, 0x00, 0x07, 0xff, 0xd0, 0x1f, 0xff, 0x06, 0x00, 0xfb, 0xbd, 0xec, 0x00, 0x00, 
	0x0f, 0xff, 0xe0, 0x3f, 0xff, 0x0c, 0x01, 0xf7, 0x3b, 0xd0, 0x00, 0x00, 0x0f, 0xff, 0xf0, 0x3f, 
	0xff, 0x0c, 0x03, 0xfe, 0xf7, 0x40, 0x00, 0x00, 0x1f, 0xff, 0xe8, 0x3f, 0xfd, 0x08, 0x0f, 0xdc, 
	0xee, 0x80, 0x00, 0x00, 0x3f, 0xff, 0xf4, 0x1f, 0xfd, 0x18, 0x0f, 0xbb, 0xda, 0x00, 0x00, 0x00, 
	0x3f, 0xff, 0xf4, 0x1f, 0xfe, 0x18, 0x3f, 0x77, 0xf4, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xf8, 0x0f, 
	0xfc, 0x18, 0x6e, 0xef, 0xc0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf4, 0x0f, 0xfa, 0x18, 0xff, 0xdf, 
	0x50, 0x00, 0x00, 0x00, 0x5f, 0xff, 0xe4, 0x0f, 0xf0, 0x1d, 0xf7, 0xf5, 0x00, 0x00, 0x00, 0x00, 
	0x0d, 0xff, 0xb8, 0x1f, 0xfc, 0x0e, 0xe7, 0xf4, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xb2, 0xc0, 0x3f, 
	0xfa, 0x17, 0xff, 0x20, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe8, 0x00, 0x7f, 0xfc, 0x36, 0x12, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0x60, 0x00, 0x7f, 0xfe, 0xfe, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x0f, 0xc0, 0x00, 0xff, 0xff, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0xc0, 0x01, 0xff, 
	0xff, 0xf4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0xc0, 0x01, 0xff, 0xfe, 0xd0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1e, 0xc0, 0x03, 0xff, 0xff, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x1e, 0xc0, 0x03, 0xff, 0xff, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0xe0, 0x07, 0xff, 
	0xff, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x70, 0x03, 0xff, 0xff, 0x80, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x3f, 0xff, 0xff, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x1f, 0xcf, 0xfc, 0xfb, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x7f, 0xcd, 
	0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xff, 0xff, 0xb0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xfc, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xa0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
//<- Ende Flashdaten

//-> Interne Methoden
bool readConfig();
bool saveConfig();
void writeFile(fs::FS &fs, String content);
String readFile(fs::FS &fs);
String wiFiListe = "";
//<- Ende Interne Methoden

static uint8_t sensorPayload[]="";

int hex2int(char *s) 
{
  int x = 0;
  for(;;) {
    char c = *s;
    if (c >= '0' && c <= '9') {
      x *= 16;
      x += c - '0'; 
    }
    else if (c >= 'A' && c <= 'F') {
      x *= 16;
      x += (c - 'A') + 10; 
    }
    else break;
    s++;
  }
  return x;
}
void generateWifiList() {
    wiFiListe = "<option value=\"null\">-- Auswahl --</option>";
    String selected ="";
    int n = WiFi.scanNetworks();
    if(n > 0) {
        for (int i = 0; i < n; ++i) {
            if(WiFi.SSID(i) == config["wifi"]["ssid"].as<String>()) {
                selected = "selected";
            }
            wiFiListe += "<option value=\""+WiFi.SSID(i)+"\" "+selected+">"+WiFi.SSID(i)+"</option>";
        }        
    }
}
void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, nicht senden"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    serializeJson(jsonPayload, sensorPayload);
    static uint8_t payload[] = "YOUR-PAYLOAD";
    LMIC_setTxData2(1, payload, sizeof(payload)-1, 0);
    Serial.println(F("Packet in der Warteschlange"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}
void loadLora(){
    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  #ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
#else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif
  

  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  //LMIC_disableChannel(0); //Send only at channel 0
  LMIC_disableChannel(1);
  LMIC_disableChannel(2);
  LMIC_disableChannel(3);
  LMIC_disableChannel(4);
  LMIC_disableChannel(5);
  LMIC_disableChannel(6);
  LMIC_disableChannel(7);
  LMIC_disableChannel(8);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start job
  do_send(&sendjob);

  loraInitialized = true;
}
void setAPMode(){

	Serial.println("starte AccessPoint...");
	WiFi.mode(WIFI_STA);
	WiFi.softAP(mySSID,"");
	Serial.print("Meine SSID lautet: ");
	Serial.println(mySSID);
	display.clearDisplay();
	display.setTextSize(1);      // Normal 1:1 pixel scale
  	display.setTextColor(WHITE);
  	display.setCursor(90,0);
	display.print("AP");
	display.setCursor(5,20);
	display.print("ACCESSPOINT MODUS");
	display.setCursor(5,40);
	display.print(mySSID);
	display.display();
}
bool readConfig(){
	String file_content = readFile(SPIFFS);

	int config_file_size = file_content.length();
	if(config_file_size > 512){
		Serial.println("config -> Datei zu groß!" );
		auto error = deserializeJson(config, esp_default_config);
		if(error) {
			Serial.println("Fehler beim laden der Werkseinstellungen!");
			return false;
		}
		return true;
	}

	auto error = deserializeJson(config, file_content);
	if(error){
		Serial.println("config -> Fehler beim lesen der Konfig!");
		auto error = deserializeJson(config, esp_default_config);
		if(error){
			Serial.println("Fehler beim laden der Werkseinstellungen!");
			return false;
		}
	}
	return true;
}

bool saveConfig(){
	String content = "";
	serializeJson(config, content);
	writeFile(SPIFFS, content);
	return true;
}

String readFile(fs::FS &fs){
	Serial.print("config -> lese Konfigurationsdatei... ");
	File file = fs.open("/config.json");
	if(!file || file.isDirectory()){
		Serial.println(" [nicht vorhanden]");		
		return "";
	}

	String fileText = "";
	while (file.available())
	{
		fileText = file.readString();
	}
	file.close();

	return fileText;
}

void writeFile(fs::FS &fs, String content) {
	Serial.println("config -> schreibe Konfigdatei...");

	File file = fs.open("/config.json", FILE_WRITE);
	if(!file){
		Serial.println("config -> Fehler beim schreiben der config!");
		return;
	}

	if(file.print(content)){
		Serial.println("config -> Konfiguration erfolgreich gespeichert.");
	} else{
		Serial.println("config -> Fehler beim schreiben der Konfiguration!");
	}
	file.close();
}


bool checkWiFi() {
    if(config["wifi"]["ssid"] != ""){
        int c = 0;
        Serial.print("Verbinde zu '");
        Serial.print(config["wifi"]["ssid"].as<const char*>());
        Serial.println("'...");
        WiFi.begin(config["wifi"]["ssid"].as<String>(), config["wifi"]["password"].as<String>());
        int x= 0;
        while (c < 20)
        {
            if(WiFi.status() == WL_CONNECTED){
                return true;
            }

            delay(500);
            Serial.print("*");
            c++;
            x += 7;
        }
        Serial.println("Verbindungstimeout. Starte AaccessPoint.");
        Serial.println("");
    }
    return false;
}


String htmlProcessor(const String& var) {
	Serial.println(var);
	if(var == "LORAAVAILABLE"){	
		String av = "false";	
		if(loraAvailable == true) {
			av = "true";
		}
		return av;
	}
	if(var == "CHIPID") {
		String av = "UNBEKANNT";
		av = mySSID;
		return av;
	}
    if(var == "WIFILIST"){
        return wiFiListe;
    }
	return String();		
}
void launchWebserver() {
    // server.rewrite("/", "index_default.html");
    // server.rewrite("/index.html", "index_ap.html").setFilter(ON_AP_FILTER);
    server.serveStatic("/",SPIFFS,"/");
    server.on("/", HTTP_GET , [](AsyncWebServerRequest *request) {
        String index = "/index_ap.html";
        if(WiFi.status() == WL_CONNECTED){
            index = "/index_default.html";
        }
		request->send(SPIFFS, index , String(), false, htmlProcessor);
	});
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/styles.css", "text/css");
	});
	server.on("/logo-buergernetzgeragreiz.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/logo-buergernetzgeragreiz.png", "image/png");
	});
	server.on("/nav_bg.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/nav_bg.png", "image/png");
	});
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/test_wifi", [](AsyncWebServerRequest *request, JsonVariant &json) {
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
        else if(data["enabled"] == false){
            config["wifi"]["ssid"] = "";
            config["wifi"]["password"] = "";
            config["wifi"]["enabled"] = false;
            response = "{\"code\": 200, \"message\":\"OK\"}";
            request->send(400, "application/json", response);
        }        
        Serial.println("request WLAN-Setup:");
        Serial.println(response);
    });
    AsyncCallbackJsonWebHandler *lora_handler = new AsyncCallbackJsonWebHandler("/test_lora", [](AsyncWebServerRequest *request, JsonVariant &json) {
        StaticJsonDocument<1024> data;
        if (json.is<JsonArray>())
        {
            data = json.as<JsonArray>();
        }
        else if (json.is<JsonObject>())
        {
            data = json.as<JsonObject>();
        }
        String response;
        serializeJson(data, response);
        // if(data["nwkey"] != NULL) {
        //     for(int i = 0; i < 15; i++){
        //         NWKSKEY[i] = data["nwkey"][i].as<u1_t>();
        //     }
        // }

        // if(data["apkey"] != NULL) {
        //     for(int i = 0; i < 15; i++){
        //         APPSKEY[i] = data["apkey"][i].as<u1_t>();
        //     }
        // }

        // if(data["devkey"] != NULL){
        //     DEVADDR = data["devkey"].as<u4_t>();
        // }
        Serial.println("request Lora-Setup:");
        Serial.println(response);
        Serial.print("DEVADDR: ");
        Serial.println(data["devkey"].as<String>());

        Serial.print("NwkSKey: ");
        Serial.println(data["nwkey"].as<String>());

        Serial.print("AppSKey: ");
        Serial.println(data["apkey"].as<String>());

        loadLora();
    });
    server.addHandler(handler);
    server.addHandler(lora_handler);
    server.begin();
}

void setup(){
    Serial.begin(115200);
    delay(1000);
    WiFi.disconnect();
    SPI.begin(SCK, MISO, MOSI, SS);
    Serial.println("Starte System....");
loadLora();
    // -> Display initialisierung
	pinMode(OLED_RST, OUTPUT);
	digitalWrite(OLED_RST, LOW);
	delay(20);
	digitalWrite(OLED_RST, HIGH);
	Wire.begin(OLED_SDA, OLED_SCL);
	//Displayinitialisierung ENDE <-

    //Prüfung ob Display vorhanden
	if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
		//setz der Variable displayAvailable auf false
		Serial.println("init -> kein LCD vorhanden.");
	} else {
		display.clearDisplay();
		display.drawBitmap(20,0, lcdlogo, 91,64, WHITE);
		display.display();
	}

    //ermitteln der ChipId
	for(int i=0; i< 17; i=i+8) {
		chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
	mySSID = std::move(String(DEFAULT_AP_START) + "-" + String(chipId));

	Serial.printf("ESP32 Chip Model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
	Serial.printf("Dieser Chip hat %d Kerne\n", ESP.getChipCores());
  	Serial.print("Chip ID: "); Serial.println(chipId);
  	
	if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
		Serial.println("init -> SPIFF Mounting Fehler! System stopped.");
		for(;;);
	}

	Serial.println("init -> SPIFF gemountet.");		
    generateWifiList();
	//Lesen der Konfiguration 'config.json' (wenn) vorhanden
    if(!readConfig()){
        // System in den Konfigmodus stellen
        aktualKonfigMode = KONFIG_MODE;

    } else {
        if(checkWiFi()) {
            aktualKonfigMode = WLAN_MODE;
        }
        auto error = deserializeJson(jsonPayload, esp_default_lora_payload);
        if(error){
            
        }
        
    }
    if(aktualKonfigMode == KONFIG_MODE){
        setAPMode();
    }
    launchWebserver();
}

void loop() {
    if(loraInitialized) {
        os_runloop_once();
    }
    if(millis() / 1000 * 60 == 60){
        generateWifiList();
    }
}

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}