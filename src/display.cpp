#ifdef HAS_DISPLAY
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes
#include "globals.h"
#include "display.h"

static uint8_t plotbuf[PLOTBUFFERSIZE] = {0};
uint8_t DisplayIsOn = 0;
hw_timer_t *displayIRQ = NULL;
Adafruit_SSD1306 display(MY_DISPLAY_WIDTH, MY_DISPLAY_HEIGHT, &Wire1, OLED_RST);
#if (HAS_DISPLAY) == 1

#else 
#error Unbekannter Displaytyp im HAL-File eingestellt!
#endif

void dp_setup(int contrast) {
#if(HAS_DISPLAY) == 1
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    ESP_LOGD(TAG,"Setup Display....");

    Wire1.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
        ESP_LOGE(TAG, "DISPLAY Failed!");
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(" FEINSTAUB ");
    display.display();
#endif
//dp_clear();
//if(contrast)
    //dp_contrast(contrast);
}

void dp_init(bool verbose) {
    dp_setup(DISPLAYCONTRAST);
    ESP_LOGD(TAG, "Init Display");
    if(verbose) {
        #if (VERBOSE)
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        // dp_setFont(MY_FONT_NORMAL);
        // dp->printf("Software v%s\r\n", PROGVERSION);
        // dp->printf("ESP32 %d cores\r\n", chip_info.cores);
        // dp->printf("Chip Rev.%d\r\n", chip_info.revision);
        // dp->printf("%dMB %s Flash", spi_flash_get_chip_size() / (1024 * 1024),
        //        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");
        // dp_dump();
        // delay(2000);
        // dp_clear();
        #endif //VERBOSE
    
    #if (HAS_LORA)
        // generate DEVEUI as QR code and text
        uint8_t buf[8], *p = buf;
        char deveui[17];
        os_getDevEui((u1_t *)buf);
        snprintf(deveui, 17, "%016llX", (*(uint64_t *)(p)));

        // display DEVEUI as QR code on the left
        //dp_contrast(30);
        //dp_printqr(3, 3, deveui);

        // display DEVEUI as plain text on the right
        // const int x_offset = QR_SCALEFACTOR * 29 + 14;
        // dp_setFont(MY_FONT_NORMAL);
        // dp->setCursor(x_offset, 0);
        // dp->printf("DEVEUI:\r\n");
        // for (uint8_t i = 0; i <= 3; i++) {
        // dp->setCursor(x_offset, i * 8 + 20);
        // dp->printf("%4.4s", deveui + i * 4);
        // }

        // give user some time to read or take picture
        //dp_dump();
    #if !(BOOTMENU)
        delay(8000);
    #endif

    #endif // HAS_LORA}
    }
    //dp_power(cfg.screenon);
}

void dp_refresh(bool nextPage) {
    static uint8_t DisplayPage = 0;
    char timeState, strftime_buf[45];
    time_t now;
    struct tm timeinfo = {0};
    #ifndef HAS_BUTTON
    static uint32_t framecounter = 0;
    const uint32_t flip_threshold = DISPLAYCYCLE * 1000 / DISPLAYREFRESH_MS;
    #endif

    // if display is switched off we don't refresh it to relax cpu
    if (!DisplayIsOn && (DisplayIsOn == cfg.screenon))
    return;

    // set display on/off according to current device configuration
    if (DisplayIsOn != cfg.screenon) {
        DisplayIsOn = cfg.screenon;
        //dp_power(cfg.screenon);
    }

    #ifndef HAS_BUTTON
    // auto flip page if we are in unattended mode
    if (++framecounter > flip_threshold) {
        framecounter = 0;
        nextPage = true;
    }
    #endif

    if (nextPage) {
        DisplayPage = (DisplayPage >= DISPLAY_PAGES - 1) ? 0 : (DisplayPage + 1);
        //dp_clear();
    } else
        //dp->setCursor(0, 0);
    
    switch (DisplayPage)
    {
        case 0:
            // dp_setFont(MY_FONT_LARGE);
            // dp->printf("*** Feinstaub ***");
            // dp->setFont(MY_FONT_SMALL);
            // dp->setCursor(0, MY_DISPLAY_FIRSTLINE);
            // dp_dump();
            display.clearDisplay();
            display.setCursor(0,0);
            display.setTextSize(2);
            display.print("Display 1");
            display.display();
            break;
        
        case 1:
            display.clearDisplay();
            display.setCursor(0,0);
            display.setTextSize(2);
            display.print("Display 2");
            display.display();

            break;
        default:
            break;
    }
}


#endif //HAS_DISPLAY