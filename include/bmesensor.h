#ifndef _BMESENSOR_H
#define _BMESENSOR_H

#if (HAS_BME)
#include <Wire.h>

#include "globals.h"
#include "irqhandler.h"
#include "config_bng.h"

#ifdef HAS_BME680
#include <bsec.h>
#elif defined HAS_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#elif defined HAS_BMP180
#include <Adafruit_BMP085>
#endif

extern Ticker bmecycler;
extern bmeStatus_t bme_status;

const uint8_t bsec_config_iaq[] = {
    #include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

//Hilfsfunktionen
int bme_init();
void setBMEIRQ(void);
void bme_storedata(bmeStatus_t *bme_store);
int checkIaqSensorStatus(void);
void loadState(void);
void updateState(void);

#endif //HAS_BME
#endif //_BMESENSOR_H