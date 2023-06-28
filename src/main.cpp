#include <Arduino.h>
#include <Wire.h>
#include <lmic.h>
#include <hal\hal.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "ESPAsyncWebServer.h"
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "DHTesp.h"
#include "SDS011.h"
#include <HTTPClient.h>
#include <hw_scan.h>
#include <map>
#include <lora_device.h>
#include <Preferences.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define KONFIG_MODE 0
#define WLAN_MODE 2


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define FORMAT_SPIFFS_IF_FAILED true
#define DEFAULT_AP_START "BNG" 

AsyncWebServer server(80);

//Unsere SSID im AP Mode = BGN-CHIPID
String mySSID ="";

uint32_t chipId = 0;
bool loraAvailable = false;
bool wlanAvailable = false;
bool expertMode = false;
uint8_t aktualKonfigMode = KONFIG_MODE;

//LoRa TTN Device-Daten
// static u1_t NWKSKEY[16];
// static u1_t APPSKEY[16];
// static u4_t DEVADDR;
// Those variables keep their values after software restart or wakeup from sleep, not after power loss or hard reset !
RTC_NOINIT_ATTR int RTCseqnoUp, RTCseqnoDn;
bool resetLora = false;

#ifdef USE_OTAA
RTC_NOINIT_ATTR u4_t otaaDevAddr;
RTC_NOINIT_ATTR u1_t otaaNetwKey[16];
RTC_NOINIT_ATTR u1_t otaaApRtKey[16];
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}
#else
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
#endif

//Daten für die Website
float pm25Datas[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float pm10Datas[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
float tempDatas[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};


static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;

const lmic_pinmap lmic_pins = { // Pins des TTGO ESP32 LoRa Board
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {26, 33, LMIC_UNUSED_PIN},
};

//LCD Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RST);
Preferences preferences;

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
        "newskey":null,
        "appskey":null,
        "devid": "",
        "devaddr":"260B19DF"
	},
	"data_setup":{
		"preferred":"lora_and_wlan"
	}
}	
)rawliteral";
String sensorResultDatas = "{\"pm10\": [], \"pm25\":[], \"tmp\":[]}";
bool loraInitialized = false;
DynamicJsonDocument config(1024);
StaticJsonDocument<512> jsonPayload;

//<- Ende Defaultkonfiguration
unsigned long lastTime = 0;
unsigned long timerDelay = 60000;
unsigned long lastDisplayTime = 0;
unsigned long timerDisplay = 5000;
int displayView = 0;

//-> Sensordaten vars
float temp, humidity, pressure, altitude, pm25, pm10;
double latitude, longitude;
std::map<std::string, byte> myHardware;
//<- Ende Sensordaten

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

char s[32];
uint8_t txBuffer[9];
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

String IpAddress2String(const IPAddress& ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

void storeFrameCounters()
{
    if(resetLora) return;
    RTCseqnoUp = LMIC.seqnoUp;
    RTCseqnoDn = LMIC.seqnoDn;
    sprintf(s, "Counters stored as %d/%d", LMIC.seqnoUp, LMIC.seqnoDn);
    preferences.begin("fcnt", false);
    preferences.putInt("seqnoUp", RTCseqnoUp);
    preferences.putInt("seqnoDn", RTCseqnoDn);
    preferences.end();
    Serial.println(s);
}

void restoreFrameCounters()
{
  preferences.begin("fcnt", true);
  RTCseqnoDn = preferences.getInt("seqnoDn", 0);
  RTCseqnoUp = preferences.getInt("seqnoUp", 0);
  preferences.end();
  LMIC.seqnoUp = RTCseqnoUp;
  LMIC.seqnoDn = RTCseqnoDn;
  sprintf(s, "Restored counters as %d/%d", LMIC.seqnoUp, LMIC.seqnoDn);
  Serial.println(s);
}

void setOrRestorePersistentCounters()
{
  esp_reset_reason_t reason = esp_reset_reason();
  if ((reason != ESP_RST_DEEPSLEEP) && (reason != ESP_RST_SW))
  {
    Serial.println(F("Counters both set to 0"));
    LMIC.seqnoUp = 0;
    LMIC.seqnoDn = 0;
  }
  else
  {
    restoreFrameCounters();
  }
}

void sortPm25Data(float newValue) {
    for(int i = 0; i < 9; i++){
        pm25Datas[i] = pm25Datas[i+1];
    }
    pm25Datas[9] = newValue;
}

void sortPm10Data(float newValue) {
    for(int i = 0; i < 9; i++){
        pm10Datas[i] = pm10Datas[i+1];
    }
    pm10Datas[9] = newValue;
}

void sortTmpData(float newValue) {
    for(int i = 0; i < 9; i++){
        tempDatas[i] = tempDatas[i+1];
    }
    tempDatas[9] = newValue;
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
    
    static uint8_t payload[9];
    int pTmp = ((int)(tempDatas[9] * 100)) + 5000;
    int pPre = (int)(pressure * 10);
    byte pHum = (int) (humidity * 2);
    int pP10 = (int)(pm10Datas[9] * 100);
    int pP25 = (int)(pm25Datas[9] * 100);
    payload[0] = pTmp >> 8;
    payload[1] = pTmp;
    payload[2] = pPre >> 8;
    payload[3] = pPre;
    payload[4] = pHum;
    payload[5] = pP10 >> 8;
    payload[6] = pP10;
    payload[7] = pP25 >> 8;
    payload[8] = pP25;
    for(int i = 0; i < sizeof( payload); i++)
    {
    Serial.println(payload[i]);
    }
    LMIC_setTxData2(1, payload, sizeof(payload)-1, 0);
    Serial.println(F("Packet in der Warteschlange"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void loadLora(){

    // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

#ifdef USE_OTAA
  esp_reset_reason_t reason = esp_reset_reason();
  if ((reason == ESP_RST_DEEPSLEEP) || (reason == ESP_RST_SW))
  {
    LMIC_setSession(0x1, otaaDevAddr, otaaNetwKey, otaaApRtKey);
  }
#else // ABP
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Disable link check validation
  LMIC_setLinkCheckMode(0);
#endif
  
  restoreFrameCounters();

  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start job
  do_send(&sendjob);
  
  // TTN uses SF9 for its RX2 window.
  //LMIC.dn2Dr = DR_SF9;
  loraInitialized = true;
  loraAvailable = true;
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

String readFile(fs::FS &fs){
	Serial.print("config -> lese Konfigurationsdatei... ");
	File file = fs.open("/config.json");
	if(!file || file.isDirectory()){
		Serial.println(" [nicht vorhanden]");	
        Serial.println("config -> erzeuge neue Datei...");
        	
        writeFile(fs, String(esp_default_config));
		return String(esp_default_config);
	}
    else {
        Serial.println("[OK]");
    }

	String fileText = "";
	while (file.available())
	{
		fileText = file.readString();
	}
	file.close();

    Serial.println(fileText);
	return fileText;
}

void writeFile(fs::FS &fs, String content) {
	Serial.println("config -> schreibe Konfigdatei...");

	File file = fs.open("/config.json", FILE_WRITE);
	if(!file){
		Serial.println("config -> Fehler beim schreiben der Konfiguration!");
		return;
	}

	if(file.print(content)){
		Serial.println("config -> Konfiguration erfolgreich gespeichert.");
	} else{
		Serial.println("config -> Fehler beim schreiben der Konfiguration!");
	}
	file.close();
}

bool readConfig(){
	String file_content = readFile(SPIFFS);

	int config_file_size = file_content.length();
	if(config_file_size > 512){
		Serial.println("config -> Datei zu groß!" );
		auto error = deserializeJson(config, esp_default_config);
		if(error) {
			Serial.println("config -> Fehler beim laden der Werkseinstellungen!");
			return false;
		}
		return true;
	}

	auto error = deserializeJson(config, file_content);
	if(error){
		Serial.println("config -> Fehler beim lesen der Konfig!");
		auto error = deserializeJson(config, esp_default_config);
		if(error){
			Serial.println("config -> Fehler beim laden der Werkseinstellungen!");
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

void scanHardware(){
    byte error, address;
    int nDevices = 0;
    for(address = 1; address < 127; address++){
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if(error == 0) {
            Serial.print("scan -> Sensor gefunden bei Adresse 0x");
            if(address < 16) {
                Serial.print("0");
            }
            Serial.println(address, HEX);
            nDevices++;
        }
        else if ( error == 4)
        {
            Serial.print("scan -> Unbekannter Fehler bei Adresse 0x");
            if(address < 16) {
                Serial.print("0");
            }
            Serial.println(address, HEX);
        }
        std::map<std::string, byte> t = GetHardware(address);
        for(std::pair<std::string, byte> e : myHardware){
                auto a = myHardware.find(e.first.c_str());
                if(a != myHardware.end()) {
                    myHardware.insert({e.first.c_str(), e.second});
                }
            }    }
    if(nDevices == 0) {
        Serial.println("scan -> Keine unterstützten I2C Sensoren gefunden.");
    }
    else{
        Serial.println("scan -> Scan abgeschlossen.");
    }
}

String htmlProcessor(const String& var) {	
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
    if(var == "CHIPMODEL") {
        return ESP.getChipModel();
    }
    if(var == "CHIPREV") {
        return String(ESP.getChipRevision());
    }
    if(var == "CORES") {
        return String(ESP.getChipCores());
    }
    if(var == "MHZ"){
        return String(ESP.getCpuFreqMHz());
    }
    if(var == "RAWCHIPID"){
        return String(chipId);
    }
    if(var == "FREEHEAP") {
        return String(ESP.getFreeHeap());
    }
    if(var == "HEAP"){
        return String(ESP.getHeapSize());
    }
    if(var == "FLASHZIZE") {
        return String(ESP.getFlashChipSize());
    }
    if(var == "WIFISTATUS") {
        if (aktualKonfigMode == KONFIG_MODE)
        {
            return "AccessPoint";
        }
        else{
            return "WLAN-Verbindung";
        }        
    }
    if(var == "SSID") {
        if(aktualKonfigMode == KONFIG_MODE){
            return mySSID;
        }
        else{
            return WiFi.SSID();
        }
    }
    if(var == "MYIP") {
        if(aktualKonfigMode == KONFIG_MODE){
            return String(IpAddress2String(WiFi.softAPIP()));
        }
        else {
            return String(IpAddress2String(WiFi.localIP()));
        }
    }
    if(var == "EXPERTMODE") {
        if(expertMode) {
            return "true";
        }
        else {
            return "false";
        }
    }
	if(var == "DEVID") {
        return String(config["lora"]["devid"].as<String>());
    }
    if(var == "INTERVAL") {
        return String(TX_INTERVAL);
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
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"code\": 200}");
        delay(2000);
        ESP.restart();
    });
    server.on("/get_cfile", HTTP_GET, [](AsyncWebServerRequest *request) {
        int params = request->params();
        String content = "";
        AsyncWebParameter* p = request->getParam(0);
        if(p->name().c_str() == "cfg_file"){
            String cFile = "/";
            cFile += p->value();
            File file = SPIFFS.open(cFile.c_str());
            if(file){
                while (file.available()){
                    content = file.readString();
                }
                file.close();
            }
            
            String result= "{\"code\": 200, \"content\":";
            result += content;
            result += "\"}";
            request->send(200, "application/json", result);
            result= String();
        }
    });
    server.on("/get_cfiles", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument jsdoc(1024);
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        JsonArray fl = jsdoc.createNestedArray("files");
        while(file){
            if(String(file.name()).substring( String(file.name()).length() - 5) ==".json"){
                fl.add(file.name());
            }
        }
        jsdoc["code"] = 200;
        String json = "";
        serializeJson(jsdoc, json);
        request->send(200, "application/json", json);
        json = String();
    });
    AsyncCallbackJsonWebHandler *geohandler = new AsyncCallbackJsonWebHandler("/geocode", [](AsyncWebServerRequest *request, JsonVariant &json){
        StaticJsonDocument<200> data;
        if (json.is<JsonArray>())
        {
            data = json.as<JsonArray>();
        }
        else if (json.is<JsonObject>())
        {
            data = json.as<JsonObject>();
        }
        DynamicJsonDocument jdoc(255);
        if(WiFi.status() == WL_CONNECTED){
            HTTPClient client;

            String street = data["street"].as<String>();
            String hnr = data["hnr"].as<String>();
            String plz = data["zip"].as<String>();
            String city = data["city"].as<String>();
street.replace(" ", "+");
city.replace(" ", "+");
hnr.replace(" ", "+");
            String q = "https://nominatim.openstreetmap.org/search?q=" + street + "+" + hnr + "+" + plz+ "+" + city +"&format=json&polygon=1&addressdetails=1";
            client.begin(q.c_str());
            int httpResponseCode = client.GET();
      
            if (httpResponseCode>0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = client.getString();
                Serial.println(payload);
            }
            else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
            // Free resources
            client.end();
        }
        

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

        Serial.println("request Lora-Setup:");
        Serial.println(response);

        bool oldLoraStatus = config["lora"]["enabled"];
        config["lora"]["devid"] = data["devid"];
        config["lora"]["devaddr"] = data["devkey"];
        config["lora"]["newskey"] = data["nwkey"];
        config["lora"]["appskey"] = data["apkey"];
        config["lora"]["enabled"] = data["enabled"];
        
        String newCfg = "";
        serializeJson(config, newCfg);
        Serial.println(newCfg);
        lastTime = millis();
        if(saveConfig()){
            if(config["lora"]["enabled"] == true) {
                loadLora();
            }  
            String myIP = "";
            DynamicJsonDocument resp(512);
            if(aktualKonfigMode == KONFIG_MODE){
                myIP = IpAddress2String(WiFi.softAPIP());
            }      
            else {
                myIP = IpAddress2String(WiFi.localIP());
            }
            if(oldLoraStatus == true && config["lora"]["enabled"] == false) {
                resp["need_restart"] = true;
            } else{
                resp["need_restart"] = false;
            }
            resp["code"] = 200;
            resp["uri"] = myIP;
            String json = "";
            serializeJson(resp, json);
            request->send(200, "application/json", json);
            json = String();
        }
        else {
            request->send(200, "application/json", "{\"code\":500, \"message\": \"Fehler beim speichern der Loradaten!\"}");
        }
    });
    AsyncCallbackJsonWebHandler *expert_handler = new AsyncCallbackJsonWebHandler("/set_expert", [](AsyncWebServerRequest *request, JsonVariant &json) {
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
        if(!data.containsKey("mode") || data["mode"] == ""  || data["mode"] == "null") {
            response = "{\"code\": 400, \"message\":\"nicht erlaubt.\"}";
            request->send(400, "application/json", response);
        }
        else if(data["mode"] == true){  
            expertMode = true;          
            response = "{\"code\": 200, \"message\":\"OK\"}";
            request->send(400, "application/json", response);
        }        
        Serial.println("request WLAN-Setup:");
        Serial.println(response);
    });
    server.on("/getchartdatas", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", sensorResultDatas);
    });
    server.on("/myconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
        String result = "";
        serializeJson(config, result);
        request->send(200, "application/json", result);
        result= String();
    });
    server.on("/factory_defaults", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(SPIFFS.exists("/config.json")){
            SPIFFS.remove("/config.json");
        }

        if(SPIFFS.exists("/device.json")){
            SPIFFS.remove("/device.json");
        }
        Serial.println("system -> System wird auf Werkseinstellung zurückgesetzt.");
        delay(1000);
        ESP.restart();
    });
    server.on("/reset_lora", HTTP_GET, [](AsyncWebServerRequest * request) {
        Serial.println("system -> Resete LoraCounter..");
        resetLora = true;
        preferences.begin("fcnt", false);
        preferences.putInt("seqnoUp", 0);
        preferences.putInt("seqnoDn", 0);
        preferences.end();
        delay(2000);
        ESP.restart();
    });
    server.on("/lora_info", HTTP_GET, [](AsyncWebServerRequest *request) {
        String result = "{\"uplink\":" + String(RTCseqnoUp)+",\"downlink\":" + String(RTCseqnoDn)+"}";
        request->send(200, "application/json", result);
        result = String();        
    });
    server.addHandler(geohandler);
    server.addHandler(handler);
    server.addHandler(lora_handler);
    server.begin();
}

void drawCentreString(const char *buf, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    display.setCursor(x - w / 2, y);
    display.print(buf);
}

void setup(){
    Wire.begin();
    Serial.begin(115200);
    delay(1000);
    WiFi.disconnect();
    SPI.begin(SCK, MISO, MOSI, SS);
    Serial.println("Starte System....");

    // -> Display initialisierung
	pinMode(OLED_RST, OUTPUT);
	digitalWrite(OLED_RST, LOW);
	delay(20);
	digitalWrite(OLED_RST, HIGH);
	Wire1.begin(OLED_SDA, OLED_SCL);
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
    Serial.println("init -> suche nach Sensoren...");
    scanHardware();
    
    //Lesen der Konfiguration 'config.json' (wenn) vorhanden
    if(!readConfig()){
        // System in den Konfigmodus stellen
        aktualKonfigMode = KONFIG_MODE;
    } else {       
        if(checkWiFi()) {
            aktualKonfigMode = WLAN_MODE;
        }        
    }

    if(aktualKonfigMode == KONFIG_MODE){
        setAPMode();
    } else {
        Serial.print("wifi -> IP: ");
        Serial.println(WiFi.localIP());
    }
    randomSeed(10041973);
    loadLora();    
    launchWebserver();
}

void loop() {
    if(loraInitialized) {
        os_runloop_once();
    }
    if((millis() - lastTime) > timerDelay){
        generateWifiList();

        Serial.println("system -> lese Sensordaten...");
        //Vorerst reine Testdaten
        long random10 = random(100,40000);
        long random25 = random(100,40000);
        long temp = random(1800,3900);
        sortPm10Data(random10 / 100.0);
        sortPm25Data(random25 / 100.0);
        sortTmpData(temp / 100.0);

        long press = random(30000, 107500);
        pressure = press / 100;
        long hum = random(1000, 10000);
        humidity = hum / 100;
        //Ende Randomtestdaten
        Serial.println("system -> Sensordaten gelesen...");
        DynamicJsonDocument jdoc(512);
        JsonArray pm25 = jdoc.createNestedArray("pm25");
        for(int i= 0; i < 10; i++) {
            pm25.add(pm25Datas[i]);
        }
        
        JsonArray pm10 = jdoc.createNestedArray("pm10");
        for(int i= 0; i < 10; i++) {
            pm10.add(pm10Datas[i]);
        }

        JsonArray tarr = jdoc.createNestedArray("tmp");
        for(int i= 0; i < 10; i++) {
            tarr.add(tempDatas[i]);
        }
        sensorResultDatas = "";
        serializeJson(jdoc, sensorResultDatas);
        lastTime = millis();
        
    }
    wlanAvailable = WiFi.status() == WL_CONNECTED ? true : false;    

    if((millis() - lastDisplayTime) > timerDisplay){
        //display.fadeout();
        display.clearDisplay();
        String dMode = "AP";
        if(aktualKonfigMode == KONFIG_MODE){
            dMode = "AP";
            if(loraAvailable && loraInitialized){
                dMode = "AP LW";
            }
        }
        else if (aktualKonfigMode == WLAN_MODE && wlanAvailable)
        {
            if(loraAvailable){
                dMode = "WL LW";
            }
            else {
                dMode = "WL";
            }
        }
        else {
            if(loraAvailable){
                dMode = "   LW";
            }
        }
        display.setTextSize(1);
        display.setCursor(90,0);
        display.print(dMode);
        switch(displayView){
            case 0: {
                display.setTextSize(1);
                display.setCursor(10,20);
                display.print("Temperatur:");
                display.setTextSize(2);
                display.setCursor(10,40);
                display.print(tempDatas[9]);
                display.setTextSize(1);
                display.cp437(true);
                display.write(167);
                display.setTextSize(2);
                display.print("C");
                display.display();
            }break;
            case 1: {
                display.setTextSize(1);
                display.setCursor(10,20);
                display.print("Pm10:");
                display.setCursor(10,40);
                display.setTextSize(2);
                display.print(pm10Datas[9]);
                display.cp437(true);
                display.write(230);
                display.print("g/m");
                display.setTextSize(1);
                display.print("3");display.display();
            } break;
            case 2: {
                display.setTextSize(1);
                display.setCursor(10,20);
                display.print("Pm25:");
                display.setTextSize(2);
                display.setCursor(10,40);
                display.print(pm25Datas[9]);
                display.cp437(true);
                display.write(230);
                display.print("g/m");
                display.setTextSize(1);
                display.print("3");
                display.display();
            } break;
            case 3: {
                display.setTextSize(1);
                display.setCursor(10,20);
                display.print("Luftfeuchte:");
                display.setTextSize(2);
                display.setCursor(10,40);
                display.print("44%");
                display.display();
            }
        }
        //display.fadein();
        displayView++;
        if(displayView > 3) {
            displayView = 0;
        }
        lastDisplayTime = millis();
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
#ifdef USE_OTAA    
      otaaDevAddr = LMIC.devaddr;
      memcpy_P(otaaNetwKey, LMIC.nwkKey, 16);
      memcpy_P(otaaApRtKey, LMIC.artKey, 16);
      sprintf(s, "got devaddr = 0x%X", LMIC.devaddr);
      Serial.println(s);
#endif
        // Disable link check validation (automatically enabled
        // during join, but not supported by TTN at this time).
        LMIC_setLinkCheckMode(0);
        // TTN uses SF9 for its RX2 window.
        LMIC.dn2Dr = DR_SF9;
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
      storeFrameCounters();
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
      case EV_TXSTART:
        Serial.println(F("EV_TXSTART"));
      break;
      case EV_JOIN_TXCOMPLETE:{
        Serial.println(F("EV_JOIN_TXCOMPLETE"));
      }break;
    default:
      Serial.print(F("Unknown event "));
      Serial.println(ev);
      break;
  }
}