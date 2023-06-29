#ifndef _TTGO_SX_H
#define _TTGO_SX_H

#include <Arduino.h>

#define HAS_LED LED_BUILTIN
#define HAS_LORA 1
#define CFG_sx1272_radio 1

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_DIO1 33
#define LORA_DIO2 LMIC_UNUSED_PIN

#define HAS_DISPLAY 1
#define MY_DISPLAY_SDA OLED_SDA
#define MY_DISPLAY_SCL OLED_SCL
#define MY_DISPLAY_ADDR 0x3C

#endif