#include "main.h"

char clientId[20] = {0}; //eindeutige ClientID 

void setup() {
    char features[100] = "";
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    #if(VERBOSE) 
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    #else
    esp_log_level_set("*", ESP_LOG_NONE);
    #endif

    do_after_reset();
    ESP_LOGI(TAG, "Starte System v%s (Runmode=%d / Neustarts=%d)", PROGVERSION, RTC_runmode, RTC_restarts);
    ESP_LOGI(TAG, "Firmwaredatum: %d", compileTime());

    #if(VERBOSE)
        if(RTC_runmode == RUNMODE_POWERCYCLE) {
            esp_chip_info_t chip_info;
            esp_chip_info(&chip_info);
            ESP_LOGI(TAG, "ESP32 Chip mit %d Kernen, WiFI%s%s, Silicon Revision %d, %dMB Flash",  chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
            chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
            ESP_LOGI(TAG, "Interner Totaler Heap %d, interner freier Heap %d", ESP.getHeapSize(), ESP.getFreeHeap());
            ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
            ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
            ESP_LOGI(TAG, "Wifi/BT software coexist version %s", esp_coex_version_get());

            #if (HAS_LORA)
                ESP_LOGI(TAG, "IBM LMIC version %d.%d.%d", LMIC_VERSION_MAJOR, LMIC_VERSION_MINOR, LMIC_VERSION_BUILD);
                ESP_LOGI(TAG, "Arduino LMIC version %d.%d.%d.%d",
                    ARDUINO_LMIC_VERSION_GET_MAJOR(ARDUINO_LMIC_VERSION),
                    ARDUINO_LMIC_VERSION_GET_MINOR(ARDUINO_LMIC_VERSION),
                    ARDUINO_LMIC_VERSION_GET_PATCH(ARDUINO_LMIC_VERSION),
                    ARDUINO_LMIC_VERSION_GET_LOCAL(ARDUINO_LMIC_VERSION));
                showLoraKeys();
            #endif
        }
    #endif

    //i2c Bus initialisieren...

    // initialize LoRa
    #if (HAS_LORA)
    strcat_P(features, " LORA");
    _ASSERT(lmic_init() == ESP_OK);
    #endif

    //GPS initialisieren
    #if (HAS_GPS) 
    strcat_P(features, " GPS");
    
    #endif

    // show compiled features
    ESP_LOGI(TAG, "Features:%s", features);
    RTC_runmode = RUNMODE_NORMAL;

    vTaskDelete(NULL);
}

void loop() {
    vTaskDelete(NULL);
}