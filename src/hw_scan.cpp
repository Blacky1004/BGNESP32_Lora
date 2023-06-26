#include <Arduino.h>
#include <map>

std::map<std::string, byte> GetHardware(byte adress){
    std::map<std::string, byte> m;
    switch(adress){
        case 0x3c : {
            m.insert({"SSD1306" ,adress});
            m.insert({"SSD1305" ,adress});
        } break;
        case 0x76 : {
            m.insert({"BME280" ,adress});
            m.insert({"BMP280" ,adress});
            m.insert({"BME680" ,adress});
            m.insert({"BME688" ,adress});
        }
    }

    return m;
}
