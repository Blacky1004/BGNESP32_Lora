#ifndef _TTGO_SX_H
#define _TTGO_SX_H

#include <Arduino.h>
#include <stdint.h>

#define HAS_LED LED_BUILTIN

#define HAS_LORA 1
//#define CFG_sx1272_radio 1

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_DIO1 33
#define LORA_DIO2 32

#define HAS_DISPLAY 1
#define MY_DISPLAY_SDA 21
#define MY_DISPLAY_SCL 13
#define MY_DISPLAY_ADDR 0x3C

#define HAS_GPS 1
//#define GPS_INT GPIO_NUM_37
#define GPS_SERIAL_TX 17
#define GPS_SERIAL_RX 16

#define HAS_WIFI 1

#define HAS_BME 1
#define HAS_BME280 GPIO_NUM_21, GPIO_NUM_13 // SDA, SCL
#define BME280_ADDR 0x76

// SDS011 dust sensor settings
//#define HAS_SDS011 1 // use SDS011
// used pins on the ESP-side:
#define SDS_TX 34     // connect to RX on the SDS011
#define SDS_RX 35     // connect to TX on the SDS011
#endif